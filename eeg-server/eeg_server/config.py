"""Shared constants of the EEG server; the wire/model values must match DroneSim.

The AI model input is 22 channels over a 4 s window - see ``mappings/`` at the repo root
(``sim_to_model.json``); ``CHANNEL_NAMES`` MUST match that file's order and
``EegConfig`` in ``DroneSim/Source/DroneSim/Eeg/EegTypes.h``. Change one side only together
with the others.
"""

from __future__ import annotations

from pathlib import Path
from typing import Final

# --- network ---------------------------------------------------------------
TCP_HOST: Final[str] = "127.0.0.1"
TCP_PORT: Final[int] = 9800
HTTP_HOST: Final[str] = "127.0.0.1"
HTTP_PORT: Final[int] = 8800

# --- EEG layout (must match EegTypes.h and mappings/sim_to_model.json) ---
CHANNEL_COUNT: Final[int] = 22
SAMPLE_RATE_HZ: Final[int] = 250

#: Samples per channel in one network frame (100 ms of signal); must match
#: EegConfig::SamplesPerFrame in EegTypes.h and fake_dronesim.
SAMPLES_PER_FRAME: Final[int] = 25

#: BCI Competition IV-2a 22-electrode montage, in the AI model's input-row order
#: (model index 0..21). This is the order the UnifiedEEGNet checkpoints were trained on;
#: keep it identical to mappings/sim_to_model.json.
CHANNEL_NAMES: Final[tuple[str, ...]] = (
    "Fz", "FC3", "FC1", "FCz", "FC2", "FC4", "C5", "C3",
    "C1", "Cz", "C2", "C4", "C6", "CP3", "CP1", "CPz",
    "CP2", "CP4", "P1", "Pz", "P2", "POz",
)

# --- AI model (eeg-server/code/model.py UnifiedEEGNet, BCI IV-2a) ------------
#: Input window the model expects: [1, CHANNEL_COUNT, WINDOW_SAMPLES].
WINDOW_SAMPLES: Final[int] = 1000  # 4.0 s @ 250 Hz
WINDOW_SECONDS: Final[float] = WINDOW_SAMPLES / SAMPLE_RATE_HZ

MODEL_N_CLASSES: Final[int] = 4

#: Model output classes, in output-logit order (index 0..3).
MI_CLASSES: Final[tuple[str, ...]] = ("left_hand", "right_hand", "feet", "tongue")

#: Default checkpoint: a cross-subject (LOSO) UnifiedEEGNet. Any of A01..A09 is a
#: reasonable generic model (each was trained on the other 8 subjects). Resolved
#: relative to the eeg-server package root.
_SERVER_ROOT: Final[Path] = Path(__file__).resolve().parents[1]
MODEL_CHECKPOINT: Final[Path] = _SERVER_ROOT / "checkpoints" / "loso_target_A01_best.pth"

#: Motor-imagery class -> drone action. STOP has no MI class; it is inferred when the
#: model is not confident about any movement (see MODEL_STOP_BIAS below).
MI_TO_ACTION: Final[dict[str, str]] = {
    "left_hand": "LEFT",
    "right_hand": "RIGHT",
    "feet": "FORWARD",
    "tongue": "BACKWARD",
}

#: STOP competes with the 4 mapped movements using weight MODEL_STOP_BIAS * (1 - max_p),
#: where max_p is the top movement probability. Higher => STOP wins more readily when the
#: model is unsure; a confident movement (max_p near 1) makes (1 - max_p) small so the
#: movement wins.
MODEL_STOP_BIAS: Final[float] = 1.0

STOP_ACTION: Final[str] = "STOP"

#: Wire order of ActionResult.action_probs; must match EegConfig::ProbOrder in DroneSim.
#: The action is always the AI model's decision - never encoded into the EEG signal.
ACTION_PROB_ORDER: Final[tuple[str, ...]] = ("FORWARD", "BACKWARD", "LEFT", "RIGHT", "STOP")

#: Inference results kept for the dashboard probability time series (~30 s at 10/s).
PROB_HISTORY_LENGTH: Final[int] = 300

# --- metric windows ----------------------------------------------------------
#: Latency samples kept for the rolling latency statistics.
LATENCY_WINDOW: Final[int] = 300

#: Frame/ack outcomes kept for the rolling reliability percentages; a lifetime average
#: would go stale and stop reflecting current conditions on a long-running connection.
RELIABILITY_WINDOW: Final[int] = 300

# --- dashboard waveform buffer ----------------------------------------------
#: Every Nth received sample is kept for the dashboard waveforms.
GRAPH_DOWNSAMPLE: Final[int] = 5

#: Downsampled samples kept per channel (2 s at 250/5 = 50 Hz).
GRAPH_SAMPLES_PER_CHANNEL: Final[int] = 100
