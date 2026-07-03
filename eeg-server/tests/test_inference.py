"""Demo-rule classifier tests: boosted channel groups must map to their action."""

from __future__ import annotations

import math
import random
import sys
from pathlib import Path
from typing import Optional

sys.path.insert(0, str(Path(__file__).parent.parent))

from eeg_server import config
from eeg_server.inference import classify_frame

_SAMPLES_PER_FRAME = 25


def _make_frame(boost_action: Optional[str], rng: random.Random) -> list[float]:
    """Builds one sample-major frame shaped like the DroneSim simulator output."""
    boost_start = config.ACTION_GROUP_START.get(boost_action, -1) if boost_action is not None else -1
    data: list[float] = []
    for sample in range(_SAMPLES_PER_FRAME):
        time_s = sample / config.SAMPLE_RATE_HZ
        for channel in range(config.CHANNEL_COUNT):
            amplitude = 10.0
            if boost_start >= 0 and boost_start <= channel < boost_start + config.GROUP_CHANNEL_COUNT:
                amplitude *= 2.5
            rhythm = 0.8 * math.sin(2 * math.pi * 10 * time_s + channel) + 0.4 * math.sin(
                2 * math.pi * 22 * time_s + channel * 2
            )
            data.append(amplitude * rhythm + rng.gauss(0.0, 4.0))
    return data


def test_boosted_groups_classify_to_their_action() -> None:
    rng = random.Random(7)
    for action in config.ACTION_GROUP_START:
        result = classify_frame(_make_frame(action, rng), config.CHANNEL_COUNT)
        assert result.action == action, f"expected {action}, got {result.action}"
        assert result.probabilities[action] > 0.5


def test_flat_signal_classifies_to_stop() -> None:
    rng = random.Random(11)
    result = classify_frame(_make_frame(None, rng), config.CHANNEL_COUNT)
    assert result.action == config.STOP_ACTION


def test_probabilities_form_a_distribution() -> None:
    rng = random.Random(3)
    result = classify_frame(_make_frame("LEFT", rng), config.CHANNEL_COUNT)
    assert set(result.probabilities) == set(config.ACTION_PROB_ORDER)
    assert abs(sum(result.probabilities.values()) - 1.0) < 1e-6
    assert all(0.0 <= value <= 1.0 for value in result.probabilities.values())
    assert result.confidence == result.probabilities[result.action]


if __name__ == "__main__":
    test_boosted_groups_classify_to_their_action()
    test_flat_signal_classifies_to_stop()
    test_probabilities_form_a_distribution()
    print("test_inference: OK")
