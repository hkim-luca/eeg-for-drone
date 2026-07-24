# eeg-for-drone

EEG 기반 드론 제어 및 시뮬레이션 프로젝트.

## 대용량 에셋 내려받기

일부 위성 텍스처 에셋은 크기(1.92GB)가 커서 git 저장소에 포함하지 않고
[GitHub Release](https://github.com/hkim-luca/eeg-for-drone/releases/tag/large-assets)
로 배포한다. 클론 후 아래 스크립트로 내려받아 올바른 경로에 배치한다.

```powershell
pwsh scripts/download_large_assets.ps1
```

| 에셋 | 경로 | 크기 |
|------|------|------|
| `daejeon_satellite_z16.uasset` | `DroneSim/Content/Maps/` | 1.92 GB |
