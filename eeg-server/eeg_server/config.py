"""Server constants. The AI model binding is configuration, not code.

``eeg-server/code/`` (model architectures, training, checkpoints) is external and is NOT
edited here. Which model the server serves - the class, checkpoint and input spec - is read
from ``model_config.json`` (overridable with the ``EEG_MODEL_CONFIG`` env var, or just the
checkpoint with ``EEG_MODEL_CHECKPOINT``). Serving a new model = drop the checkpoint in and
edit that file; no code change.

The model's channel count/order (``CHANNEL_NAMES``) must stay consistent with UE
``DroneSim/Source/DroneSim/Eeg/EegTypes.h`` and ``mappings/sim_to_model.json``.
"""

from __future__ import annotations

import json
import os
from pathlib import Path
from typing import Any, Final

# --- network (env: EEG_SERVER_HOST / EEG_TCP_PORT / EEG_HTTP_PORT) ----------
#: Bind address for both the DroneSim link and the dashboard; 0.0.0.0 for remote clients.
TCP_HOST: Final[str] = os.getenv("EEG_SERVER_HOST", "127.0.0.1")
TCP_PORT: Final[int] = int(os.getenv("EEG_TCP_PORT", "9800"))
HTTP_HOST: Final[str] = TCP_HOST
HTTP_PORT: Final[int] = int(os.getenv("EEG_HTTP_PORT", "8800"))

#: Samples per channel in one network frame (100 ms of signal); must match
#: EegConfig::SamplesPerFrame in EegTypes.h (wire concern, not the model).
SAMPLES_PER_FRAME: Final[int] = 25

# --- AI model binding (from model_config.json, NOT hardcoded) ---------------
_SERVER_ROOT: Final[Path] = Path(__file__).resolve().parents[1]

#: Defaults used only when the config file / a key is missing (keeps the server runnable).
_DEFAULT_CHANNEL_NAMES: Final[tuple[str, ...]] = (
    "Fz", "FC3", "FC1", "FCz", "FC2", "FC4", "C5", "C3",
    "C1", "Cz", "C2", "C4", "C6", "CP3", "CP1", "CPz",
    "CP2", "CP4", "P1", "Pz", "P2", "POz",
)
_DEFAULT_MI_CLASSES: Final[tuple[str, ...]] = ("left_hand", "right_hand", "feet", "tongue")
_DEFAULT_MI_TO_ACTION: Final[dict[str, str]] = {
    "left_hand": "LEFT", "right_hand": "RIGHT", "feet": "FORWARD", "tongue": "BACKWARD",
}


def _load_model_config() -> dict[str, Any]:
    """Loads model_config.json (EEG_MODEL_CONFIG overrides the path); {} if absent/invalid."""
    override = os.getenv("EEG_MODEL_CONFIG")
    path = Path(override) if override else _SERVER_ROOT / "model_config.json"
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

# --- EEG layout (must match EegTypes.h and mappings/sim_to_model.json) ---
CHANNEL_COUNT: Final[int] = int(_MODEL_CFG.get("n_channels", 22))
SAMPLE_RATE_HZ: Final[int] = int(_MODEL_CFG.get("sample_rate_hz", 250))

#: Model input-row electrode order; the order the checkpoint was trained on.
CHANNEL_NAMES: Final[tuple[str, ...]] = tuple(_MODEL_CFG.get("channel_names", _DEFAULT_CHANNEL_NAMES))

#: Input window the model expects: [1, CHANNEL_COUNT, WINDOW_SAMPLES].
WINDOW_SAMPLES: Final[int] = int(_MODEL_CFG.get("window_samples", 1000))
WINDOW_SECONDS: Final[float] = WINDOW_SAMPLES / SAMPLE_RATE_HZ

MODEL_N_CLASSES: Final[int] = int(_MODEL_CFG.get("n_classes", 4))

#: Model output classes, in output-logit order (index 0..n_classes-1).
MI_CLASSES: Final[tuple[str, ...]] = tuple(_MODEL_CFG.get("mi_classes", _DEFAULT_MI_CLASSES))

#: Motor-imagery class -> drone action. STOP has no MI class; it is inferred when the
#: model is not confident about any movement (see MODEL_STOP_BIAS below).
MI_TO_ACTION: Final[dict[str, str]] = dict(_MODEL_CFG.get("mi_to_action", _DEFAULT_MI_TO_ACTION))

#: STOP competes with the mapped movements using weight MODEL_STOP_BIAS * (1 - max_p),
#: where max_p is the top movement probability. Higher => STOP wins more readily when unsure.
MODEL_STOP_BIAS: Final[float] = float(_MODEL_CFG.get("stop_bias", 1.0))

STOP_ACTION: Final[str] = "STOP"

#: Wire order of ActionResult.action_probs; must match EegConfig::ProbOrder in DroneSim.
#: The action is always the AI model's decision - never encoded into the EEG signal.
ACTION_PROB_ORDER: Final[tuple[str, ...]] = ("FORWARD", "BACKWARD", "LEFT", "RIGHT", "STOP")

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
