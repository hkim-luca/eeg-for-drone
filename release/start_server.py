#!/usr/bin/env python3
"""EEG 서버 실행 (Ubuntu 24.04 / Windows 공용).

처음 한 번은 eeg-server/.venv 가상환경을 만들고 패키지를 설치한다.
    python3 start_server.py
"""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
from pathlib import Path

MIN_PYTHON = (3, 10)
IS_WINDOWS = os.name == "nt"

SERVER_DIR = Path(__file__).resolve().parent / "eeg-server"
VENV_DIR = SERVER_DIR / ".venv"
READY_MARK = VENV_DIR / ".ready"
VENV_PYTHON = VENV_DIR / ("Scripts/python.exe" if IS_WINDOWS else "bin/python")


def fail(message: str) -> int:
    print(message, file=sys.stderr)
    if IS_WINDOWS:
        input("(Enter로 종료)")
    return 1


def setup_venv() -> int:
    print("처음 실행이라 가상환경을 만들고 패키지를 설치합니다. 몇 분 걸립니다...")
    if VENV_DIR.exists():
        shutil.rmtree(VENV_DIR)

    if subprocess.run([sys.executable, "-m", "venv", str(VENV_DIR)]).returncode != 0 or not VENV_PYTHON.exists():
        shutil.rmtree(VENV_DIR, ignore_errors=True)
        hint = "" if IS_WINDOWS else "\nUbuntu에서는 먼저 설치하세요: sudo apt install python3-venv"
        return fail("가상환경을 만들지 못했습니다." + hint)

    pip = [str(VENV_PYTHON), "-m", "pip", "install", "--disable-pip-version-check", "-r", "requirements-server.txt"]
    if subprocess.run(pip, cwd=SERVER_DIR).returncode != 0:
        return fail("패키지 설치에 실패했습니다. 인터넷 연결을 확인한 뒤 다시 실행하세요.")

    READY_MARK.touch()
    return 0


def main() -> int:
    if sys.version_info < MIN_PYTHON:
        return fail(f"Python {MIN_PYTHON[0]}.{MIN_PYTHON[1]} 이상이 필요합니다 (현재 {sys.version.split()[0]}).")
    if not SERVER_DIR.is_dir():
        return fail(f"서버 폴더를 찾을 수 없습니다: {SERVER_DIR}")

    if not READY_MARK.exists():
        code = setup_venv()
        if code != 0:
            return code

    os.chdir(SERVER_DIR)
    command = [str(VENV_PYTHON), "-m", "eeg_server"]
    if not IS_WINDOWS:
        # 프로세스를 서버로 교체: Ctrl+C·종료 신호가 서버에 바로 전달된다.
        os.execv(command[0], command)

    try:
        subprocess.run(command)
    except KeyboardInterrupt:
        pass
    input("서버가 종료됐습니다. (Enter로 닫기)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
