"""Shared constants of the EEG server; the wire/demo values must match DroneSim.

The channel-group demo rule mirrors ``EegConfig`` in
``DroneSim/Source/DroneSim/Eeg/EegTypes.h``: change one side only together
with the other.
"""

from __future__ import annotations

from typing import Final

# --- network ---------------------------------------------------------------
TCP_HOST: Final[str] = "127.0.0.1"
TCP_PORT: Final[int] = 9800
HTTP_HOST: Final[str] = "127.0.0.1"
HTTP_PORT: Final[int] = 8800

# --- EEG layout (must match EegTypes.h) -------------------------------------
CHANNEL_COUNT: Final[int] = 32
SAMPLE_RATE_HZ: Final[int] = 250
GROUP_CHANNEL_COUNT: Final[int] = 6

STOP_ACTION: Final[str] = "STOP"

#: First channel of the group boosted for each movement action.
ACTION_GROUP_START: Final[dict[str, int]] = {
    "FORWARD": 0,
    "LEFT": 8,
    "RIGHT": 18,
    "BACKWARD": 26,
}

# --- demo classification rule ----------------------------------------------
#: Wire order of ActionResult.action_probs; must match EegConfig::ProbOrder in DroneSim.
ACTION_PROB_ORDER: Final[tuple[str, ...]] = ("FORWARD", "BACKWARD", "LEFT", "RIGHT", "STOP")

#: Softmax temperature over the normalized group energies; higher = more decisive.
SOFTMAX_SHARPNESS: Final[float] = 8.0

#: Baseline score of STOP; a resting signal (all groups near the mean) maps to STOP.
STOP_BIAS: Final[float] = 1.25

#: Inference results kept for the dashboard probability time series (~30 s at 10/s).
PROB_HISTORY_LENGTH: Final[int] = 300

# --- metric windows ----------------------------------------------------------
#: Latency samples kept for the rolling latency statistics.
LATENCY_WINDOW: Final[int] = 300

# --- dashboard waveform buffer ----------------------------------------------
#: Every Nth received sample is kept for the dashboard waveforms.
GRAPH_DOWNSAMPLE: Final[int] = 5

#: Downsampled samples kept per channel (2 s at 250/5 = 50 Hz).
GRAPH_SAMPLES_PER_CHANNEL: Final[int] = 100
