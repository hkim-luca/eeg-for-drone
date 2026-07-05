"""Evaluation metrics of the EEG pipeline, computed server-side.

Two metrics are tracked, matching the project's evaluation criteria:

* **latency** - real-time responsiveness, split into the 3 pipeline segments
  (device->infer, infer processing, infer->control) plus the full device->control
  total. device->infer and infer->control are direct cross-clock subtractions
  (DroneSim's clock vs. the server's own clock) - correct as long as the two
  clocks read the same, which holds today (both run on 127.0.0.1) and on any
  closely time-synced deployment.
* **reliability** - communication reliability of the device -> DroneSim ->
  control chain: frame loss detected via sequence gaps, and the share of
  inferred actions confirmed by a control ack. The percentages are rolling
  (config.RELIABILITY_WINDOW outcomes), not a lifetime average, so they keep
  reflecting current conditions on a long-running connection.
"""

from __future__ import annotations

from collections import deque
from dataclasses import dataclass
from typing import Deque, Optional

from . import config


@dataclass
class LatencySample:
    """One completed action's full segment breakdown, for the latency history charts."""

    device_to_infer_ms: float
    infer_duration_ms: float
    infer_to_control_ms: float
    device_to_control_ms: float


@dataclass
class PendingAction:
    """Action sent to DroneSim, waiting for its control ack."""

    #: Server-local duration (ms) spent classifying the frame; NOT an absolute timestamp.
    infer_duration_ms: float
    frame_sent_ms: float
    #: device->infer leg, already known when the action is sent (see on_action_sent).
    device_to_infer_ms: float
    #: Server clock (ms) when the ActionResult was written to the socket; filled in by
    #: mark_response_sent shortly after on_action_sent, once the response is actually sent.
    response_sent_ms: float = 0.0


class MetricsStore:
    """Rolling metrics; single-threaded (owned by the asyncio loop)."""

    def __init__(self) -> None:
        #: Server-only duration (classify_frame), the one exactly-measurable pipeline segment.
        self._infer_duration_ms: Deque[float] = deque(maxlen=config.LATENCY_WINDOW)
        #: device->infer leg: server's frame-received clock minus DroneSim's frame-sent clock.
        self._device_to_infer_ms: Deque[float] = deque(maxlen=config.LATENCY_WINDOW)
        #: infer->control leg: DroneSim's control-applied clock minus the server's response-sent clock.
        self._infer_to_control_ms: Deque[float] = deque(maxlen=config.LATENCY_WINDOW)
        self._device_to_control_ms: Deque[float] = deque(maxlen=config.LATENCY_WINDOW)
        #: Rolling per-frame/per-action outcomes backing frame_percent/ack_percent below.
        self._frame_outcomes: Deque[bool] = deque(maxlen=config.RELIABILITY_WINDOW)
        self._ack_outcomes: Deque[bool] = deque(maxlen=config.RELIABILITY_WINDOW)
        self._pending_actions: dict[int, PendingAction] = {}
        self._last_frame_seq: Optional[int] = None
        self.frames_received: int = 0
        self.frames_lost: int = 0
        self.actions_sent: int = 0
        self.acks_received: int = 0

    def on_frame(self, seq: int) -> None:
        """Accounts one received frame for loss/sequencing tracking."""
        self.frames_received += 1
        if self._last_frame_seq is not None and seq > self._last_frame_seq + 1:
            lost = seq - self._last_frame_seq - 1
            self.frames_lost += lost
            for _ in range(lost):
                self._frame_outcomes.append(False)
        if self._last_frame_seq is None or seq > self._last_frame_seq:
            self._last_frame_seq = seq
        self._frame_outcomes.append(True)

    def on_action_sent(self, action_seq: int, infer_duration_ms: float, frame_sent_ms: float,
                       frame_received_ms: float) -> None:
        """Registers a sent action so its ack can be matched later.

        ``frame_received_ms`` (server clock) minus ``frame_sent_ms`` (DroneSim clock) is the
        device->infer leg; recorded immediately since both are already known."""
        self.actions_sent += 1
        device_to_infer_ms = frame_received_ms - frame_sent_ms
        self._infer_duration_ms.append(infer_duration_ms)
        self._device_to_infer_ms.append(device_to_infer_ms)
        self._pending_actions[action_seq] = PendingAction(infer_duration_ms=infer_duration_ms,
                                                           frame_sent_ms=frame_sent_ms,
                                                           device_to_infer_ms=device_to_infer_ms)
        # an ack that never arrives must not leak; the window bounds the wait, and a
        # timed-out entry counts as a miss for the rolling ack-reliability window
        if len(self._pending_actions) > config.LATENCY_WINDOW:
            oldest = min(self._pending_actions)
            del self._pending_actions[oldest]
            self._ack_outcomes.append(False)

    def mark_response_sent(self, action_seq: int, response_sent_ms: float) -> None:
        """Records when the ActionResult was actually written to the socket, so on_ack can
        later compute the infer->control leg against it."""
        pending = self._pending_actions.get(action_seq)
        if pending is not None:
            pending.response_sent_ms = response_sent_ms

    def on_ack(self, action_seq: int, control_ms: float) -> Optional["LatencySample"]:
        """Matches a control ack and records the remaining latency paths.

        ``control_ms`` (DroneSim clock) minus ``pending.response_sent_ms`` (server clock) is
        the infer->control leg; ``control_ms`` minus ``pending.frame_sent_ms`` (both DroneSim's
        own clock) is the exact device->control total. Returns the completed segment breakdown
        for this action (for the dashboard/HUD latency history charts), or None if the ack
        doesn't match a pending action."""
        pending = self._pending_actions.pop(action_seq, None)
        if pending is None:
            return None
        self.acks_received += 1
        self._ack_outcomes.append(True)
        device_to_control_ms = control_ms - pending.frame_sent_ms
        self._device_to_control_ms.append(device_to_control_ms)
        infer_to_control_ms = 0.0
        if pending.response_sent_ms > 0.0:
            infer_to_control_ms = control_ms - pending.response_sent_ms
            self._infer_to_control_ms.append(infer_to_control_ms)
        return LatencySample(device_to_infer_ms=pending.device_to_infer_ms,
                             infer_duration_ms=pending.infer_duration_ms,
                             infer_to_control_ms=infer_to_control_ms,
                             device_to_control_ms=device_to_control_ms)

    def reset_stream(self) -> None:
        """Called when DroneSim reconnects; sequence numbering starts over."""
        self._last_frame_seq = None
        self._pending_actions.clear()

    def snapshot(self) -> dict[str, object]:
        """JSON-ready summary for the dashboard."""
        return {
            "latency_ms": {
                "device_to_infer": _stats(self._device_to_infer_ms),
                "infer_duration": _stats(self._infer_duration_ms),
                "infer_to_control": _stats(self._infer_to_control_ms),
                "device_to_control": _stats(self._device_to_control_ms),
            },
            "reliability": {
                "frames_received": self.frames_received,
                "frames_lost": self.frames_lost,
                "frame_percent": _percent(sum(self._frame_outcomes), len(self._frame_outcomes)),
                "actions_sent": self.actions_sent,
                "acks_received": self.acks_received,
                "ack_percent": _percent(sum(self._ack_outcomes), len(self._ack_outcomes)),
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
