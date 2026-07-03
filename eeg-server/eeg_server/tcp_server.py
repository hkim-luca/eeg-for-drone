"""Asyncio TCP endpoint DroneSim connects to.

One DroneSim client at a time: each ``EegFrame`` is classified immediately and
the resulting ``ActionResult`` is sent back; ``ControlAck`` messages close the
latency loop. Metrics and dashboard state are updated on the way. Messages are
length-prefixed protobuf (see protocol.py).
"""

from __future__ import annotations

import asyncio
import logging
from typing import Final

from . import config
from . import eeg_link_pb2 as pb
from .inference import classify_frame
from .metrics_log import MetricsCsvLogger, now_kst
from .protocol import MAX_MESSAGE_BYTES, build_action_payload, now_ms, parse_client_message, read_payload
from .state import ServerState

_LOGGER: Final[logging.Logger] = logging.getLogger("eeg_server.tcp")


class EegTcpServer:
    """Serves the DroneSim EEG link on ``config.TCP_HOST:config.TCP_PORT``."""

    def __init__(self, state: ServerState) -> None:
        self._state = state
        self._next_action_seq = 0
        self._csv_log = MetricsCsvLogger()

    async def serve_forever(self) -> None:
        server = await asyncio.start_server(
            self._handle_client, config.TCP_HOST, config.TCP_PORT, limit=MAX_MESSAGE_BYTES
        )
        _LOGGER.info("EEG TCP server listening on %s:%d", config.TCP_HOST, config.TCP_PORT)
        async with server:
            await server.serve_forever()

    async def _handle_client(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter) -> None:
        peer = writer.get_extra_info("peername")
        _LOGGER.info("DroneSim connected: %s", peer)
        self._state.set_connected(True)
        self._state.metrics.reset_stream()

        try:
            while True:
                payload = await read_payload(reader)
                try:
                    message = parse_client_message(payload)
                except ValueError as error:
                    _LOGGER.warning("dropping malformed message: %s", error)
                    continue
                if message.WhichOneof("msg") == "eeg":
                    await self._handle_frame(message.eeg, writer)
                else:
                    self._state.metrics.on_ack(message.ack.action_seq, message.ack.t_control_ms)
        except (asyncio.IncompleteReadError, ConnectionResetError):
            pass  # DroneSim closed the link; it reconnects on its own
        except ValueError as error:
            _LOGGER.error("closing connection after framing error: %s", error)
        finally:
            self._state.set_connected(False)
            writer.close()
            _LOGGER.info("DroneSim disconnected: %s", peer)

    async def _handle_frame(self, frame: pb.EegFrame, writer: asyncio.StreamWriter) -> None:
        """Classifies one frame and streams the inferred action back."""
        if frame.channels <= 0 or len(frame.data) % frame.channels != 0:
            _LOGGER.warning("frame %d: %d values do not divide into %d channels", frame.seq, len(frame.data),
                            frame.channels)
            return

        data = list(frame.data)
        result = classify_frame(data, frame.channels)
        infer_ms = now_ms()
        moment = now_kst()
        probabilities = [result.probabilities[action] for action in config.ACTION_PROB_ORDER]

        self._state.metrics.on_frame(frame.seq)
        self._state.on_frame(data, frame.channels)
        self._state.on_inference(result.action, result.confidence, probabilities, moment.strftime("%H:%M:%S"))

        action_seq = self._next_action_seq
        self._next_action_seq += 1
        self._state.metrics.on_action_sent(action_seq, infer_ms, frame.t_sent_ms)

        # persist every dashboard value as one KST-stamped time-series row
        metrics = self._state.metrics.snapshot()
        latency = metrics["latency_ms"]
        reliability = metrics["reliability"]
        self._csv_log.log_row(moment, result.action, result.confidence,
                              [100.0 * value for value in probabilities],
                              latency["infer_to_control"]["last"], latency["device_to_control"]["last"],
                              reliability["frame_percent"], reliability["ack_percent"])

        writer.write(build_action_payload(action_seq, frame.seq, result.action, result.confidence, infer_ms,
                                          probabilities))
        await writer.drain()
