"""Shared state between the asyncio TCP loop and the dashboard HTTP thread.

The TCP loop is the only writer; the HTTP thread only reads snapshots.
A lock guards the handoff so the dashboard always sees a consistent view.
"""

from __future__ import annotations

import threading
from collections import deque
from typing import Deque, Final

from . import config
from .metrics import LatencySample, MetricsStore

_EMPTY_ACTION: Final[str] = "-"


class ServerState:
    """Everything the dashboard shows: waveforms, current actions, metrics."""

    def __init__(self) -> None:
        self._lock = threading.Lock()
        self.metrics = MetricsStore()
        self._waveforms: list[Deque[float]] = [
            deque(maxlen=config.GRAPH_SAMPLES_PER_CHANNEL) for _ in range(config.CHANNEL_COUNT)
        ]
        self._downsample_phase = 0
        self._connected = False
        self._inferred_action = _EMPTY_ACTION
        self._confidence = 0.0
        #: One entry per inference result, ordered by config.ACTION_PROB_ORDER (percent).
        self._prob_history: Deque[list[float]] = deque(maxlen=config.PROB_HISTORY_LENGTH)
        #: KST clock label (HH:MM:SS) of each _prob_history entry, for the chart x-axis.
        self._prob_times: Deque[str] = deque(maxlen=config.PROB_HISTORY_LENGTH)
        #: One entry per acked action: [device_to_infer, infer_duration, infer_to_control,
        #: overhead, total] in ms, for the latency pipeline line chart.
        self._latency_history: Deque[list[float]] = deque(maxlen=config.PROB_HISTORY_LENGTH)
        #: KST clock label (HH:MM:SS) of each _latency_history entry, for the chart x-axis.
        self._latency_times: Deque[str] = deque(maxlen=config.PROB_HISTORY_LENGTH)
        #: Drone physics parameters last announced by DroneSim (proto field names);
        #: empty until the first PhysicsSettings message arrives.
        self._physics_settings: dict[str, object] = {}

    def set_connected(self, connected: bool) -> None:
        with self._lock:
            self._connected = connected
            if not self._connected:
                self._inferred_action = _EMPTY_ACTION
                self._confidence = 0.0

    def on_frame(self, data: list[float], channel_count: int) -> None:
        """Feeds the dashboard waveforms with a downsampled copy of one frame."""
        with self._lock:
            sample_count = len(data) // channel_count
            for sample in range(sample_count):
                self._downsample_phase = (self._downsample_phase + 1) % config.GRAPH_DOWNSAMPLE
                if self._downsample_phase != 0:
                    continue
                base = sample * channel_count
                for channel in range(min(channel_count, config.CHANNEL_COUNT)):
                    self._waveforms[channel].append(round(data[base + channel], 2))

    def on_inference(self, action: str, confidence: float, probabilities: list[float], time_label: str) -> None:
        """Records one result; ``probabilities`` follows ``config.ACTION_PROB_ORDER``,
        ``time_label`` is the KST clock (HH:MM:SS) shown on the chart x-axis."""
        with self._lock:
            self._inferred_action = action
            self._confidence = confidence
            self._prob_history.append([round(100.0 * value, 2) for value in probabilities])
            self._prob_times.append(time_label)

    def on_latency(self, sample: LatencySample, time_label: str) -> None:
        """Records one completed action's segment breakdown; ``time_label`` is the KST clock
        (HH:MM:SS) shown on the chart x-axis. Overhead is whatever the 3 measured segments
        don't account for; total is the exact end-to-end device->control duration (not a sum
        of the other 4), same definitions as the HUD widget's pipeline chart."""
        with self._lock:
            measured_ms = sample.device_to_infer_ms + sample.infer_duration_ms + sample.infer_to_control_ms
            overhead_ms = max(sample.device_to_control_ms - measured_ms, 0.0)
            self._latency_history.append([
                round(max(sample.device_to_infer_ms, 0.0), 2),
                round(max(sample.infer_duration_ms, 0.0), 2),
                round(max(sample.infer_to_control_ms, 0.0), 2),
                round(overhead_ms, 2),
                round(max(sample.device_to_control_ms, 0.0), 2),
            ])
            self._latency_times.append(time_label)

    def on_physics_settings(self, settings: dict[str, object]) -> None:
        """Stores the physics parameters DroneSim announced, keyed by proto field name."""
        with self._lock:
            self._physics_settings = dict(settings)

    def snapshot(self) -> dict[str, object]:
        """JSON-ready dashboard state; called from the HTTP thread."""
        with self._lock:
            return {
                "connected": self._connected,
                "inferred_action": self._inferred_action,
                "confidence": round(self._confidence, 3),
                "sample_rate_hz": config.SAMPLE_RATE_HZ // config.GRAPH_DOWNSAMPLE,
                "channel_names": list(config.CHANNEL_NAMES),
                "waveforms": [list(channel) for channel in self._waveforms],
                "prob_order": list(config.ACTION_PROB_ORDER),
                "prob_history": [list(entry) for entry in self._prob_history],
                "prob_times": list(self._prob_times),
                "latency_history": [list(entry) for entry in self._latency_history],
                "latency_times": list(self._latency_times),
                "physics_settings": dict(self._physics_settings),
                "metrics": self.metrics.snapshot(),
            }
