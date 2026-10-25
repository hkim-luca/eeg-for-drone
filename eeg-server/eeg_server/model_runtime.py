"""Real AI model runtime: UnifiedEEGNet (BCI IV-2a) over a sliding 4 s window.

DroneSim streams short EEG frames (100 ms); this module keeps a per-connection
rolling buffer of the last ``WINDOW_SAMPLES`` samples per channel and, once the
buffer is full, runs the trained ``UnifiedEEGNet`` checkpoint to get 4 motor-imagery
class probabilities, which :func:`mi_probs_to_action` maps to the 4 DroneSim actions.

The pure pieces (:class:`WindowBuffer`, :func:`mi_probs_to_action`) have no heavy
dependencies and are unit-tested directly. The PyTorch parts live in
:class:`TorchModel` and are loaded lazily; if torch/numpy or the checkpoint are
unavailable, :func:`load_model` returns an unavailable stub and the server sends no
actions (see :class:`eeg_server.inference.StreamClassifier`).
"""

from __future__ import annotations

import logging
from collections import deque
from typing import Deque, Final, Optional, Protocol

from . import config

_LOGGER: Final[logging.Logger] = logging.getLogger("eeg_server.model")


class WindowBuffer:
    """Rolling per-channel buffer of the most recent ``window_samples`` samples.

    Frames arrive sample-major (``data[sample * channels + channel]``); the buffer
    stores them channel-major so :meth:`matrix` can hand the model a ``[C, T]`` view.
    """

    def __init__(self, channel_count: int = config.CHANNEL_COUNT,
                 window_samples: int = config.WINDOW_SAMPLES) -> None:
        self.channel_count = channel_count
        self.window_samples = window_samples
        self._channels: list[Deque[float]] = [deque(maxlen=window_samples) for _ in range(channel_count)]

    def reset(self) -> None:
        for channel in self._channels:
            channel.clear()

    def push(self, data: list[float], channel_count: int) -> None:
        """Appends one sample-major frame; ignores frames with a mismatched channel count."""
        if channel_count != self.channel_count:
            return
        sample_count = len(data) // channel_count
        for sample in range(sample_count):
            base = sample * channel_count
            for channel in range(channel_count):
                self._channels[channel].append(data[base + channel])

    def is_full(self) -> bool:
        return len(self._channels[0]) >= self.window_samples

    def matrix(self) -> list[list[float]]:
        """Channel-major snapshot ``[channel_count][window_samples]`` of the current window."""
        return [list(channel) for channel in self._channels]


def mi_probs_to_action(mi_probs: list[float]) -> tuple[str, float, dict[str, float]]:
    """Maps the motor-imagery class probabilities to a DroneSim action distribution.

    ``mi_probs`` follows ``config.MI_CLASSES`` order; each class adds its probability to the
    action given by ``config.MI_TO_ACTION``. Returns (action, confidence, probabilities) with
    probabilities over ``config.ACTION_PROB_ORDER`` summing to ~1.
    """
    scores = {action: 0.0 for action in config.ACTION_PROB_ORDER}
    for cls, prob in zip(config.MI_CLASSES, mi_probs):
        action = config.MI_TO_ACTION.get(cls)
        if action in scores:
            scores[action] += float(prob)

    total = sum(scores.values())
    if total <= 0.0:
        probabilities = {action: 1.0 / len(scores) for action in scores}
    else:
        probabilities = {action: score / total for action, score in scores.items()}

    action = max(probabilities, key=lambda name: probabilities[name])
    return action, probabilities[action], probabilities


def channel_layout_mismatch(model_channels: list[object]) -> Optional[str]:
    """Checks DroneSim's model-input layout (sim_to_model.json) against the model's channel count.

    Returns None when DroneSim fills exactly ``config.CHANNEL_COUNT`` inputs, else why not; frames
    of a mismatched layout must not be classified.
    """
    if len(model_channels) != config.CHANNEL_COUNT:
        return (f"sim_to_model.json has {len(model_channels)} model channels, "
                f"ai_model_config.json n_channels is {config.CHANNEL_COUNT}")
    return None


class Predictor(Protocol):
    """Callable that turns a ``[C, T]`` window into ``MODEL_N_CLASSES`` probabilities."""

    def __call__(self, window: list[list[float]]) -> list[float]: ...


