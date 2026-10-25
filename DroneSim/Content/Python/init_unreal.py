"""에디터 시작 시(UE가 자동 실행) 누락된 대용량 에셋을 GitHub Release에서 내려받는다.

대용량 위성 텍스처는 git에 포함하지 않으므로, 프로젝트를 열 때 이 스크립트가 받아온다.
다운로드는 게임 스레드에서 진행률 창(ScopedSlowTask, 취소 가능)을 띄우며 청크 단위로
수행한다. 모든 Unreal API 호출이 게임 스레드에서 일어나므로 "Attempted to access Unreal
API from outside the main game thread" 문제가 없다. 임시 파일(.part)로 받아 크기·SHA256을
검증한 뒤 최종 경로로 옮기고, 애셋 레지스트리를 재스캔한다.
"""
from __future__ import annotations

import hashlib
import os
import urllib.request
from typing import Any, Final

import unreal

_REPO: Final[str] = "hkim-luca/eeg-for-drone"
_TAG: Final[str] = "large-assets"
_LOG_TAG: Final[str] = "[large-assets]"
_CHUNK: Final[int] = 1 << 20
_MB: Final[int] = 1 << 20

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
    return os.path.normpath(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))


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


def _package_paths(assets: list[dict[str, Any]]) -> list[str]:
    content_dir = os.path.join(_project_dir(), "Content")
    paths: set[str] = set()
    for asset in assets:
        rel = os.path.relpath(os.path.dirname(_abs_dest(asset)), content_dir)
        paths.add("/Game/" + rel.replace(os.sep, "/"))
    return sorted(paths)


class _Cancelled(Exception):
    pass


def _download_one(asset: dict[str, Any]) -> None:
    """게임 스레드에서 호출: 진행률 창을 띄우고 청크 단위로 받아 검증 후 최종 경로로 옮긴다."""
    dest = _abs_dest(asset)
    size = int(asset["size"])
    expected_sha = str(asset["sha256"]).lower()
    name = str(asset["name"])
    os.makedirs(os.path.dirname(dest), exist_ok=True)
    part = dest + ".part"
    url = f"https://github.com/{_REPO}/releases/download/{_TAG}/{name}"

    total_mb = max(size // _MB, 1)
    with unreal.ScopedSlowTask(float(size), f"{name} 다운로드 중...") as task:
        task.make_dialog(True)  # 취소 버튼 포함
        with urllib.request.urlopen(url) as response, open(part, "wb") as out:
            downloaded = 0
            while True:
                if task.should_cancel():
                    raise _Cancelled("사용자가 다운로드를 취소했습니다")
                chunk = response.read(_CHUNK)
                if not chunk:
                    break
                out.write(chunk)
                downloaded += len(chunk)
                task.enter_progress_frame(float(len(chunk)), f"{name}  {downloaded // _MB} / {total_mb} MB")

    got = os.path.getsize(part)
    if got != size:
        os.remove(part)
        raise RuntimeError(f"{name} 크기 불일치: 기대 {size} / 실제 {got}")
    if _sha256(part).lower() != expected_sha:
        os.remove(part)
        raise RuntimeError(f"{name} SHA256 불일치")
    os.replace(part, dest)


def _run_download() -> None:
    """게임 스레드(첫 틱)에서 한 번 실행."""
    missing = [asset for asset in _ASSETS if not _present(asset)]
    if not missing:
        return

    names = ", ".join(str(a["name"]) for a in missing)
    unreal.log_warning(f"{_LOG_TAG} 누락 에셋 다운로드 시작: {names}")
    try:
        for asset in missing:
            _download_one(asset)
    except _Cancelled as cancel:
        unreal.log_warning(f"{_LOG_TAG} {cancel}")
        return
    except Exception as exc:  # noqa: BLE001
        unreal.log_error(f"{_LOG_TAG} 다운로드 실패: {exc}")
        unreal.EditorDialog.show_message("대용량 에셋 다운로드 실패", str(exc), unreal.AppMsgType.OK)
        return

    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.scan_paths_synchronous(_package_paths(missing), force_rescan=True)
    unreal.log_warning(f"{_LOG_TAG} 다운로드 완료 및 애셋 재스캔: {names}")
    unreal.EditorDialog.show_message(
        "대용량 에셋 준비 완료",
        f"위성 텍스처 다운로드가 끝났습니다.\n({names})\n\n"
        "해당 텍스처를 쓰는 맵이 이미 열려 있었다면 다시 열어 반영하세요.",
        unreal.AppMsgType.OK,
    )


def _on_first_tick(_delta_seconds: float) -> None:
    # 에디터가 뜬 뒤 첫 틱에서 한 번만 실행하고 콜백 해제(이후 블로킹 다운로드는 진행률 창이 담당)
    handle = _state.get("handle")
    if handle is not None:
        unreal.unregister_slate_post_tick_callback(handle)
        _state["handle"] = None
    _run_download()


def _main() -> None:
    if not [asset for asset in _ASSETS if not _present(asset)]:
        return
    # 다운로드는 게임 스레드에서 해야 하지만, 에디터 UI가 뜬 뒤 진행률 창을 보여주려고
    # 시작 스크립트 단계가 아니라 첫 틱으로 미룬다.
    _state["handle"] = unreal.register_slate_post_tick_callback(_on_first_tick)


_main()
