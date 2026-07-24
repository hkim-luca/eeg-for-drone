"""NeuroSky MindWave(ThinkGear) 시리얼 패킷 파서.

MindWave는 날바이트가 아니라 다음 프레임 구조로 전송한다.
    0xAA 0xAA  PLENGTH  payload(PLENGTH bytes)  CHKSUM
payload 내부의 code 별 의미:
    0x02 신호품질(0=양호, 200=미접촉) / 0x04 집중도 / 0x05 명상도
    0x80 RAW 파형값(16bit signed, 512Hz) / 0x83 ASIC 대역파워(8밴드 x 3byte)
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Final

import serial

SYNC: Final[int] = 0xAA
EXCODE: Final[int] = 0x55
CODE_POOR_SIGNAL: Final[int] = 0x02
CODE_ATTENTION: Final[int] = 0x04
CODE_MEDITATION: Final[int] = 0x05
CODE_RAW_WAVE: Final[int] = 0x80
CODE_ASIC_POWER: Final[int] = 0x83
MAX_PLENGTH: Final[int] = 169
RAW_SAMPLE_RATE_HZ: Final[int] = 512

BAND_NAMES: Final[tuple[str, ...]] = (
    "delta",
    "theta",
    "lowAlpha",
    "highAlpha",
    "lowBeta",
    "highBeta",
    "lowGamma",
    "midGamma",
)


@dataclass
class ThinkGearPacket:
    """한 패킷에서 파싱된 값. 없는 스칼라는 -1, 없는 컨테이너는 비어 있음."""

    raw: list[int] = field(default_factory=list)
    poor_signal: int = -1
    attention: int = -1
    meditation: int = -1
    bands: dict[str, int] = field(default_factory=dict)


def _read_byte(ser: serial.Serial) -> int:
    chunk: bytes = ser.read(1)
    if len(chunk) == 0:
        raise TimeoutError("serial read timeout")
    return chunk[0]


def _to_int16(data: bytes) -> int:
    return int.from_bytes(data, "big", signed=True)


def _parse_bands(data: bytes) -> dict[str, int]:
    return {
        name: int.from_bytes(data[k * 3 : k * 3 + 3], "big")
        for k, name in enumerate(BAND_NAMES)
    }


def _sync(ser: serial.Serial) -> None:
    """0xAA 0xAA 동기 시퀀스를 만날 때까지 스트림을 소비한다."""
    while True:
        if _read_byte(ser) != SYNC:
            continue
        if _read_byte(ser) == SYNC:
            return


def _parse_payload(payload: bytes) -> ThinkGearPacket:
    pkt = ThinkGearPacket()
    i: int = 0
    n: int = len(payload)
    while i < n:
        code: int = payload[i]
        if code == EXCODE:
            i += 1
            continue
        if code < CODE_RAW_WAVE:
            if i + 1 >= n:
                break
            value: int = payload[i + 1]
            if code == CODE_POOR_SIGNAL:
                pkt.poor_signal = value
            elif code == CODE_ATTENTION:
                pkt.attention = value
            elif code == CODE_MEDITATION:
                pkt.meditation = value
            i += 2
        else:
            if i + 1 >= n:
                break
            vlength: int = payload[i + 1]
            start: int = i + 2
            end: int = start + vlength
            if end > n:
                break
            data: bytes = payload[start:end]
            if code == CODE_RAW_WAVE and vlength == 2:
                pkt.raw.append(_to_int16(data))
            elif code == CODE_ASIC_POWER and vlength == 24:
                pkt.bands = _parse_bands(data)
            i = end
    return pkt


def extract_packets(buf: bytearray) -> list[ThinkGearPacket]:
    """버퍼에서 완전한 패킷을 모두 꺼내 파싱하고, 소비한 바이트를 buf에서 제거한다.

    미완성 패킷·앞쪽 잡음은 버퍼에 남겨 다음 호출에서 이어 처리한다.
    바이트 단위 시리얼 읽기를 피해 GIL 점유를 줄이는 것이 목적이다.
    """
    packets: list[ThinkGearPacket] = []
    i: int = 0
    n: int = len(buf)
    while True:
        while i + 1 < n and not (buf[i] == SYNC and buf[i + 1] == SYNC):
            i += 1
        if i + 2 >= n:
            break
        plength: int = buf[i + 2]
        if plength > MAX_PLENGTH:
            i += 1
            continue
        checksum_idx: int = i + 3 + plength
        if checksum_idx >= n:
            break
        payload: bytes = bytes(buf[i + 3 : checksum_idx])
        if ((~(sum(payload) & 0xFF)) & 0xFF) == buf[checksum_idx]:
            packets.append(_parse_payload(payload))
        i = checksum_idx + 1
    del buf[:i]
    return packets


def read_packet(ser: serial.Serial) -> ThinkGearPacket:
    """한 개의 유효 패킷을 읽어 파싱한다.

    길이 초과·체크섬 불일치 등 무효 패킷은 빈 ThinkGearPacket으로 반환한다.
    데이터가 오지 않으면 TimeoutError를 발생시킨다.
    """
    _sync(ser)
    plength: int = _read_byte(ser)
    if plength > MAX_PLENGTH:
        return ThinkGearPacket()
    payload: bytes = ser.read(plength)
    if len(payload) < plength:
        return ThinkGearPacket()
    checksum: int = _read_byte(ser)
    if ((~(sum(payload) & 0xFF)) & 0xFF) != checksum:
        return ThinkGearPacket()
    return _parse_payload(bytes(payload))
