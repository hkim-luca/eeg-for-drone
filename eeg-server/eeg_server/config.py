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
#: A group must exceed the mean RMS of the other groups by this factor to win.
CLASSIFY_RATIO: Final[float] = 1.5

# --- metric windows ----------------------------------------------------------
#: Frames kept for the rolling accuracy metric (~30 s at 10 frames/s).
ACCURACY_WINDOW: Final[int] = 300

#: Latency samples kept for the rolling latency statistics.
LATENCY_WINDOW: Final[int] = 300

# --- dashboard waveform buffer ----------------------------------------------
#: Every Nth received sample is kept for the dashboard waveforms.
GRAPH_DOWNSAMPLE: Final[int] = 5

#: Downsampled samples kept per channel (2 s at 250/5 = 50 Hz).
GRAPH_SAMPLES_PER_CHANNEL: Final[int] = 100
