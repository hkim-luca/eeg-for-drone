"""Server constants. The AI model binding is configuration, not code.

``eeg-server/code/`` (model architectures, training, checkpoints) is external and is NOT
edited here. Which model the server serves - the class, checkpoint and input spec - is read
from ``ai_model_config.json`` (overridable with the ``EEG_MODEL_CONFIG`` env var, or just the
checkpoint with ``EEG_MODEL_CHECKPOINT``). Serving a new model = drop the checkpoint in and
edit that file; no code change.

Which electrode feeds which model input is not configured here: DroneSim routes signals by
name with ``release/config/sim_to_model.json``; the server only checks the channel count.
"""

from __future__ import annotations

import json
import os
from pathlib import Path
from typing import Any, Final

# --- network (env: EEG_SERVER_HOST / EEG_TCP_PORT / EEG_HTTP_PORT) ----------
#: Bind address for both the DroneSim link and the dashboard; all interfaces so a DroneSim
#: on the same PC or on another PC can connect without changing anything.
TCP_HOST: Final[str] = os.getenv("EEG_SERVER_HOST", "0.0.0.0")
TCP_PORT: Final[int] = int(os.getenv("EEG_TCP_PORT", "9800"))
HTTP_HOST: Final[str] = TCP_HOST
HTTP_PORT: Final[int] = int(os.getenv("EEG_HTTP_PORT", "8800"))

#: Dashboard address to open on this PC (a wildcard bind is reachable via loopback).
DASHBOARD_URL: Final[str] = f"http://{'127.0.0.1' if HTTP_HOST == '0.0.0.0' else HTTP_HOST}:{HTTP_PORT}/"

#: Samples per channel in one network frame (100 ms of signal); must match
#: EegConfig::SamplesPerFrame in EegTypes.h (wire concern, not the model).
SAMPLES_PER_FRAME: Final[int] = 25

# --- AI model binding (from ai_model_config.json, NOT hardcoded) ---------------
_SERVER_ROOT: Final[Path] = Path(__file__).resolve().parents[1]

#: Defaults used only when the config file / a key is missing (keeps the server runnable).
_DEFAULT_MI_CLASSES: Final[tuple[str, ...]] = ("left_hand", "right_hand", "feet", "tongue")
_DEFAULT_MI_TO_ACTION: Final[dict[str, str]] = {
    "left_hand": "LEFT", "right_hand": "RIGHT", "feet": "FORWARD", "tongue": "BACKWARD",
}


def _load_model_config() -> dict[str, Any]:
    """Loads ai_model_config.json (EEG_MODEL_CONFIG overrides the path); {} if absent/invalid.

    It sits in the shared config folder: <package>/config next to eeg-server, or release/config
    in the repository.
    """
    override = os.getenv("EEG_MODEL_CONFIG")
    candidates = [Path(override)] if override else [
        _SERVER_ROOT.parent / "config" / "ai_model_config.json",
        _SERVER_ROOT.parent / "release" / "config" / "ai_model_config.json",
    ]
    path = next((candidate for candidate in candidates if candidate.exists()), candidates[0])
    try:
        with open(path, encoding="utf-8") as handle:
            data = json.load(handle)
        return data if isinstance(data, dict) else {}
    except (OSError, json.JSONDecodeError):
        return {}


_MODEL_CFG: Final[dict[str, Any]] = _load_model_config()

#: Model architecture class name in eeg-server/code/model.py.
MODEL_CLASS: Final[str] = str(_MODEL_CFG.get("model_class", "UnifiedEEGNet"))

#: Checkpoint path (EEG_MODEL_CHECKPOINT env wins); relative paths resolve under the server root.
_checkpoint_value: Final[str] = os.getenv("EEG_MODEL_CHECKPOINT") or str(
    _MODEL_CFG.get("checkpoint", "checkpoints/loso_target_A01_best.pth")
)
MODEL_CHECKPOINT: Final[Path] = (
    Path(_checkpoint_value) if os.path.isabs(_checkpoint_value) else _SERVER_ROOT / _checkpoint_value
)

# --- EEG layout (entry count of release/config/sim_to_model.json index must match) ---
CHANNEL_COUNT: Final[int] = int(_MODEL_CFG.get("n_channels", 22))
SAMPLE_RATE_HZ: Final[int] = int(_MODEL_CFG.get("sample_rate_hz", 250))

#: Input window the model expects: [1, CHANNEL_COUNT, WINDOW_SAMPLES].
WINDOW_SAMPLES: Final[int] = int(_MODEL_CFG.get("window_samples", 1000))
WINDOW_SECONDS: Final[float] = WINDOW_SAMPLES / SAMPLE_RATE_HZ

MODEL_N_CLASSES: Final[int] = int(_MODEL_CFG.get("n_classes", 4))

#: Model output classes, in output-logit order (index 0..n_classes-1).
MI_CLASSES: Final[tuple[str, ...]] = tuple(_MODEL_CFG.get("mi_classes", _DEFAULT_MI_CLASSES))

#: Motor-imagery class -> drone action.
MI_TO_ACTION: Final[dict[str, str]] = dict(_MODEL_CFG.get("mi_to_action", _DEFAULT_MI_TO_ACTION))

#: Wire order of ActionResult.action_probs; must match EegConfig::ProbOrder in DroneSim.
#: The action is always the AI model's decision - never encoded into the EEG signal.
ACTION_PROB_ORDER: Final[tuple[str, ...]] = ("FORWARD", "BACKWARD", "LEFT", "RIGHT")

# --- dashboard / metrics (server-only) --------------------------------------
#: Inference results kept for the dashboard probability time series (~30 s at 10/s).
PROB_HISTORY_LENGTH: Final[int] = 300

#: Latency samples kept for the rolling latency statistics.
LATENCY_WINDOW: Final[int] = 300

#: Frame/ack outcomes kept for the rolling reliability percentages; a lifetime average
#: would go stale and stop reflecting current conditions on a long-running connection.
RELIABILITY_WINDOW: Final[int] = 300

#: Every Nth received sample is kept for the dashboard waveforms.
GRAPH_DOWNSAMPLE: Final[int] = 5

#: Downsampled samples kept per channel (2 s at 250/5 = 50 Hz).
GRAPH_SAMPLES_PER_CHANNEL: Final[int] = 100
