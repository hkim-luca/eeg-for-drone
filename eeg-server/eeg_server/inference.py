"""EEG classification: the AI model's decision over a sliding 4 s window.

:class:`StreamClassifier` is what the TCP server drives (one per connection). It buffers
incoming EEG frames into the model's 4 s window and returns the UnifiedEEGNet decision
(:mod:`eeg_server.model_runtime`). The action is ALWAYS the model's output - it is never
encoded into or recovered from the raw signal. While the window is still filling, or when
PyTorch/the checkpoint are unavailable, there is no result and the drone hovers.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Optional

from . import config
from .model_runtime import TorchModel, WindowBuffer, load_model, mi_probs_to_action


@dataclass
class InferenceResult:
    """Outcome of classifying one EEG frame / window."""

    action: str
    #: Probability of the winning action, in [0, 1].
    confidence: float
    #: Probability per action label, summing to ~1.
    probabilities: dict[str, float]


# A single model is shared across connections (weights are read-only); each connection
# keeps its own WindowBuffer. Loaded lazily so importing this module stays cheap.
_SHARED_MODEL: Optional[TorchModel] = None


def get_shared_model() -> TorchModel:
    global _SHARED_MODEL
    if _SHARED_MODEL is None:
        _SHARED_MODEL = load_model()
    return _SHARED_MODEL


class StreamClassifier:
    """Per-connection classifier: the AI model over a sliding window."""

    def __init__(self, model: Optional[TorchModel] = None) -> None:
        self._model = model if model is not None else get_shared_model()
        self._buffer = WindowBuffer()

    def reset(self) -> None:
        """Clears the window buffer; call when a new DroneSim client connects."""
        self._buffer.reset()

    @property
    def using_model(self) -> bool:
        return self._model.available

    def classify(self, data: list[float], channel_count: int) -> Optional[InferenceResult]:
        """Buffers one frame; returns the model's decision once the window is full, else None."""
        if not self._model.available:
            return None

        self._buffer.push(data, channel_count)
        if not self._buffer.is_full():
            return None
        action, confidence, probabilities = mi_probs_to_action(self._model.predict(self._buffer.matrix()))
        return InferenceResult(action=action, confidence=confidence, probabilities=probabilities)
