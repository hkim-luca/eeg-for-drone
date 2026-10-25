"""JSON wire-format round-trip tests (no protobuf, no torch)."""

from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from eeg_server import config
from eeg_server.protocol import build_action_payload, parse_client_message

_PREFIX = struct.Struct("<I")


def _encode(message: dict) -> bytes:
    return json.dumps(message).encode("utf-8")


def _fake_metrics() -> dict:
    leg = {"mean": 1.0, "last": 2.0, "p95": 3.0}
    return {
        "latency_ms": {"device_to_infer": leg, "infer_to_control": leg, "device_to_control": leg},
        "reliability": {"frame_percent": 99.0, "ack_percent": 98.0, "frames_lost": 1},
    }


def test_parse_client_message_accepts_each_type() -> None:
    for kind in ("eeg", "ack", "physics"):
        message = parse_client_message(_encode({"type": kind, "x": 1}))
        assert message["type"] == kind


def test_parse_client_message_rejects_bad_input() -> None:
    for payload in (b"not json", _encode([1, 2, 3]), _encode({"type": "nope"}), _encode({"no": "type"})):
        try:
            parse_client_message(payload)
        except ValueError:
            continue
        raise AssertionError(f"expected ValueError for {payload!r}")


def test_build_action_payload_round_trips() -> None:
    probs = [0.1, 0.2, 0.4, 0.3]
    framed = build_action_payload(
        action_seq=7, eeg_seq=42, action="LEFT", confidence=0.4,
        infer_duration_ms=1.5, probabilities=probs, metrics=_fake_metrics(),
    )
    (length,) = _PREFIX.unpack(framed[: _PREFIX.size])
    body = framed[_PREFIX.size:]
    assert length == len(body)

    message = json.loads(body.decode("utf-8"))
    assert message["type"] == "action"
    assert message["action_seq"] == 7
    assert message["eeg_seq"] == 42
    assert message["action"] == "LEFT"
    assert message["action_probs"] == probs
    assert len(message["action_probs"]) == len(config.ACTION_PROB_ORDER)
    assert message["reliability_overall_percent"] == 98.0  # min(frame, ack)
    assert message["latency_device_to_infer_p95_ms"] == 3.0


if __name__ == "__main__":
    test_parse_client_message_accepts_each_type()
    test_parse_client_message_rejects_bad_input()
    test_build_action_payload_round_trips()
    print("test_protocol: OK")
