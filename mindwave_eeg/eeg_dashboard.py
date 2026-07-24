"""MindWave/TGAM EEG 실시간 통합 대시보드 (단일 스레드).

블루투스 데이터 포트(COM5)로 들어오는 ThinkGear 스트림을 파싱해
  - 상단: raw 파형(512Hz, 시간축 스크롤)
  - 중단: 8개 대역파워 막대
  - 하단: attention/meditation eSense 시간축 이중 라인 차트
을 한 창에 표시한다.

시리얼 읽기는 애니메이션 콜백(메인 스레드) 안에서 timeout=0 논블로킹으로만
수행하므로, GUI 이벤트 루프를 붙잡지 않는다(창 드래그가 항상 가능).

실행:  python eeg_dashboard.py
필요:  pip install pyserial numpy matplotlib
"""

from __future__ import annotations

import importlib.util
import time
from collections import deque
from dataclasses import dataclass, field
from typing import Final

import matplotlib

if importlib.util.find_spec("PyQt5") is not None:
    matplotlib.use("QtAgg")  # Tk보다 실시간 애니메이션 응답성이 좋음
else:
    print("PyQt5 미설치 — 기본 백엔드 사용. 'pip install PyQt5' 권장.")

import matplotlib.pyplot as plt
import numpy as np
import numpy.typing as npt
import serial
from matplotlib.animation import FuncAnimation
from matplotlib.artist import Artist

from thinkgear import BAND_NAMES, RAW_SAMPLE_RATE_HZ, extract_packets

PORT: Final[str] = "COM5"
BAUD: Final[int] = 57600
RAW_WINDOW: Final[int] = RAW_SAMPLE_RATE_HZ * 2  # 2초 분량
NO_CONTACT: Final[int] = 200
RAW_YLIM: Final[int] = 2048  # raw 파형 고정 Y범위(±) — blit과 양립하도록 고정
ESENSE_HISTORY: Final[int] = 120  # attention/meditation 이력 길이(초, 1Hz 갱신)
PROFILE: Final[bool] = False  # True면 콜백 소요시간(ms) 최대치를 2초마다 출력


@dataclass
class EEGState:
    raw: deque[int] = field(
        default_factory=lambda: deque([0] * RAW_WINDOW, maxlen=RAW_WINDOW)
    )
    bands: dict[str, int] = field(
        default_factory=lambda: {name: 0 for name in BAND_NAMES}
    )
    attention: int = 0
    meditation: int = 0
    poor_signal: int = NO_CONTACT
    att_history: deque[int] = field(
        default_factory=lambda: deque([0] * ESENSE_HISTORY, maxlen=ESENSE_HISTORY)
    )
    med_history: deque[int] = field(
        default_factory=lambda: deque([0] * ESENSE_HISTORY, maxlen=ESENSE_HISTORY)
    )


def _quality_text(poor: int) -> str:
    if poor <= 0:
        return "signal: GOOD"
    if poor >= NO_CONTACT:
        return "signal: NO CONTACT"
    return f"signal: noisy ({poor})"


