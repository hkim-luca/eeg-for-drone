"""Window buffer, MI->action mapping, and StreamClassifier tests (torch-free, via a stub)."""

from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from eeg_server import config
from eeg_server.inference import StreamClassifier
from eeg_server.model_runtime import TorchModel, WindowBuffer, mi_probs_to_action

_FRAME_SAMPLES = config.SAMPLES_PER_FRAME


def _frame(value: float = 1.0) -> list[float]:
    """One sample-major frame of constant value for every channel."""
    return [value] * (_FRAME_SAMPLES * config.CHANNEL_COUNT)


def test_window_buffer_fills_after_enough_frames() -> None:
    buffer = WindowBuffer()
    frames_needed = config.WINDOW_SAMPLES // _FRAME_SAMPLES
    for _ in range(frames_needed - 1):
        buffer.push(_frame(), config.CHANNEL_COUNT)
        assert not buffer.is_full()
    buffer.push(_frame(), config.CHANNEL_COUNT)
    assert buffer.is_full()
    matrix = buffer.matrix()
    assert len(matrix) == config.CHANNEL_COUNT
    assert all(len(channel) == config.WINDOW_SAMPLES for channel in matrix)


def test_window_buffer_ignores_mismatched_channel_count() -> None:
    buffer = WindowBuffer()
    buffer.push([0.0] * (_FRAME_SAMPLES * 8), channel_count=8)
    assert not buffer.is_full()
    assert all(len(channel) == 0 for channel in buffer.matrix())


def test_mi_probs_map_confident_class_to_its_action() -> None:
    # left_hand dominant -> LEFT (config.MI_TO_ACTION)
    action, confidence, probs = mi_probs_to_action([0.9, 0.04, 0.03, 0.03])
    assert action == "LEFT"
    assert confidence == max(probs.values())
    assert abs(sum(probs.values()) - 1.0) < 1e-9
    assert set(probs) == set(config.ACTION_PROB_ORDER)


def test_mi_probs_keep_only_movement_actions() -> None:
    action, _, probs = mi_probs_to_action([0.1, 0.2, 0.6, 0.1])
    assert action == "FORWARD"  # feet
    assert list(probs) == list(config.ACTION_PROB_ORDER)


def test_stream_classifier_warms_up_then_uses_model() -> None:
    # tongue dominant -> BACKWARD once the window is full
    model = TorchModel(predictor=lambda window: [0.05, 0.05, 0.05, 0.85], available=True)
    classifier = StreamClassifier(model=model)

    frames_needed = config.WINDOW_SAMPLES // _FRAME_SAMPLES
    for _ in range(frames_needed - 1):
        assert classifier.classify(_frame(), config.CHANNEL_COUNT) is None  # warming up
    result = classifier.classify(_frame(), config.CHANNEL_COUNT)
    assert result.action == "BACKWARD"


def test_stream_classifier_sends_nothing_without_model() -> None:
    classifier = StreamClassifier(model=TorchModel(predictor=None, available=False))
    # no model available -> no action (it is never faked from the signal)
    assert classifier.classify(_frame(), config.CHANNEL_COUNT) is None


if __name__ == "__main__":
    test_window_buffer_fills_after_enough_frames()
    test_window_buffer_ignores_mismatched_channel_count()
    test_mi_probs_map_confident_class_to_its_action()
    test_mi_probs_keep_only_movement_actions()
    test_stream_classifier_warms_up_then_uses_model()
    test_stream_classifier_sends_nothing_without_model()
    print("test_model_runtime: OK")
