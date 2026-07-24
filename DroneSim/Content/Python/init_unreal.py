"""에디터 시작 시(UE가 자동 실행) 누락된 대용량 에셋을 백그라운드로 내려받는다.

SHA256 검증을 통과한 파일만 다운로드 스크립트가 최종 경로로 옮기므로, 완료
판단은 크기만으로 한다.
"""
from __future__ import annotations

import json
import os
import subprocess
from typing import Any, Final, Optional

import unreal

_MANIFEST_REL: Final[str] = "scripts/large_assets.json"
_DOWNLOAD_SCRIPT_REL: Final[str] = "scripts/download_large_assets.ps1"
_POLL_INTERVAL_TICKS: Final[int] = 120
_LOG_TAG: Final[str] = "[large-assets]"

_state: dict[str, Any] = {}


def _repo_root() -> str:
    project_dir = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
    return os.path.dirname(os.path.normpath(project_dir))


def _missing_assets(repo_root: str) -> list[dict[str, Any]]:
    manifest_path = os.path.join(repo_root, _MANIFEST_REL)
    with open(manifest_path, "r", encoding="utf-8") as handle:
        manifest = json.load(handle)

    missing: list[dict[str, Any]] = []
    for asset in manifest["assets"]:
        dest = os.path.join(repo_root, asset["dest"])
        size = int(asset["size"])
        if not (os.path.exists(dest) and os.path.getsize(dest) == size):
            missing.append({"name": str(asset["name"]), "dest": dest, "size": size})
    return missing


def _launch_download(repo_root: str) -> Optional[subprocess.Popen[bytes]]:
    script = os.path.join(repo_root, _DOWNLOAD_SCRIPT_REL)
    try:
        return subprocess.Popen(
            ["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", script],
            cwd=repo_root,
            creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0),
        )
    except OSError as exc:
        unreal.log_error(f"{_LOG_TAG} 다운로드 실행 실패: {exc}")
        return None


def _package_paths(dests: list[str]) -> list[str]:
    content_dir = os.path.normpath(
        unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir())
    )
    paths: set[str] = set()
    for dest in dests:
        rel = os.path.relpath(os.path.dirname(os.path.normpath(dest)), content_dir)
        paths.add("/Game/" + rel.replace(os.sep, "/"))
    return sorted(paths)


def _finish(missing: list[dict[str, Any]]) -> None:
    package_paths = _package_paths([m["dest"] for m in missing])
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.scan_paths_synchronous(package_paths, force_rescan=True)

    names = ", ".join(m["name"] for m in missing)
    unreal.log_warning(f"{_LOG_TAG} 다운로드 완료 및 애셋 재스캔: {names}")
    unreal.EditorDialog.show_message(
        "대용량 에셋 준비 완료",
        f"위성 텍스처 다운로드가 끝났습니다.\n({names})\n\n"
        "해당 텍스처를 쓰는 맵이 이미 열려 있었다면 다시 열어 반영하세요.",
        unreal.AppMsgType.OK,
    )


def _stop() -> None:
    handle = _state.get("handle")
    if handle is not None:
        unreal.unregister_slate_post_tick_callback(handle)
        _state["handle"] = None


def _on_tick(_delta_seconds: float) -> None:
    _state["ticks"] = int(_state.get("ticks", 0)) + 1
    if int(_state["ticks"]) % _POLL_INTERVAL_TICKS != 0:
        return

    missing = _state["missing"]
    remaining = [
        m for m in missing
        if not (os.path.exists(m["dest"]) and os.path.getsize(m["dest"]) == m["size"])
    ]
    if not remaining:
        _stop()
        _finish(missing)
        return

    proc: Optional[subprocess.Popen[bytes]] = _state.get("proc")
    if proc is not None and proc.poll() is not None:
        unreal.log_error(
            f"{_LOG_TAG} 다운로드 프로세스가 종료됐지만 파일이 완성되지 않았습니다. "
            f"scripts/download_large_assets.ps1 을 수동 실행하세요."
        )
        _stop()


def _main() -> None:
    repo_root = _repo_root()
    try:
        missing = _missing_assets(repo_root)
    except (OSError, ValueError, KeyError) as exc:
        unreal.log_warning(f"{_LOG_TAG} 매니페스트 확인 실패: {exc}")
        return

    if not missing:
        return

    names = ", ".join(m["name"] for m in missing)
    unreal.log_warning(f"{_LOG_TAG} 누락 에셋 백그라운드 다운로드 시작: {names}")
    proc = _launch_download(repo_root)
    if proc is None:
        return

    _state.update(missing=missing, proc=proc, ticks=0)
    _state["handle"] = unreal.register_slate_post_tick_callback(_on_tick)


_main()
