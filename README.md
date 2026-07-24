# eeg-for-drone

EEG 기반 드론 제어 및 시뮬레이션 프로젝트.

## 대용량 에셋 내려받기

일부 위성 텍스처 에셋은 크기(1.92GB)가 커서 git 저장소에 포함하지 않고
[GitHub Release](https://github.com/hkim-luca/eeg-for-drone/releases/tag/large-assets)
로 배포한다.

**자동:** `DroneSim/DroneSim.uproject` 를 에디터에서 열면
`DroneSim/Content/Python/init_unreal.py` 가 누락 에셋을 백그라운드로 내려받는다
(에디터 시작을 막지 않음). 최초 1회 다운로드 완료 후, 해당 텍스처를 쓰는 맵이
이미 열려 있었다면 다시 열어 반영한다.

**수동(대안):** CI 등 에디터 없이 받아야 할 때는 아래 스크립트를 실행한다.

```powershell
powershell -File scripts/download_large_assets.ps1
```

대상 목록은 `scripts/large_assets.json` 매니페스트에서 관리한다.

| 에셋 | 경로 | 크기 |
|------|------|------|
| `daejeon_satellite_z16.uasset` | `DroneSim/Content/Maps/` | 1.92 GB |
