"""에디터 시작 시(UE가 자동 실행) 누락된 대용량 에셋을 GitHub Release에서 내려받는다.

대용량 위성 텍스처는 git에 포함하지 않으므로, 프로젝트를 열 때 이 스크립트가
백그라운드 스레드로 내려받아(에디터 시작 논블로킹) 완료되면 애셋 레지스트리를
재스캔한다. 임시 파일(.part)로 받아 크기·SHA256을 검증한 뒤 최종 경로로 옮긴다.
"""
from __future__ import annotations

import hashlib
import os
import threading
import urllib.request
from typing import Any, Final

import unreal

_REPO: Final[str] = "hkim-luca/eeg-for-drone"
_TAG: Final[str] = "large-assets"
_LOG_TAG: Final[str] = "[large-assets]"
_POLL_INTERVAL_TICKS: Final[int] = 120
_CHUNK: Final[int] = 1 << 20

# dest 는 프로젝트 폴더(Content 상위) 기준 상대 경로.
_ASSETS: Final[list[dict[str, Any]]] = [
    {
        "name": "daejeon_satellite_z16.uasset",
        "dest": "Content/Maps/daejeon_satellite_z16.uasset",
        "size": 2058129989,
        "sha256": "090b5b7305bf35771ccf059630aa28df72050aedddd51e936da0b6762ce85d12",
    },
]

_state: dict[str, Any] = {}


def _project_dir() -> str:
    return os.path.normpath(
        unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
    )


def _abs_dest(asset: dict[str, Any]) -> str:
    return os.path.join(_project_dir(), str(asset["dest"]))


def _present(asset: dict[str, Any]) -> bool:
    dest = _abs_dest(asset)
    return os.path.exists(dest) and os.path.getsize(dest) == int(asset["size"])


def _sha256(path: str) -> str:
    digest = hashlib.sha256()
    with open(path, "rb") as handle:
        for chunk in iter(lambda: handle.read(_CHUNK), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _download(asset: dict[str, Any]) -> None:
    dest = _abs_dest(asset)
    size = int(asset["size"])
    expected_sha = str(asset["sha256"]).lower()
    os.makedirs(os.path.dirname(dest), exist_ok=True)

    part = dest + ".part"
    url = f"https://github.com/{_REPO}/releases/download/{_TAG}/{asset['name']}"
    urllib.request.urlretrieve(url, part)

    got = os.path.getsize(part)
    if got != size:
        os.remove(part)
        raise RuntimeError(f"{asset['name']} 크기 불일치: 기대 {size} / 실제 {got}")
    actual_sha = _sha256(part).lower()
    if actual_sha != expected_sha:
        os.remove(part)
        raise RuntimeError(f"{asset['name']} SHA256 불일치: 기대 {expected_sha} / 실제 {actual_sha}")
    os.replace(part, dest)


def _package_paths(assets: list[dict[str, Any]]) -> list[str]:
    content_dir = os.path.join(_project_dir(), "Content")
    paths: set[str] = set()
    for asset in assets:
        rel = os.path.relpath(os.path.dirname(_abs_dest(asset)), content_dir)
        paths.add("/Game/" + rel.replace(os.sep, "/"))
    return sorted(paths)


def _finish(assets: list[dict[str, Any]]) -> None:
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.scan_paths_synchronous(_package_paths(assets), force_rescan=True)

    names = ", ".join(str(a["name"]) for a in assets)
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
    if not _state.get("done"):
        return

    _stop()
    error = _state.get("error")
    if error:
        unreal.log_error(f"{_LOG_TAG} 다운로드 실패: {error}")
        return
    _finish(_state["assets"])


def _worker(assets: list[dict[str, Any]]) -> None:
    try:
        for asset in assets:
            _download(asset)
        _state["error"] = None
    except Exception as exc:  # 스레드 예외를 게임 스레드로 전달
        _state["error"] = str(exc)
    _state["done"] = True


def _main() -> None:
    missing = [asset for asset in _ASSETS if not _present(asset)]
    if not missing:
        return

    names = ", ".join(str(a["name"]) for a in missing)
    unreal.log_warning(f"{_LOG_TAG} 누락 에셋 백그라운드 다운로드 시작: {names}")

    _state.update(done=False, error=None, assets=missing, ticks=0)
    threading.Thread(target=_worker, args=(missing,), daemon=True).start()
    _state["handle"] = unreal.register_slate_post_tick_callback(_on_tick)


_main()
