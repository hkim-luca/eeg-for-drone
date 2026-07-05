"""Metric bookkeeping tests: latency matching, loss counting."""

from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from eeg_server.metrics import MetricsStore


def test_sequence_gap_counts_as_loss() -> None:
    store = MetricsStore()
    store.on_frame(0)
    store.on_frame(3)  # frames 1 and 2 were lost
    reliability = store.snapshot()["reliability"]
    assert reliability["frames_received"] == 2
    assert reliability["frames_lost"] == 2
    assert reliability["frame_percent"] == 50.0


def test_ack_records_all_latency_paths() -> None:
    store = MetricsStore()
    # frame_sent_ms/control_ms are DroneSim's clock; frame_received_ms/response_sent_ms are
    # the server's own clock - device_to_infer and infer_to_control are direct cross-clock
    # subtractions, device_to_control is the exact DroneSim-only total
    store.on_action_sent(0, infer_duration_ms=2.0, frame_sent_ms=990.0, frame_received_ms=995.0)
    store.mark_response_sent(0, response_sent_ms=1_000.0)
    sample = store.on_ack(0, control_ms=1_012.0)
    assert sample is not None
    assert sample.device_to_infer_ms == 5.0
    assert sample.infer_duration_ms == 2.0
    assert sample.infer_to_control_ms == 12.0
    assert sample.device_to_control_ms == 22.0

    latency = store.snapshot()["latency_ms"]
    assert latency["device_to_infer"]["last"] == 5.0
    assert latency["infer_duration"]["last"] == 2.0
    assert latency["infer_to_control"]["last"] == 12.0
    assert latency["device_to_control"]["last"] == 22.0

    reliability = store.snapshot()["reliability"]
    assert reliability["actions_sent"] == 1
    assert reliability["acks_received"] == 1
    assert reliability["ack_percent"] == 100.0


def test_unknown_ack_is_ignored() -> None:
    store = MetricsStore()
    assert store.on_ack(42, control_ms=5.0) is None
    assert store.snapshot()["reliability"]["acks_received"] == 0


if __name__ == "__main__":
    test_sequence_gap_counts_as_loss()
    test_ack_records_all_latency_paths()
    test_unknown_ack_is_ignored()
    print("test_metrics: OK")
