"""Dummy EEG inference server for DroneSim running mode.

Receives simulated 32-electrode EEG frames from DroneSim over TCP, classifies
them into scenario actions with a fixed demo rule (the real AI model will
replace :mod:`eeg_server.inference` later), streams the inferred actions back,
and serves a web dashboard with the evaluation metrics.
"""

from __future__ import annotations

__version__ = "0.1.0"