class TorchModel:
    """UnifiedEEGNet checkpoint wrapped behind :meth:`predict`.

    Set ``available`` is False when torch/numpy/the checkpoint could not be loaded;
    callers then skip the model and use the fallback rule.
    """

    def __init__(self, predictor: Optional[Predictor] = None, available: bool = False) -> None:
        self._predictor = predictor
        self.available = available

    def predict(self, window: list[list[float]]) -> list[float]:
        if self._predictor is None:
            raise RuntimeError("TorchModel.predict called while unavailable")
        return self._predictor(window)


def _warn_config_mismatch(payload: dict) -> None:
    """Logs where ai_model_config.json disagrees with the metadata stored in the checkpoint."""
    import json

    expected = {
        "model_class": config.MODEL_CLASS,
        "n_channels": config.CHANNEL_COUNT,
        "n_classes": config.MODEL_N_CLASSES,
        "n_samples": config.WINDOW_SAMPLES,
    }
    for key, value in expected.items():
        if key in payload and payload[key] != value:
            _LOGGER.warning("ai_model_config.json %s=%s but checkpoint has %s", key, value, payload[key])

    meta = payload.get("checkpoint_metadata")
    if isinstance(meta, str):
        try:
            meta = json.loads(meta)
        except ValueError:
            meta = None
    if isinstance(meta, dict):
        rate = meta.get("sampling_rate_hz")
        if rate is not None and rate != config.SAMPLE_RATE_HZ:
            _LOGGER.warning("ai_model_config.json sample_rate_hz=%s but checkpoint has %s", config.SAMPLE_RATE_HZ, rate)
        _LOGGER.info("checkpoint preprocessing: alignment=%s band=%s-%s Hz", meta.get("alignment"),
                     meta.get("fmin_hz"), meta.get("fmax_hz"))


def load_model(checkpoint_path=config.MODEL_CHECKPOINT) -> TorchModel:
    """Loads the model named by config.MODEL_CLASS; returns an unavailable stub on failure."""
    try:
        import importlib
        import sys
        from pathlib import Path

        import numpy as np
        import torch

        # code/model.py is external and not a package; add it to the path and resolve the
        # configured class by name so swapping the model is config-only (no code change here).
        code_dir = Path(__file__).resolve().parents[1] / "code"
        if str(code_dir) not in sys.path:
            sys.path.insert(0, str(code_dir))
        model_cls = getattr(importlib.import_module("model"), config.MODEL_CLASS)

        if not Path(checkpoint_path).exists():
            raise FileNotFoundError(checkpoint_path)

        device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
        net = model_cls(n_classes=config.MODEL_N_CLASSES, n_channels=config.CHANNEL_COUNT).to(device)
        payload = torch.load(checkpoint_path, map_location=device)
        if isinstance(payload, dict):
            _warn_config_mismatch(payload)
        state = payload.get("model_state_dict", payload) if isinstance(payload, dict) else payload
        net.load_state_dict(state, strict=True)
        net.eval()

        def predictor(window: list[list[float]]) -> list[float]:
            x = np.asarray(window, dtype=np.float32)              # [C, T]
            # Per-channel z-score over time: the leak-free align='none' preprocessing the
            # model was trained/evaluated with (dataset._per_trial_standardize). No TTA/RA:
            # those need a set of trials, not a single live window.
            mean = x.mean(axis=1, keepdims=True)
            std = x.std(axis=1, keepdims=True) + 1e-8
            x = (x - mean) / std
            with torch.no_grad():
                logits = net(torch.from_numpy(x).unsqueeze(0).to(device))  # [1, C, T] -> [1, n_classes]
                probs = torch.softmax(logits, dim=1).squeeze(0).tolist()
            return [float(p) for p in probs]

        _LOGGER.info("AI model loaded: %s (%s, %d ch, %d samples) on %s",
                     Path(checkpoint_path).name, config.MODEL_CLASS, config.CHANNEL_COUNT, config.WINDOW_SAMPLES,
                     device)
        return TorchModel(predictor=predictor, available=True)
    except Exception as error:  # noqa: BLE001 - any failure must degrade gracefully
        _LOGGER.warning("AI model unavailable (%s); no actions will be sent until a model loads", error)
        return TorchModel(predictor=None, available=False)
