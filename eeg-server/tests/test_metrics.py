"""Metric bookkeeping tests: accuracy window, latency matching, loss counting."""

from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from eeg_server.metrics import MetricsStore


def test_accuracy_counts_matches() -> None:
    store = MetricsStore()
    store.on_frame(0, "FORWARD", "FORWARD")
    store.on_frame(1, "FORWARD", "STOP")
    store.on_frame(2, "LEFT", "LEFT")
    store.on_frame(3, "STOP", "STOP")
    snapshot = store.snapshot()
    assert snapshot["accuracy"] == {"window": 4, "percent": 75.0}


def test_sequence_gap_counts_as_loss() -> None:
    store = MetricsStore()
    store.on_frame(0, "STOP", "STOP")
    store.on_frame(3, "STOP", "STOP")  # frames 1 and 2 were lost
    reliability = store.snapshot()["reliability"]
    assert reliability["frames_received"] == 2
    assert reliability["frames_lost"] == 2
    assert reliability["frame_percent"] == 50.0


def test_ack_records_both_latency_paths() -> None:
    store = MetricsStore()
    store.on_action_sent(0, infer_ms=1_000.0, frame_sent_ms=990.0)
    store.on_ack(0, control_ms=1_012.0)
    latency = store.snapshot()["latency_ms"]
    assert latency["infer_to_control"]["last"] == 12.0
    assert latency["device_to_control"]["last"] == 22.0

    reliability = store.snapshot()["reliability"]
    assert reliability["actions_sent"] == 1
    assert reliability["acks_received"] == 1
    assert reliability["ack_percent"] == 100.0


def test_unknown_ack_is_ignored() -> None:
    store = MetricsStore()
    store.on_ack(42, control_ms=5.0)
    assert store.snapshot()["reliability"]["acks_received"] == 0


if __name__ == "__main__":
    test_accuracy_counts_matches()
    test_sequence_gap_counts_as_loss()
    test_ack_records_both_latency_paths()
    test_unknown_ack_is_ignored()
    print("test_metrics: OK")
