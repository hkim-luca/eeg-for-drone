"""Stand-in DroneSim client for end-to-end testing of the EEG server.

Speaks the exact wire protocol of ``UEegRunnerComponent``: streams simulated
32-channel frames at 10 Hz as length-prefixed protobuf, receives the inferred
actions and confirms them with control acks. Run the server first, then::

    python tools/fake_dronesim.py --seconds 10
"""

from __future__ import annotations

import argparse
import math
import random
import socket
import struct
import sys
import time
from pathlib import Path
from typing import Final

sys.path.insert(0, str(Path(__file__).parent.parent))

from eeg_server import config
from eeg_server import eeg_link_pb2 as pb
from eeg_server.protocol import frame_payload, now_ms

_SAMPLES_PER_FRAME: Final[int] = 25
_FRAME_INTERVAL_S: Final[float] = _SAMPLES_PER_FRAME / config.SAMPLE_RATE_HZ
_LENGTH_PREFIX: Final[struct.Struct] = struct.Struct("<I")

#: (action, seconds) demo script mirroring FEegSignalSimulator.
_DEMO_SCRIPT: Final[list[tuple[str, float]]] = [
    ("FORWARD", 4.0), ("STOP", 2.0), ("LEFT", 3.0), ("STOP", 2.0),
    ("RIGHT", 3.0), ("STOP", 2.0), ("BACKWARD", 3.0), ("STOP", 2.0),
]


def _make_frame_data(active_action: str, base_time_s: float, rng: random.Random) -> list[float]:
    boost_start = config.ACTION_GROUP_START.get(active_action, -1)
    data: list[float] = []
    for sample in range(_SAMPLES_PER_FRAME):
        time_s = base_time_s + sample / config.SAMPLE_RATE_HZ
        for channel in range(config.CHANNEL_COUNT):
            boosted = boost_start >= 0 and boost_start <= channel < boost_start + config.GROUP_CHANNEL_COUNT
            amplitude = 10.0 * (2.5 if boosted else 1.0)
            rhythm = 0.8 * math.sin(2 * math.pi * 10 * time_s + channel) + 0.4 * math.sin(
                2 * math.pi * 22 * time_s + channel * 2
            )
            data.append(amplitude * rhythm + rng.gauss(0.0, 4.0))
    return data


def _read_exact(sock: socket.socket, count: int) -> bytes:
    chunks = bytearray()
    while len(chunks) < count:
        chunk = sock.recv(count - len(chunks))
        if not chunk:
            raise ConnectionError("server closed the connection")
        chunks.extend(chunk)
    return bytes(chunks)


def _read_action(sock: socket.socket) -> pb.ActionResult:
    (length,) = _LENGTH_PREFIX.unpack(_read_exact(sock, _LENGTH_PREFIX.size))
    message = pb.ServerMessage.FromString(_read_exact(sock, length))
    return message.action


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=config.TCP_PORT)
    parser.add_argument("--seconds", type=float, default=10.0, help="how long to stream")
    args = parser.parse_args()

    rng = random.Random(1234)
    correct = 0
    total = 0

    with socket.create_connection((args.host, args.port), timeout=5.0) as sock:
        deadline = time.monotonic() + args.seconds
        seq = 0
        script_index = 0
        script_elapsed = 0.0

        while time.monotonic() < deadline:
            active_action = _DEMO_SCRIPT[script_index][0]
            message = pb.ClientMessage()
            message.eeg.seq = seq
            message.eeg.t_sent_ms = now_ms()
            message.eeg.rate = config.SAMPLE_RATE_HZ
            message.eeg.channels = config.CHANNEL_COUNT
            message.eeg.data.extend(_make_frame_data(active_action, seq * _FRAME_INTERVAL_S, rng))
            sock.sendall(frame_payload(message.SerializeToString()))

            action = _read_action(sock)
            total += 1
            correct += int(action.action == active_action)

            # confirm the control application, like the pawn tick does
            ack = pb.ClientMessage()
            ack.ack.action_seq = action.action_seq
            ack.ack.t_control_ms = now_ms()
            sock.sendall(frame_payload(ack.SerializeToString()))

            seq += 1
            script_elapsed += _FRAME_INTERVAL_S
            if script_elapsed >= _DEMO_SCRIPT[script_index][1]:
                script_elapsed = 0.0
                script_index = (script_index + 1) % len(_DEMO_SCRIPT)
            time.sleep(_FRAME_INTERVAL_S)

    print(f"fake_dronesim: {total} frames, inferred==scripted {correct}/{total} "
          f"({100.0 * correct / max(total, 1):.1f}%)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
