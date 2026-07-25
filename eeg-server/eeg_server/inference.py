"""Dummy EEG classifier implementing the fixed demo rule.

DroneSim's simulated device boosts the amplitude of one channel group per
movement action (see ``config.ACTION_GROUP_START``). This classifier turns the
RMS energy of each group into a probability per action with a softmax: a group
that clearly dominates pulls the distribution toward its action, while a
resting signal (all groups near the mean) favors STOP through a fixed bias.
The inferred action is the argmax, so the label always matches the reported
distribution. The real AI model will replace this module while keeping
:func:`classify_frame`'s signature.
"""

from __future__ import annotations

import math
from dataclasses import dataclass

from . import config


@dataclass
class InferenceResult:
    """Outcome of classifying one EEG frame."""

    action: str
    #: Probability of the winning action, in [0, 1].
    confidence: float
    #: Probability per action label, summing to ~1.
    probabilities: dict[str, float]


def _group_rms(data: list[float], channel_count: int, group_start: int) -> float:
    """RMS over all samples of the ``GROUP_CHANNEL_COUNT`` channels of one group."""
    total = 0.0
    count = 0
    sample_count = len(data) // channel_count
    for sample in range(sample_count):
        base = sample * channel_count
        for channel in range(group_start, group_start + config.GROUP_CHANNEL_COUNT):
            value = data[base + channel]
            total += value * value
            count += 1
    return math.sqrt(total / count) if count > 0 else 0.0


def classify_frame(data: list[float], channel_count: int) -> InferenceResult:
    """Classifies one sample-major EEG frame into a per-action probability distribution."""
    rms_by_action = {
        action: _group_rms(data, channel_count, start)
        for action, start in config.ACTION_GROUP_START.items()
    }
    mean_rms = sum(rms_by_action.values()) / len(rms_by_action)

    # normalized group energy; 1.0 = average. STOP competes with a fixed baseline so
    # it wins whenever no movement group stands out from the rest.
    scores = {
        action: (rms / mean_rms if mean_rms > 0.0 else 1.0)
        for action, rms in rms_by_action.items()
    }
    scores[config.STOP_ACTION] = config.STOP_BIAS

    peak = max(scores.values())  # subtracted for numerical stability
    weights = {action: math.exp(config.SOFTMAX_SHARPNESS * (score - peak)) for action, score in scores.items()}
    total_weight = sum(weights.values())
    probabilities = {action: weight / total_weight for action, weight in weights.items()}

    action = max(probabilities, key=lambda name: probabilities[name])
    return InferenceResult(action=action, confidence=probabilities[action], probabilities=probabilities)
