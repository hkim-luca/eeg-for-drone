"""Framing and JSON message helpers for the DroneSim <-> server link.

Each message is a UTF-8 JSON object behind a 4-byte little-endian length prefix.
DroneSim sends client messages discriminated by ``"type"`` (``eeg`` frames, control
``ack`` s, ``physics`` settings); the server answers with an ``action`` result. The
field names mirror ``proto/eeg_link.proto`` (kept only as a schema reference now); the
UE side encodes/decodes the same JSON by hand in ``DroneSim/.../Eeg/EegProto.cpp``.
"""

from __future__ import annotations

import asyncio
import json
import struct
import time
from typing import Any, Final

_LENGTH_PREFIX: Final[struct.Struct] = struct.Struct("<I")

#: A message longer than this is a protocol error, not real traffic.
MAX_MESSAGE_BYTES: Final[int] = 1 << 20

#: Client message discriminators (ClientMessage.msg oneof in the proto reference).
_CLIENT_TYPES: Final[frozenset[str]] = frozenset({"eeg", "ack", "physics", "device"})


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


def parse_client_message(payload: bytes) -> dict[str, Any]:
    """Decodes one DroneSim JSON message; raises ``ValueError`` on malformed input.

    Returns the parsed object; ``message["type"]`` is one of ``eeg``/``ack``/``physics``/``device``.
    """
    try:
        message = json.loads(payload.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as error:
        raise ValueError(f"invalid JSON ClientMessage: {error}") from error
    if not isinstance(message, dict):
        raise ValueError("ClientMessage must be a JSON object")
    kind = message.get("type")
    if kind not in _CLIENT_TYPES:
        raise ValueError(f"ClientMessage has unknown type {kind!r}")
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
    """Serializes one framed JSON ``action`` result.

    ``probabilities`` must follow ``config.ACTION_PROB_ORDER``; ``metrics`` is
    ``MetricsStore.snapshot()``, forwarded so DroneSim's EEG HUD can show the same
    latency/reliability numbers as the dashboard. ``infer_duration_ms`` is a server-local
    duration (not an absolute timestamp) so it stays valid even when the server runs on a
    different, unsynchronized-clock machine than DroneSim.
    """
    device_to_infer = metrics["latency_ms"]["device_to_infer"]
    infer_latency = metrics["latency_ms"]["infer_to_control"]
    device_latency = metrics["latency_ms"]["device_to_control"]
    reliability = metrics["reliability"]

    message = {
        "type": "action",
        "action_seq": action_seq,
        "eeg_seq": eeg_seq,
        "action": action,
        "confidence": confidence,
        "infer_duration_ms": infer_duration_ms,
        "action_probs": list(probabilities),
        "latency_device_to_infer_mean_ms": device_to_infer["mean"],
        "latency_device_to_infer_last_ms": device_to_infer["last"],
        "latency_device_to_infer_p95_ms": device_to_infer["p95"],
        "latency_infer_to_control_mean_ms": infer_latency["mean"],
        "latency_infer_to_control_last_ms": infer_latency["last"],
        "latency_infer_to_control_p95_ms": infer_latency["p95"],
        "latency_device_to_control_mean_ms": device_latency["mean"],
        "latency_device_to_control_last_ms": device_latency["last"],
        "latency_device_to_control_p95_ms": device_latency["p95"],
        "reliability_overall_percent": min(reliability["frame_percent"], reliability["ack_percent"]),
        "reliability_frame_percent": reliability["frame_percent"],
        "reliability_frames_lost": reliability["frames_lost"],
        "reliability_ack_percent": reliability["ack_percent"],
    }
    return frame_payload(json.dumps(message, separators=(",", ":")).encode("utf-8"))


def now_ms() -> float:
    """Current wall clock as Unix epoch milliseconds."""
    return time.time() * 1000.0