def main() -> None:
    try:
        ser = serial.Serial(PORT, BAUD, timeout=0)  # timeout=0 => 논블로킹 읽기
    except serial.SerialException as exc:
        print(f"{PORT} 연결 실패: {exc}")
        return
    print(f"{PORT} 연결 성공. 창을 닫으면 종료됩니다.")

    state = EEGState()
    buf = bytearray()
    prof: dict[str, float] = {"max_ms": 0.0, "last_report": time.perf_counter()}

    fig, (ax_raw, ax_band, ax_esense) = plt.subplots(
        3, 1, figsize=(10, 9), constrained_layout=True
    )
    fig.suptitle("EEG Dashboard (COM5)", fontweight="bold")

    x: npt.NDArray[np.int32] = np.arange(RAW_WINDOW, dtype=np.int32)
    (raw_line,) = ax_raw.plot(x, np.zeros(RAW_WINDOW), color="C0", lw=0.8)
    ax_raw.set_title(f"Raw EEG waveform ({RAW_SAMPLE_RATE_HZ} Hz)")
    ax_raw.set_ylabel("Amplitude (raw)")
    ax_raw.set_xlim(0, RAW_WINDOW)
    ax_raw.set_ylim(-RAW_YLIM, RAW_YLIM)
    ax_raw.grid(True, alpha=0.3)
    quality_txt = ax_raw.text(
        0.99, 0.95, "", transform=ax_raw.transAxes,
        ha="right", va="top", fontsize=10, fontweight="bold",
    )

    bars = ax_band.bar(range(len(BAND_NAMES)), [1] * len(BAND_NAMES), color="C1")
    ax_band.set_xticks(range(len(BAND_NAMES)))
    ax_band.set_xticklabels(BAND_NAMES, rotation=30, ha="right", fontsize=8)
    ax_band.set_yscale("log")
    ax_band.set_ylim(1, 1e7)
    ax_band.set_ylabel("Band power")
    ax_band.set_title("EEG band power")
    ax_band.grid(True, axis="y", alpha=0.3)

    xe: npt.NDArray[np.int32] = np.arange(ESENSE_HISTORY, dtype=np.int32)
    (att_line,) = ax_esense.plot(
        xe, np.zeros(ESENSE_HISTORY), color="C3", lw=1.5, label="Attention"
    )
    (med_line,) = ax_esense.plot(
        xe, np.zeros(ESENSE_HISTORY), color="C2", lw=1.5, label="Meditation"
    )
    ax_esense.set_title("Attention / Meditation (device eSense)")
    ax_esense.set_xlabel("time (s, scrolling)")
    ax_esense.set_ylabel("eSense (0-100)")
    ax_esense.set_xlim(0, ESENSE_HISTORY)
    ax_esense.set_ylim(0, 100)
    ax_esense.grid(True, alpha=0.3)
    ax_esense.legend(loc="upper left", fontsize=8)
    esense_txt = ax_esense.text(
        0.99, 0.95, "", transform=ax_esense.transAxes,
        ha="right", va="top", fontsize=11, fontweight="bold",
    )

    def update(_frame: int) -> tuple[Artist, ...]:
        t0: float = time.perf_counter()
        chunk: bytes = ser.read(8192)  # 논블로킹: 지금 버퍼에 있는 것만 즉시 반환
        if chunk:
            buf.extend(chunk)
            for pkt in extract_packets(buf):
                state.raw.extend(pkt.raw)
                if pkt.poor_signal >= 0:
                    state.poor_signal = pkt.poor_signal
                if pkt.meditation >= 0:
                    state.meditation = pkt.meditation
                if pkt.attention >= 0:  # 1Hz eSense 갱신 → 이력에 함께 기록
                    state.attention = pkt.attention
                    state.att_history.append(pkt.attention)
                    state.med_history.append(state.meditation)
                if pkt.bands:
                    state.bands = pkt.bands

        raw_arr: npt.NDArray[np.int32] = np.fromiter(
            state.raw, dtype=np.int32, count=len(state.raw)
        )
        raw_line.set_ydata(raw_arr)
        quality_txt.set_text(_quality_text(state.poor_signal))
        for bar, name in zip(bars, BAND_NAMES):
            bar.set_height(max(1, state.bands.get(name, 0)))
        att_line.set_ydata(
            np.fromiter(state.att_history, dtype=np.int32, count=ESENSE_HISTORY)
        )
        med_line.set_ydata(
            np.fromiter(state.med_history, dtype=np.int32, count=ESENSE_HISTORY)
        )
        esense_txt.set_text(
            f"Att {state.attention:>3}   Med {state.meditation:>3}"
        )

        if PROFILE:
            elapsed_ms: float = (time.perf_counter() - t0) * 1000.0
            prof["max_ms"] = max(prof["max_ms"], elapsed_ms)
            now: float = time.perf_counter()
            if now - prof["last_report"] >= 2.0:
                print(f"[profile] update() 최대 {prof['max_ms']:.1f} ms / 2s")
                prof["max_ms"] = 0.0
                prof["last_report"] = now

        return (raw_line, quality_txt, att_line, med_line, esense_txt, *bars)

    _ani = FuncAnimation(fig, update, interval=50, blit=True, cache_frame_data=False)
    try:
        plt.show()
    finally:
        ser.close()
        print("연결을 종료했습니다.")


if __name__ == "__main__":
    main()
