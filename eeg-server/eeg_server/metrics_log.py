"""Time-series CSV log of the dashboard values.

One file per server run under ``logs/``, one row per inference result (10/s).
The first column is the KST wall clock; the remaining columns mirror what the
dashboard shows: inferred action, per-action probabilities and the latest
latency/reliability figures.
"""

from __future__ import annotations

import datetime
from pathlib import Path
from typing import Final, Optional, TextIO

from . import config

KST: Final[datetime.timezone] = datetime.timezone(datetime.timedelta(hours=9), name="KST")

_LOG_DIR: Final[Path] = Path(__file__).parent.parent / "logs"

_HEADER: Final[str] = ",".join(
    ["kst", "inferred_action", "confidence"]
    + [f"{action.lower()}_pct" for action in config.ACTION_PROB_ORDER]
    + ["infer_to_control_ms", "device_to_control_ms", "frame_reliability_pct", "ack_reliability_pct"]
)


def now_kst() -> datetime.datetime:
    """Current wall clock in KST (UTC+9)."""
    return datetime.datetime.now(tz=KST)


def format_kst(moment: datetime.datetime) -> str:
    """KST timestamp with millisecond precision, e.g. ``2026-07-03 11:18:16.362+09:00``."""
    return moment.strftime("%Y-%m-%d %H:%M:%S.") + f"{moment.microsecond // 1000:03d}+09:00"


class MetricsCsvLogger:
    """Appends one row per inference result; the file is created on the first row."""

    def __init__(self) -> None:
        self._file: Optional[TextIO] = None
        self._path: Optional[Path] = None

    def log_row(
        self,
        moment: datetime.datetime,
        inferred_action: str,
        confidence: float,
        probabilities_pct: list[float],
        infer_to_control_ms: float,
        device_to_control_ms: float,
        frame_reliability_pct: float,
        ack_reliability_pct: float,
    ) -> None:
        """Writes one KST-stamped row; ``probabilities_pct`` follows ACTION_PROB_ORDER."""
        writer = self._ensure_file(moment)
        values = (
            [format_kst(moment), inferred_action, f"{confidence:.4f}"]
            + [f"{value:.2f}" for value in probabilities_pct]
            + [f"{infer_to_control_ms:.2f}", f"{device_to_control_ms:.2f}",
               f"{frame_reliability_pct:.2f}", f"{ack_reliability_pct:.2f}"]
        )
        writer.write(",".join(values) + "\n")
        writer.flush()  # a crash must not lose the run's evaluation data

    def get_path(self) -> Optional[Path]:
        """Path of the CSV being written, or ``None`` before the first row."""
        return self._path

    def _ensure_file(self, moment: datetime.datetime) -> TextIO:
        if self._file is None:
            _LOG_DIR.mkdir(parents=True, exist_ok=True)
            self._path = _LOG_DIR / moment.strftime("eeg_metrics_%Y%m%d_%H%M%S.csv")
            self._file = self._path.open("w", encoding="utf-8", newline="")
            self._file.write(_HEADER + "\n")
        return self._file
