"""Server test harness: a stand-in DroneSim client for end-to-end checks.

Speaks the exact wire protocol of ``UEegRunnerComponent``: streams 22-channel EEG
frames at 10 Hz as length-prefixed JSON, receives the inferred actions and confirms
them with control acks. The signal is action-agnostic noise + alpha/beta rhythm (the
action is decided solely by the server's AI model, never encoded here). Run the server
first, then::

    python tools/fake_dronesim.py --seconds 10
"""

from __future__ import annotations

import argparse
import json
import math
import random
import socket
import struct
import sys
import time
from collections import Counter
from pathlib import Path
from typing import Final

sys.path.insert(0, str(Path(__file__).parent.parent))

from eeg_server import config
from eeg_server.protocol import frame_payload, now_ms

_SAMPLES_PER_FRAME: Final[int] = config.SAMPLES_PER_FRAME
_FRAME_INTERVAL_S: Final[float] = _SAMPLES_PER_FRAME / config.SAMPLE_RATE_HZ
_LENGTH_PREFIX: Final[struct.Struct] = struct.Struct("<I")


def _make_frame_data(base_time_s: float, rng: random.Random) -> list[float]:
    """One sample-major frame of plausible, action-agnostic EEG (alpha/beta + noise)."""
    data: list[float] = []
    for sample in range(_SAMPLES_PER_FRAME):
        time_s = base_time_s + sample / config.SAMPLE_RATE_HZ
        for channel in range(config.CHANNEL_COUNT):
            rhythm = 0.8 * math.sin(2 * math.pi * 10 * time_s + channel) + 0.4 * math.sin(
                2 * math.pi * 22 * time_s + channel * 2
            )
            data.append(10.0 * rhythm + rng.gauss(0.0, 4.0))
    return data


def _read_exact(sock: socket.socket, count: int) -> bytes:
    chunks = bytearray()
    while len(chunks) < count:
        chunk = sock.recv(count - len(chunks))
        if not chunk:
            raise ConnectionError("server closed the connection")
        chunks.extend(chunk)
    return bytes(chunks)


def _read_action(sock: socket.socket) -> dict:
    (length,) = _LENGTH_PREFIX.unpack(_read_exact(sock, _LENGTH_PREFIX.size))
    return json.loads(_read_exact(sock, length).decode("utf-8"))


def _send(sock: socket.socket, message: dict) -> None:
    sock.sendall(frame_payload(json.dumps(message, separators=(",", ":")).encode("utf-8")))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=config.TCP_PORT)
    parser.add_argument("--seconds", type=float, default=10.0, help="how long to stream")
    args = parser.parse_args()

    rng = random.Random(1234)
    actions: Counter[str] = Counter()

    with socket.create_connection((args.host, args.port), timeout=5.0) as sock:
        deadline = time.monotonic() + args.seconds
        seq = 0
        while time.monotonic() < deadline:
            _send(sock, {
                "type": "eeg",
                "seq": seq,
                "t_sent_ms": now_ms(),
                "rate": config.SAMPLE_RATE_HZ,
                "channels": config.CHANNEL_COUNT,
                "data": _make_frame_data(seq * _FRAME_INTERVAL_S, rng),
            })

            action = _read_action(sock)
            actions[action["action"]] += 1

            # confirm the control application, like the pawn tick does
            _send(sock, {"type": "ack", "action_seq": action["action_seq"], "t_control_ms": now_ms()})

            seq += 1
            time.sleep(_FRAME_INTERVAL_S)

    print(f"fake_dronesim: {sum(actions.values())} frames; inferred actions: {dict(actions)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
