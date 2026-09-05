# Third-party Environment Assets

## Kenney Nature Kit 2.1

- 공식 페이지: https://kenney.nl/assets/nature-kit
- 원본 다운로드: 공식 Kenney 배포 ZIP
- License: CC0 1.0 Universal
- 저작자 표시: 선택 사항이지만 프로젝트 기록을 위해 Kenney 출처를 유지
- 런타임 네트워크 의존성: 없음
- ZIP SHA-256: `FA7974A0D342BFE63C38664BA9F8EC1A4AAB8EA25F099BDC56870E33588C4D9D`
- 프로젝트 원본 보관: `DroneSim/SourceData/Environment/ThirdParty/Kenney/NatureKit`

프로젝트에 import한 Static Mesh:

- `DroneSim/Content/Environment/Vegetation/Shrubs/SM_ENV_CC0_Shrub_A`
- `DroneSim/Content/Environment/Vegetation/Shrubs/SM_ENV_CC0_Shrub_B`
- `DroneSim/Content/Environment/Vegetation/Grass/SM_ENV_CC0_GrassClump_A`
- `DroneSim/Content/Environment/Vegetation/Grass/SM_ENV_CC0_GrassClump_B`
- `DroneSim/Content/Environment/Vegetation/Riparian/SM_ENV_CC0_RiparianTallPlant_A`
- `DroneSim/Content/Environment/Vegetation/Riparian/SM_ENV_CC0_RiparianTallPlant_B`
- `DroneSim/Content/Environment/Rocks/SM_ENV_CC0_Rock_Large_A`
- `DroneSim/Content/Environment/Rocks/SM_ENV_CC0_Rock_Large_B`
- `DroneSim/Content/Environment/Rocks/SM_ENV_CC0_Rock_Small_A`
- `DroneSim/Content/Environment/Rocks/SM_ENV_CC0_Rock_Small_B`

`RiparianTallPlant` 두 Asset은 실제 reed 종을 확인한 자산이 아니다. 수변 초본 PCG 규칙과 성능을 시험하기 위한 generic low-poly proxy로만 취급한다.

Nature Kit은 stylized low-poly 자산이므로 기존 사실적 Aleppo Pine/Black Alder와 시각적으로 어울리는지 Editor에서 승인하기 전에는 production PCG Graph에 연결하지 않는다.

## Water Materials by tharlevfx

- Asset title: `Water Materials`
- Creator: `tharlevfx`
- Fab listing: https://www.fab.com/listings/063155ea-d9d2-4f29-b09f-33270b0bc861
- 사용자 제공 short link: https://fab.com/s/0dff090dba78
- License: Creative Commons Attribution 4.0 International (`CC BY 4.0`)
- License text: https://creativecommons.org/licenses/by/4.0/
- 프로젝트 import 경로: `DroneSim/Content/WaterMaterials`
- 과거 시험 parent: `DroneSim/Content/WaterMaterials/Materials/M_River_Cheaper` (원본 보존)
- 현재 사용 텍스처: `/Game/WaterMaterials/Textures/T_Water_Normal_Subtle`
- 현재 프로젝트 water master: `/Game/Environment/River/Materials/M_ENV_RiverSurface_LongitudinalPhase`
- 현재 프로젝트 Material Instance 폴더: `/Game/Environment/River/Materials/Instances/Flow/`
- 런타임 네트워크 의존성: 없음

현재 Level은 프로젝트 소유 water master와 flow Material Instance를 사용하며, 위 원본 normal texture를 참조한다. 과거 시험용 `MI_ENV_RiverSurface_WaterMaterials_Flow`는 미사용 참조를 확인한 뒤 정리했다. 원본 WaterMaterials 라이브러리는 수정하지 않았다. 배포 시 이 문서 또는 동등한 Credits 문서에 저작자, Asset 제목, Fab 링크와 기존 `CC BY 4.0` 라이선스 기록을 유지한다. 이 정리는 기존 출처 기록을 보존한 것이며 라이선스를 새로 심사한 작업은 아니다.
