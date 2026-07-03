"""Dummy EEG classifier implementing the fixed demo rule.

DroneSim's simulated device boosts the amplitude of one channel group per
movement action (see ``config.ACTION_GROUP_START``). This classifier computes
the RMS of each group and picks the group that clearly dominates; when no
group dominates, the frame is a STOP. The real AI model will replace this
module while keeping :func:`classify_frame`'s signature.
"""

from __future__ import annotations

import math
from dataclasses import dataclass

from . import config


@dataclass
class InferenceResult:
    """Outcome of classifying one EEG frame."""

    action: str
    confidence: float


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
    """Classifies one sample-major EEG frame into a scenario action label."""
    rms_by_action = {
        action: _group_rms(data, channel_count, start)
        for action, start in config.ACTION_GROUP_START.items()
    }

    best_action = max(rms_by_action, key=lambda action: rms_by_action[action])
    best_rms = rms_by_action[best_action]
    other_rms = [rms for action, rms in rms_by_action.items() if action != best_action]
    other_mean = sum(other_rms) / len(other_rms) if other_rms else 0.0

    if other_mean <= 0.0 or best_rms / other_mean < config.CLASSIFY_RATIO:
        # no group dominates: resting signal, which the demo rule maps to STOP.
        # Confidence grows as the best group falls back toward the baseline.
        margin = best_rms / other_mean if other_mean > 0.0 else 1.0
        confidence = min(1.0, max(0.0, (config.CLASSIFY_RATIO - margin) / (config.CLASSIFY_RATIO - 1.0)))
        return InferenceResult(action=config.STOP_ACTION, confidence=confidence)

    confidence = min(1.0, (best_rms / other_mean) / (config.CLASSIFY_RATIO * 2.0))
    return InferenceResult(action=best_action, confidence=confidence)
