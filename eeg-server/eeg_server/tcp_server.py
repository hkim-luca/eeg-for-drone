"""Asyncio TCP endpoint DroneSim connects to.

One DroneSim client at a time: each EEG frame is fed to a per-connection
:class:`StreamClassifier` (the real AI model over a sliding 4 s window when available,
else the demo rule) and the resulting action is sent back; control acks close the
latency loop. Metrics and dashboard state are updated on the way. Messages are
length-prefixed UTF-8 JSON (see protocol.py).
"""

from __future__ import annotations

import asyncio
import logging
import math
from typing import Any, Final

from . import config
from .inference import StreamClassifier
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

        # Fresh classifier per connection: a new sliding window, no leftover samples.
        classifier = StreamClassifier()
        _LOGGER.info("classifier: %s", "AI model" if classifier.using_model else "no model (holding STOP)")

        try:
            while True:
                payload = await read_payload(reader)
                try:
                    message = parse_client_message(payload)
                except ValueError as error:
                    _LOGGER.warning("dropping malformed message: %s", error)
                    continue
                kind = message["type"]
                if kind == "eeg":
                    await self._handle_frame(message, classifier, writer)
                elif kind == "physics":
                    # a NaN/Infinity double would make the /api/state JSON unparseable
                    # in the browser and freeze the dashboard; drop non-finite values
                    settings = {
                        name: value for name, value in message.items()
                        if name != "type" and not (isinstance(value, float) and not math.isfinite(value))
                    }
                    self._state.on_physics_settings(settings)
                    _LOGGER.info("physics settings updated: %d fields", len(settings))
                else:  # ack
                    latency = self._state.metrics.on_ack(message["action_seq"], message["t_control_ms"])
                    if latency is not None:
                        self._state.on_latency(latency, now_kst().strftime("%H:%M:%S"))
        except (asyncio.IncompleteReadError, ConnectionResetError):
            pass  # DroneSim closed the link; it reconnects on its own
        except ValueError as error:
            _LOGGER.error("closing connection after framing error: %s", error)
        finally:
            self._state.set_connected(False)
            writer.close()
            _LOGGER.info("DroneSim disconnected: %s", peer)

    async def _handle_frame(self, frame: dict[str, Any], classifier: StreamClassifier,
                            writer: asyncio.StreamWriter) -> None:
        """Classifies one frame and streams the inferred action back."""
        # server's own clock as early as possible - this is the "device->infer" leg's
        # server-side endpoint, compared directly against t_sent_ms (DroneSim's clock)
        frame_received_ms = now_ms()

        seq = int(frame.get("seq", 0))
        t_sent_ms = float(frame.get("t_sent_ms", 0.0))
        channels = int(frame.get("channels", 0))
        data = frame.get("data", [])
        if channels <= 0 or not isinstance(data, list) or len(data) % channels != 0:
            _LOGGER.warning("frame %d: %d values do not divide into %d channels", seq, len(data), channels)
            return

        result = classifier.classify(data, channels)
        infer_duration_ms = now_ms() - frame_received_ms
        moment = now_kst()
        probabilities = [result.probabilities[action] for action in config.ACTION_PROB_ORDER]

        self._state.metrics.on_frame(seq)
        self._state.on_frame(data, channels)
        self._state.on_inference(result.action, result.confidence, probabilities, moment.strftime("%H:%M:%S"))

        action_seq = self._next_action_seq
        self._next_action_seq += 1
        self._state.metrics.on_action_sent(action_seq, infer_duration_ms, t_sent_ms, frame_received_ms)

        # persist every dashboard value as one KST-stamped time-series row
        metrics = self._state.metrics.snapshot()
        latency = metrics["latency_ms"]
        reliability = metrics["reliability"]
        self._csv_log.log_row(moment, result.action, result.confidence,
                              [100.0 * value for value in probabilities],
                              latency["infer_to_control"]["last"], latency["device_to_control"]["last"],
                              reliability["frame_percent"], reliability["ack_percent"])

        # response_sent_ms is the "infer->control" leg's server-side endpoint, compared
        # directly against ControlAck.t_control_ms (DroneSim's clock) once the ack arrives
        self._state.metrics.mark_response_sent(action_seq, now_ms())
        writer.write(build_action_payload(action_seq, seq, result.action, result.confidence,
                                          infer_duration_ms, probabilities, metrics))
        await writer.drain()
