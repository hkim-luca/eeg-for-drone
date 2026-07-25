"""Framing and message helpers over the protobuf schema (proto/eeg_link.proto).

Each protobuf message travels over TCP behind a 4-byte little-endian length
prefix. DroneSim sends ``ClientMessage`` (EEG frames and control acks); the
server answers with ``ServerMessage`` (inferred action results).
"""

from __future__ import annotations

import asyncio
import struct
import time
from typing import Final

from google.protobuf.message import DecodeError

from . import eeg_link_pb2 as pb

_LENGTH_PREFIX: Final[struct.Struct] = struct.Struct("<I")

#: A message longer than this is a protocol error, not real traffic.
MAX_MESSAGE_BYTES: Final[int] = 1 << 20


def frame_payload(payload: bytes) -> bytes:
    """Prepends the 4-byte little-endian length prefix."""
    return _LENGTH_PREFIX.pack(len(payload)) + payload


async def read_payload(reader: asyncio.StreamReader) -> bytes:
    """Reads one length-prefixed payload; raises ``IncompleteReadError`` at EOF
    and ``ValueError`` when the peer announces an implausible length."""
    prefix = await reader.readexactly(_LENGTH_PREFIX.size)
    (length,) = _LENGTH_PREFIX.unpack(prefix)
    if length > MAX_MESSAGE_BYTES:
        raise ValueError(f"announced message length {length} exceeds {MAX_MESSAGE_BYTES}")
    return await reader.readexactly(length)


def parse_client_message(payload: bytes) -> pb.ClientMessage:
    """Decodes one DroneSim message; raises ``ValueError`` on malformed bytes."""
    message = pb.ClientMessage()
    try:
        message.ParseFromString(payload)
    except DecodeError as error:
        raise ValueError(f"invalid ClientMessage: {error}") from error
    if message.WhichOneof("msg") is None:
        raise ValueError("ClientMessage without payload")
    return message


def build_action_payload(
    action_seq: int,
    eeg_seq: int,
    action: str,
    confidence: float,
    infer_duration_ms: float,
    probabilities: list[float],
    metrics: dict[str, object],
) -> bytes:
    """Serializes one framed ``ServerMessage`` carrying an ``ActionResult``.
    ``probabilities`` must follow ``config.ACTION_PROB_ORDER``; ``metrics`` is
    ``MetricsStore.snapshot()``, forwarded so DroneSim's EEG HUD can show the
    same latency/reliability numbers as the dashboard. ``infer_duration_ms`` is a
    server-local duration (not an absolute timestamp) so it stays valid even when
    the server runs on a different, unsynchronized-clock machine than DroneSim."""
    message = pb.ServerMessage()
    message.action.action_seq = action_seq
    message.action.eeg_seq = eeg_seq
    message.action.action = action
    message.action.confidence = confidence
    message.action.infer_duration_ms = infer_duration_ms
    message.action.action_probs.extend(probabilities)

    device_to_infer = metrics["latency_ms"]["device_to_infer"]
    infer_latency = metrics["latency_ms"]["infer_to_control"]
    device_latency = metrics["latency_ms"]["device_to_control"]
    reliability = metrics["reliability"]
    message.action.latency_device_to_infer_mean_ms = device_to_infer["mean"]
    message.action.latency_device_to_infer_last_ms = device_to_infer["last"]
    message.action.latency_device_to_infer_p95_ms = device_to_infer["p95"]
    message.action.latency_infer_to_control_mean_ms = infer_latency["mean"]
    message.action.latency_infer_to_control_last_ms = infer_latency["last"]
    message.action.latency_infer_to_control_p95_ms = infer_latency["p95"]
    message.action.latency_device_to_control_mean_ms = device_latency["mean"]
    message.action.latency_device_to_control_last_ms = device_latency["last"]
    message.action.latency_device_to_control_p95_ms = device_latency["p95"]
    message.action.reliability_overall_percent = min(reliability["frame_percent"], reliability["ack_percent"])
    message.action.reliability_frame_percent = reliability["frame_percent"]
    message.action.reliability_frames_lost = reliability["frames_lost"]
    message.action.reliability_ack_percent = reliability["ack_percent"]
    return frame_payload(message.SerializeToString())


def now_ms() -> float:
    """Current wall clock as Unix epoch milliseconds."""
    return time.time() * 1000.0
