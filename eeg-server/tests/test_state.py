"""Dashboard state tests: waveforms follow the channels DroneSim announces."""

from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from eeg_server import config
from eeg_server.state import ServerState


def test_no_channels_until_device_announced() -> None:
    snapshot = ServerState().snapshot()
    assert snapshot["waveforms"] == []
    assert snapshot["channel_names"] == []


def test_device_limits_waveforms_to_connected_channels() -> None:
    state = ServerState()
    state.on_device([7, 9, 11], ["C3", "Cz", ""])
    frame = [float(channel) for channel in range(config.CHANNEL_COUNT)] * config.GRAPH_DOWNSAMPLE
    state.on_frame(frame, config.CHANNEL_COUNT)

    snapshot = state.snapshot()
    assert snapshot["channel_names"] == ["C3", "Cz", "ch11"]
    assert snapshot["waveforms"] == [[7.0], [9.0], [11.0]]


if __name__ == "__main__":
    test_no_channels_until_device_announced()
    test_device_limits_waveforms_to_connected_channels()
    print("test_state: OK")
