"""Evaluation metrics of the EEG pipeline, computed server-side.

Three metrics are tracked, matching the project's evaluation criteria:

* **accuracy** - EEG command classification accuracy: inferred action vs the
  ground truth DroneSim embeds in each simulated frame.
* **latency** - real-time responsiveness: inference finished -> pawn control
  applied (from the ack), plus the full device -> control path.
* **reliability** - communication reliability of the device -> DroneSim ->
  control chain: frame loss detected via sequence gaps, and the share of
  inferred actions confirmed by a control ack.
"""

from __future__ import annotations

from collections import deque
from dataclasses import dataclass
from typing import Deque, Optional

from . import config


@dataclass
class PendingAction:
    """Action sent to DroneSim, waiting for its control ack."""

    infer_ms: float
    frame_sent_ms: float


class MetricsStore:
    """Rolling metrics; single-threaded (owned by the asyncio loop)."""

    def __init__(self) -> None:
        self._accuracy_window: Deque[bool] = deque(maxlen=config.ACCURACY_WINDOW)
        self._infer_to_control_ms: Deque[float] = deque(maxlen=config.LATENCY_WINDOW)
        self._device_to_control_ms: Deque[float] = deque(maxlen=config.LATENCY_WINDOW)
        self._pending_actions: dict[int, PendingAction] = {}
        self._last_frame_seq: Optional[int] = None
        self.frames_received: int = 0
        self.frames_lost: int = 0
        self.actions_sent: int = 0
        self.acks_received: int = 0

    def on_frame(self, seq: int, true_action: str, inferred_action: str) -> None:
        """Accounts one received frame and its classification result."""
        self.frames_received += 1
        if self._last_frame_seq is not None and seq > self._last_frame_seq + 1:
            self.frames_lost += seq - self._last_frame_seq - 1
        if self._last_frame_seq is None or seq > self._last_frame_seq:
            self._last_frame_seq = seq
        self._accuracy_window.append(inferred_action == true_action)

    def on_action_sent(self, action_seq: int, infer_ms: float, frame_sent_ms: float) -> None:
        """Registers a sent action so its ack can be matched later."""
        self.actions_sent += 1
        self._pending_actions[action_seq] = PendingAction(infer_ms=infer_ms, frame_sent_ms=frame_sent_ms)
        # an ack that never arrives must not leak; the window bounds the wait
        if len(self._pending_actions) > config.LATENCY_WINDOW:
            oldest = min(self._pending_actions)
            del self._pending_actions[oldest]

    def on_ack(self, action_seq: int, control_ms: float) -> None:
        """Matches a control ack and records both latency paths."""
        pending = self._pending_actions.pop(action_seq, None)
        if pending is None:
            return
        self.acks_received += 1
        self._infer_to_control_ms.append(control_ms - pending.infer_ms)
        self._device_to_control_ms.append(control_ms - pending.frame_sent_ms)

    def accuracy_percent(self) -> float:
        """Rolling classification accuracy, also sent to DroneSim with each action."""
        return _percent(sum(self._accuracy_window), len(self._accuracy_window))

    def reset_stream(self) -> None:
        """Called when DroneSim reconnects; sequence numbering starts over."""
        self._last_frame_seq = None
        self._pending_actions.clear()

    def snapshot(self) -> dict[str, object]:
        """JSON-ready summary for the dashboard."""
        return {
            "accuracy": {
                "window": len(self._accuracy_window),
                "percent": _percent(sum(self._accuracy_window), len(self._accuracy_window)),
            },
            "latency_ms": {
                "infer_to_control": _stats(self._infer_to_control_ms),
                "device_to_control": _stats(self._device_to_control_ms),
            },
            "reliability": {
                "frames_received": self.frames_received,
                "frames_lost": self.frames_lost,
                "frame_percent": _percent(self.frames_received, self.frames_received + self.frames_lost),
                "actions_sent": self.actions_sent,
                "acks_received": self.acks_received,
                "ack_percent": _percent(self.acks_received, self.actions_sent),
            },
        }


def _percent(part: float, whole: float) -> float:
    return round(100.0 * part / whole, 2) if whole > 0 else 0.0


def _stats(values: Deque[float]) -> dict[str, float]:
    if not values:
        return {"last": 0.0, "mean": 0.0, "p95": 0.0}
    ordered = sorted(values)
    p95_index = min(len(ordered) - 1, int(0.95 * len(ordered)))
    return {
        "last": round(values[-1], 2),
        "mean": round(sum(values) / len(values), 2),
        "p95": round(ordered[p95_index], 2),
    }
