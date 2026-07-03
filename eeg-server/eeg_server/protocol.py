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
    infer_ms: float,
    probabilities: list[float],
) -> bytes:
    """Serializes one framed ``ServerMessage`` carrying an ``ActionResult``.
    ``probabilities`` must follow ``config.ACTION_PROB_ORDER``."""
    message = pb.ServerMessage()
    message.action.action_seq = action_seq
    message.action.eeg_seq = eeg_seq
    message.action.action = action
    message.action.confidence = confidence
    message.action.t_infer_ms = infer_ms
    message.action.action_probs.extend(probabilities)
    return frame_payload(message.SerializeToString())


def now_ms() -> float:
    """Current wall clock as Unix epoch milliseconds."""
    return time.time() * 1000.0
