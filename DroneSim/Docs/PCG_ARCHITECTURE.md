# PCG Natural Environment Architecture

## Active rendering-only performance trial (2026-09-05)

The spatial architecture below remains unchanged. The working map and seven production PCG graphs now have synchronized rendering descriptors: tree WPO stops at 300 m; the four formerly 2200 m grass regions stop rendering at 1000 m (800 m start-cull metadata). No PCG regeneration, density/position change, material edit, water-mask edit, or river mesh change occurred. Tree shadow flags and existing Nanite assets are retained; native High scalability reduces shadow/GI/reflection/post-process quality in the local Editor user settings. This user preference is shared with other UE 5.7 projects and is not a packaged-build quality policy. River LOD was tested in memory and rejected because boundary preservation was not guaranteed. See PERFORMANCE_TRIAL.md for exact values, limits, evidence and the new pre-trial rollback point.

## Preserved spatial specification — pre-commit consolidation (2026-09-04)

The accepted V72 river geometry and vegetation placement are retained. Active assets now use canonical, version-free paths. A subsequent distance-policy correction limits very distant riparian-tree rendering; see PCG_PARAMETERS.md. The folded history below records former implementations and must not be treated as the current asset list.

- Engine: UE 5.7.4 CL 51494982. Working map: `/Game/Maps/Daejeon_PCG_Work`.
- Original `/Game/Maps/Daejeon`, Git-tracked configuration/C++ and supplied libraries are protected and unchanged.
- Landscape: 2017 × 2017 vertices, 1024 components, ±3,024,000 cm XY; World Partition is not used.
- River: 27 StaticMeshActors, not WaterBodyRiver. Current meshes: `/Game/Environment/River/Production/Meshes/SM_ENV_RiverSurface_Daejeon_X##_Y##`.
- Current water master: `/Game/Environment/River/Materials/M_ENV_RiverSurface_LongitudinalPhase`.
- Material instances: `River/Materials/Instances/Flow/`. Textures: `River/Textures/Production/Flow/`.
- The X02/Y00–X03/Y00 shared atlas remains separate. `MI_ENV_RiverSurface_Inlets` still inherits `MI_ENV_RiverSurface_EastNetwork`; their data and settings are not flattened or resampled.

| Production layer | Volumes | Current graphs under `/Game/Environment/PCG/Graphs/` |
|---|---:|---|
| Forest | 64 | `PCG_ENV_ForestHighSuitabilityProduction` (63), `PCG_ENV_ForestHighSuitabilityOpenWater` (X00/Y00) |
| Riparian trees | 64 | `PCG_ENV_RiparianTrees` |
| Lower layer | 27 | `PCG_ENV_RiparianRiverBankProduction` (22), `PCG_ENV_RiparianRiverBankDenseY00X02` (1), `PCG_ENV_RiparianUnifiedBank` (3), `PCG_ENV_RiparianUnifiedBankDenseX02` (1) |

The previous `OpenWaterPrototype` is a required regional variant, not unused work. Its behavior is unchanged under the new name. `PCG_EXCL_Manual_Central_01` is also retained: its `PCG_Exclude_Vegetation` tag is consumed by production selectors. Constraint volumes and temporarily empty production cells are not deleted just because they contain no instances.

The six current PCG masks, under `PCG/Data/Mask/`, are:

- `T_ENV_ForestSuitability_Daejeon` and `T_ENV_ForestOpenWaterExclusion_Daejeon`.
- `T_ENV_RiparianZones_Daejeon`.
- `T_ENV_RiverWater_Daejeon`, `T_ENV_RiverBankBand_Daejeon`, `T_ENV_WaterChannel_Daejeon`.

Shared subgraphs have the same responsibilities, with version suffixes removed. Water and bank masks derive from the accepted visible footprint; changes must remain synchronized to prevent grass on water. Runtime generation, geometry, density, material speed, collision and culling are not altered by consolidation.

The consolidation performance audit found that the former V55 distance pass omitted `PCG_ENV_RiparianTrees`. Its four spawner descriptors and 237 saved ISM components were aligned with the forest policy: start-cull metadata 350,000 cm, end-cull 500,000 cm, WPO disable 100,000 cm. The active 2026-09-05 trial lowers only that WPO threshold to 30,000 cm, consistently across tree categories. All 35,442 legacy riparian-tree transforms are preserved, and PCG was not regenerated. A gradual fade requires material support and is not assumed merely from the start-cull value. Original tree meshes/materials are unchanged.

57 unused PCG/river assets were retired and 87 active assets renamed through native Unreal asset operations. Imported Vegetation/Rocks/Fab/Megaplant/PN_GrassLibrary/WaterMaterials, Plugins and SourceData were not used as deletion candidates. 53 leftover redirectors were removed only after saving and checking referencers. The exact old/new mapping is recorded in the cleanup manifest.

Full verified backup (accepted map, all Environment content and documentation):
`C:/Users/kdyde/Documents/Codex/2026-08-08/ue-5-7-4-pcg-agents-2/work/precommit_cleanup_backup/`

Audit, rename/deletion plan and fresh-process validation are in the adjacent `work/precommit_*.json` files. Earlier one-shot generation scripts outside the project are retained as historical/recovery tools, not current reapply commands. Restore a matching map + Environment set together with the Editor closed; never mix old masks and a newer river mesh. No Git commit or staging was performed.

<details>
<summary>Historical implementation notes — pre-consolidation paths and superseded counts</summary>

## 1. 문서 상태

- Project: `DroneSim`
- Unreal Engine: `5.7.4` (`CL 51494982`)
- 원본 Level: `/Game/Maps/Daejeon`
- 작업 Level: `/Game/Maps/Daejeon_PCG_Work`
- 현재 단계: River Surface V70 hole fill/접지, 공유 PCG water/bank mask V70 동기화, 전역 수면 침범 제거 완료
- 생성 정책: Editor에서 생성 결과를 저장하고 생산 PCG Component는 `GenerateOnDemand`
- Last Updated: `2026-09-04`

이 문서는 현재 저장된 Asset과 UE 5.7.4 Editor Python 검사 결과만 기록한다. 과거 1km/2km Prototype 수치는 현재 생산 상태가 아니라 회귀 이력이며 `PCG_TEST_REPORT.md`에 필요한 항목만 남긴다.

> 2026-08-29 V40 정리 이후 Level의 강변 Lower Layer Prototype Actor는 `0`개다. 아래 문서의 V4–V37 Prototype 설명은 문제 원인과 의사결정의 역사 기록이며, 해당 자산이 현재 프로젝트에 존재한다는 뜻이 아니다. 현재 생산 경로는 Riparian Lower Layer V40/V49/V52 계열과 River Surface V23/V38/V48 Mesh + Longitudinal Phase V61 Material Instance다.

## 2. 확인된 프로젝트 구조

- C++ 프로젝트이며 Runtime module은 `DroneSim`이다.
- 안정 PCG Framework plugin을 사용한다.
- `ProceduralVegetationEditor`는 Static Mesh 변환용 Editor 도구로만 사용한다.
- `Water`, `PCGWaterInterop` 등 Water/Experimental 기능은 핵심 의존성으로 사용하지 않는다.
- 패키징 대상은 Windows이다.
- 작업 Level은 World Partition을 사용하지 않는다.
- Landscape Actor는 1개, 해상도는 `2017 × 2017`, Component는 `1,024`개다.
- Landscape XY 범위는 `-3,024,000 .. +3,024,000 cm`, 전체 폭은 약 `60.48 km`다.
- 확인된 Landscape paint/weightmap layer는 없다.
- Water Body, Water Zone, River Spline, Road Spline은 확인되지 않았다.

## 3. 생산 PCG 구성

### Forest

- Graph: `/Game/Environment/PCG/Graphs/PCG_ENV_ForestRegion`
- Sampling Subgraph: `/Game/Environment/PCG/Subgraphs/PCG_ENV_ForestRegionSampling`
- Meshes: `/Game/Environment/Vegetation/Trees/SM_ENV_AleppoPine_A` through `_D`
- Level Actors: `PCG_ENV_Forest_Y00_X00` through `PCG_ENV_Forest_Y07_X07`
- Outliner folder: `Environment/PCG/Production/Forest`

데이터 흐름은 다음과 같다.

```text
Owning PCGVolume Input
→ Landscape Surface Sampler
→ Normal To Density / slope threshold
→ ForestPatchNoise / ForestPatchThreshold
→ PCG_Exclude_Vegetation tagged-volume Difference
→ Forest suitability texture mask
→ Water-channel Binary Difference
→ Bounds Modifier / Self Pruning
→ Landscape-only World Raycast
→ TreeVariation
→ A/B/C/D Static Mesh Spawner (ISM)
```

`PCG_ENV_ForestRegionSampling`은 별도 Allowed Volume을 조회하지 않는다. 각 생산 PCGVolume의 own input bounds가 후보 생성 영역이다.

### Riparian

- Graph: `/Game/Environment/PCG/Graphs/PCG_ENV_RiparianTrees`
- Sampling Subgraph: Forest와 동일한 `PCG_ENV_ForestRegionSampling`
- Influence Subgraph: `/Game/Environment/PCG/Subgraphs/PCG_ENV_RiparianInfluence`
- Meshes: `/Game/Environment/Vegetation/Riparian/Trees/SM_ENV_BlackAlder_A` through `_D`
- Level Actors: `PCG_ENV_Riparian_Y00_X00` through `PCG_ENV_Riparian_Y07_X07`
- Outliner folder: `Environment/PCG/Production/Riparian`

Riparian은 Forest의 검증된 slope, manual exclusion, spacing, ground trace 흐름을 재사용한다. Forest suitability와 Riparian influence를 모두 만족하는 후보만 유지하고 water core는 별도 `Binary Difference`로 제거한다.

### Riparian Lower Layer V40 — 현재 생산 경로

- Graph: `/Game/Environment/PCG/Graphs/PCG_ENV_RiparianBankBandPrototypeV40`
- Bank Subgraph: `/Game/Environment/PCG/Subgraphs/PCG_ENV_RiverBankBandProductionV40`
- Water Subgraph: `/Game/Environment/PCG/Subgraphs/PCG_ENV_RiverWaterMaskProductionV40`
- Bank Texture: `/Game/Environment/PCG/Data/Mask/T_ENV_RiverBankBand_Daejeon_ProductionV40`
- Water Texture: `/Game/Environment/PCG/Data/Mask/T_ENV_RiverWater_Daejeon_ProductionV40`
- Level Actors: Production `19`, Prototype `0`
- River tile exception: `X05_Y00`은 Landscape/constraint 적용 후 후보 `0`으로 검증돼 Lower Layer Actor를 만들지 않는다.
- Outliner folder: `Environment/PCG/Production/Riparian/LowerLayer`
- 출력: grass/accent/young alder `ISM`, Editor pre-generated, `GenerateOnDemand`

V40은 이전 V37의 식생 종류, scale/rotation variation, Landscape grounding, ISM 출력과 성능 정책을 재사용한다. 변경된 핵심은 공간 입력이다. 넓은 `T_ENV_RiparianZones_Daejeon_Extended_VisualMatch` density를 `NearBankVegetationOnly`로 자르던 V37은 실제 강 경계와의 거리가 아니어서 강에서 멀리 떨어진 patch를 만들 수 있었다. V40은 현재 강 표면을 생성한 동일한 `T_ENV_RiverSurface_Daejeon_ShortGapBridges_v38.png` raster에서 수면 바깥쪽 ring을 직접 계산한다.

```text
V38 production river raster
→ water binary mask
→ outside-water 0–30 m inner bank (density 1.0)
→ outside-water 30–60 m transition bank (density 190/255)
→ V37 vegetation selection / variation / ground trace
→ water binary Difference
→ grass/accent/young alder ISM
```

Mask는 `2017 × 2017`, 약 `30 m/pixel`이다. 수면 pixel `30,547`, inner-bank pixel `11,055`, outer-transition pixel `10,908`, 전체 bank pixel `21,963`이며 bank/water overlap은 `0`이다. 이 60 m 폭은 현재 raster 해상도에서 가능한 가장 단순한 양안 연속 띠다. 실제 제방 폭이나 수문학적 식생대를 의미하지 않는다.

Production grass는 collision/shadow를 끄고 dense layer는 `50–500 m`, accent layer는 `80–700 m` culling을 사용한다. young alder는 collision을 끄고 shadow를 유지하며 `100–1,000 m` culling을 사용한다. World Partition/runtime PCG로 전환하지 않고 저장된 ISM 결과를 사용한다.

### 공용 수동 exclusion

- Actor: `PCG_EXCL_Manual_Central_01`
- Tag: `PCG_Exclude_Vegetation`
- Folder: `Environment/PCG/Constraints`

Forest와 Riparian Graph는 같은 tag를 조회하며, 필요한 경우 동일 tag의 추가 PCGVolume을 배치해 공용 제외 영역을 확장할 수 있다. 현재 도로·건물 footprint 전체를 표현하는 자동 exclusion data는 없다.

## 4. 전역 Tile Architecture

World Partition이 없는 현재 Level에서 하나의 거대한 PCGVolume을 재생성하지 않도록 Landscape를 각 계층마다 정확한 `8 × 8` tile로 분할했다.

- Tile size: `756,000 × 756,000 cm` = `7.56 × 7.56 km`
- Tile count: Forest `64`, Riparian `64`
- 생산 PCGVolume: `128`
- 수동 exclusion PCGVolume: `1`
- Tile Z center: `25,000 cm`
- Tile vertical extent: `50,000 cm`
- X/Y center: `-2,646,000`, `-1,890,000`, `-1,134,000`, `-378,000`, `378,000`, `1,134,000`, `1,890,000`, `2,646,000 cm`
- Forest seed: `137 + row*8 + column`, 범위 `137..200`
- Riparian seed: `241 + row*8 + column`, 범위 `241..304`
- 생산 trigger: 모든 `128/128` Component가 `GenerateOnDemand`

이 grid는 Landscape 외곽과 정확히 일치한다. Tile은 Editor 생성과 관리 단위를 나누지만 World Partition streaming을 대신하지 않는다. 저장된 ISM은 현재 Level 로드 시 함께 존재하므로 live runtime 성능 측정이 별도로 필요하다.

## 5. 공간 Mask

### 활성 Asset

- Forest suitability: `/Game/Environment/PCG/Data/Mask/T_ENV_ForestSuitability_Daejeon`
- Hard water exclusion: `/Game/Environment/PCG/Data/Mask/T_ENV_WaterChannel_Daejeon_Extended_PrimaryCalibrated`
- Riparian zones: `/Game/Environment/PCG/Data/Mask/T_ENV_RiparianZones_Daejeon_Extended_VisualMatch`

Water/Riparian mask는 `2017 × 2017`, Grayscale, sRGB off, NoMipmaps, Never Stream, Virtual Texture off, Bilinear, Clamp X/Y로 저장됐다. 두 Subgraph 모두 absolute transform scale `(3024000, 3024000, 1)`을 사용하고 `DensityMergeFunction = Set`으로 입력 density를 명시적으로 교체한다.

### 좌표 등록

원본 데이터는 OSM/Overpass GeoJSON이며 런타임에는 GeoJSON을 처리하지 않는다. 모든 결과는 개발 시 로컬 raster로 변환되어 프로젝트 Asset으로 저장된다.

프로젝트의 기본 위·경도 원점은 `(36.3504, 127.3845)`이며 `X=East`, `Y=-North`다. 제공된 `daejeon_satellite_z16`과 여러 저수지 제어점을 비교해 다음 영상 등록을 적용했다.

```text
x_sat = 1.1225866717114303 * x_base - 107.90908937682859
y_sat = 1.0082081114061412 * y_base - 7.532280681195135
```

UE 5.7 `UPCGTextureData`와 외부 검증이 공유하는 texel-center 공식은 다음과 같다.

```text
pixel_x = ((world_x_cm + 3024000) / 6048000) * 2017 - 0.5
pixel_y = ((world_y_cm + 3024000) / 6048000) * 2017 - 0.5
```

Landscape Material `/Game/Maps/M_Daejeon`의 실제 Base Color 경로는 `LandscapeLayerCoords → Multiply(0.0004959999932907522) → Frac → daejeon_satellite_z16`이다. Material 내부에 별도 회전 node는 없지만, GIS 좌표를 만든 v7 Overlay와 Editor의 표시 방향 사이에는 Y축 방향 불일치가 있었다. 사용자가 Editor에서 `Mirror Y`와 X scale을 조절해 주요 하도를 맞춘 결과를 regional calibration으로 채택했다. 따라서 visible-water 검수용 좌표는 실제 Material의 다음 pixel-center 변환과 아래 사용자 보정을 함께 검사한다.

```text
u = ((world_x_cm - landscape_origin_x_cm) / 3000) * 0.0004959999932907522
v = ((world_y_cm - landscape_origin_y_cm) / 3000) * 0.0004959999932907522
pixel_x = u * 2017 - 0.5
pixel_y = v * 2017 - 0.5
```

v7에서 저장된 수동 Actor Transform은 `Location=(-485149.0006,-1538182.6156,0) cm`, `Yaw=180°`, `Scale=(-1.12,1,1)`이다. 이 Transform을 좌표 생성식에 bake하면 다음 regional world affine이 된다.

```text
world_x_corrected = 1.12 * world_x_source + 45169.8801 cm
world_y_corrected = -1.0 * world_y_source - 3060805.2311 cm
```

이 값은 승인된 Primary 하천 구역용 보정이며 아직 대청호·탑정호를 포함한 전역 보정으로 확정하지 않았다. 생산 Asset에는 음수 Actor scale을 전파하지 않고, v9부터 vertex 좌표에 bake해 Actor scale `(1,1,1)`을 사용한다.

이 등록은 제공된 위성영상에 맞춘 시각 보정이며 공인 GIS/측량 결과가 아니다. 전역 affine 보정 뒤에도 위치별 잔차가 존재하므로, OSM 형상을 visible water로 만들 때 전역 또는 feature 전체에 동일한 translation을 적용해서는 안 된다. 약 `30 m/pixel` 입력이므로 좁은 수로, 정밀 제방, 실제 수위를 결정하는 데이터로 사용하지 않는다.

Source와 재생성 자료는 cook 대상이 아닌 다음 위치에 보존한다.

- `SourceData/Environment/River/SatelliteExtent`
- `SourceData/Environment/River/Generated_Extended`

원천은 OpenStreetMap이며 배포 시 `© OpenStreetMap contributors`, ODbL 출처 표기가 필요하다.

## 6. Visible River Prototype

현재 Level에는 수계 전체가 아니라 한 지역의 주요 하천 형상과 좌표 정합을 육안 검수하기 위한 Centerline 진단 Overlay 하나가 존재한다.

- Actor: `ENV_RiverPrimaryAlignmentOverlayPrototype_07`
- Folder: `Environment/River/Prototype`
- Mesh: `/Game/Environment/River/Meshes/SM_ENV_RiverPrimaryAlignmentOverlayPrototype_07`
- Material: `/Game/Environment/River/Materials/M_ENV_RiverSurface_CorrectedPrototype`
- Source: `Daejeon_SatelliteExtent_MajorWater_Centerline.geojson`
- Projection: WGS84를 기준 원점 `(36.3504, 127.3845)`, `30 m/pixel`로 직접 투영. image-fitted affine 미사용
- Region: 약 `21 × 15 km`
- Selected source: 주요 하천 `7` features, 연결 상태를 유지한 `7` clipped polylines
- Named rivers: `갑천`, `금강`, `미호강`, `미호천`
- Total centerline length: 약 `47.33 km`
- Diagnostic width: 고정 `90 m`. 광역 시점 가독성용이며 실제 하천 폭이 아님
- Resampling: segment 길이가 `60 m`를 넘지 않도록 분할
- Geometry: `1,810` vertices, `1,796` triangles
- Grounding: 모든 vertex를 Landscape에 trace하고 `8 m` 위에 배치. 좌표 확인용 Overlay 높이이며 실제 수위가 아님
- Collision, overlap, shadow, distance-field lighting, decals, Nanite, ray tracing: disabled

v5는 단일 Water Area Polygon `osm_id=34546126`을 면으로 채웠고 기술 검사는 통과했지만, 위성 영상에서 실제 연속 수로가 아닌 육지를 크게 따라가는 것이 확인돼 육안 정합 검수에 실패했다. 한 Polygon을 수면 전체로 해석하는 접근은 중단했다. v5 Actor는 제거했고 Mesh는 당시 비교용으로 보존했으나 v20 정리에서 함께 제거했다.

v6는 VisualMatch의 `15`개 하천과 `455`개 잘린 source segment를 폭 `30 m`, 지면 위 `18 cm`에 그렸다. 좌표는 맞았지만 광역 시점에서 작은 하천과 구름/지형 가림이 섞이고 선분 경계가 강조돼 사용자가 주요 강줄기의 정합을 판정하기 어려웠다. v6 Actor는 제거했고 Mesh는 당시 원인 비교용으로 보존했으나 v20 정리에서 함께 제거했다.

v7은 Major Centerline에서 금강/갑천/미호강/미호천만 선택하고, 원본 feature 연결을 유지한 polyline으로 다시 생성했다. 사용자가 Editor에서 `Mirror Y`와 X scale `-1.12`를 적용한 뒤 주요 하도와 일치한다고 판정했다. 저장·재로드 후 Level에는 v7 Actor `1`, v6 Actor `0`이 존재하며 전역 저장 수량은 Forest `310,501`, Riparian `34,153`으로 유지됐다. v7도 실제 강 폭, 수평 수면, flow, depth, Landscape carve 또는 Water Body 동작을 제공하지 않는다.

승인된 regional calibration을 실제 VisualMatch Water Area에 bake한 v9 시험 표면도 추가했다.

- Actor: `ENV_RiverWaterAreaRegionPrototype_09`
- Mesh: `/Game/Environment/River/Meshes/SM_ENV_RiverWaterAreaRegionPrototype_09`
- Source: VisualMatch Water Area 중 Primary Centerline corridor와 교차하는 `river` Polygon `8`개
- Approximate area: `10.3014 km²`
- Raster/grid: `30 m`, active cell `11,446`
- Geometry: `13,572` vertices, `22,892` triangles
- Grounding: 모든 vertex를 Landscape에 trace하고 `25 cm` 위에 배치
- Actor Transform: identity scale `(1,1,1)`; regional calibration은 vertex 좌표에 bake
- Collision, overlap, shadow, distance field, Nanite, ray tracing: disabled
- 역할: 수평 형상과 vegetation exclusion 검수용 draped surface. 실제 수평 수위나 Water Body가 아님

별도 시험 Texture `/Game/Environment/PCG/Data/Mask/T_ENV_WaterChannel_Daejeon_CalibratedPrimary`는 같은 보정 형상을 `2017 × 2017`로 만들고 shoreline 안전 여유 `1 pixel = 약 30 m`를 추가했다. 생산 경로에는 이 Texture를 단독 사용하지 않고 기존 Extended VisualMatch와 pixel-wise maximum으로 병합한 `/Game/Environment/PCG/Data/Mask/T_ENV_WaterChannel_Daejeon_Extended_PrimaryCalibrated`를 사용한다. 기존 양수 pixel `207,096`개를 모두 보존하면서 교정 Primary 수역 pixel `19,127`개를 추가해 최종 양수 pixel은 `226,223`개다.

v9 육안 승인 후 생산 water-mask Subgraph의 Texture reference를 위 병합 Asset으로 변경했다. 전체 128개 PCG 타일을 재생성하지 않고 실제 교정 수역과 겹친 Forest `6`개, Riparian `2`개만 갱신했다.

- Forest: `310,501 → 308,892` (`-1,609`)
- Riparian: `34,153 → 34,139` (`-14`)
- 교정 Primary 수역 cell 내부 overlap: Forest `945 → 0`, Riparian `5 → 0`
- 비대상 타일 instance 수: 전부 변경 없음
- 대상 타일 A/B/C/D variation: 전부 유지
- Fresh-load Map Check: `0 error, 0 warning`

### 대표 저수지 경계 진단 v13

Primary 구역에서 승인한 v9 world affine을 대청호·탑정호에 그대로 적용하면 대청호가 북쪽으로 크게 이동하고 탑정호가 Landscape 밖으로 빠지는 것을 외부 preview에서 확인했다. 따라서 v9 보정은 전역 보정으로 승격하지 않았다.

원본 VisualMatch Water Area의 대청호(`osm_id=413915`)와 탑정호(`osm_id=18072679`)는 WGS84를 위성 pixel 및 Landscape world로 직접 투영했을 때 `daejeon_satellite_z16` 수역 경계를 따른다. 기존 v7의 Y축 수동 반전은 GIS 데이터 자체가 아니라 OBJ import의 right-handed → left-handed 좌표 변환을 생성기에서 상쇄하지 않은 것이 주원인이었다. v13은 OBJ local Y를 import 전에 미리 반전하고 Actor Transform은 identity로 유지한다.

- Actors: `ENV_RiverBoundary_DaecheongPrototype_13`, `ENV_RiverBoundary_TapjeongPrototype_13`
- Meshes: `/Game/Environment/River/Meshes/SM_ENV_RiverBoundary_DaecheongPrototype_13`, `/Game/Environment/River/Meshes/SM_ENV_RiverBoundary_TapjeongPrototype_13`
- 역할: 실제 수면이 아닌 shoreline 좌표 정합용 `90 m` ribbon
- Daecheong: source `16,828` points → diagnostic `2,220` points, `4,428` traced vertices/triangles
- Tapjeong: source `547` points → diagnostic `119` points, `234` traced vertices/triangles
- Simplification tolerance: `45 m`; maximum segment: `240 m`; clearance: `8 m`
- Collision/overlap/shadow/distance field/decals/Nanite/ray tracing: disabled
- Direct-projection expected center 대비 saved Actor bounds center 오차: 대청호 약 `10.35 m`, 탑정호 약 `17.50 m`
- Forest/Riparian: `308,892 / 34,139` 유지; v9 Actor 유지

두 Actor는 생산 수면이 아니며 Editor 육안 승인 전에는 filled water surface 또는 전역 mask 변경의 근거로 사용하지 않는다. 대표 두 지점이 통과하면 같은 direct projection + OBJ Y compensation을 전역 수계의 tile/feature 배치에 사용한다.

### 대표 저수지 외곽 경계 진단 v14

v13 육안 보정에서 대청호와 탑정호 모두 X scale `1.12`가 필요했고, 각 Actor 위치 이동을 전역 좌표식으로 환산한 결과 기존 위성 Texture registration과 일치했다. 따라서 수동 Actor Transform을 생산 규칙으로 남기지 않고 다음 pixel affine을 WGS84 투영 결과에 bake했다.

- `x_sat = 1.1225866717114303 × x_base - 107.90908937682859`
- `y_sat = 1.0082081114061412 × y_base - 7.532280681195135`
- OBJ local Y pre-flip 유지
- Actor scale `(1,1,1)`, rotation `0`

v13은 대청호 Polygon의 outer ring과 내부 ring 5개를 모두 같은 ribbon으로 그려 섬/구멍도 하천처럼 보였다. v14는 실제 수면 외곽을 판단하기 위해 outer ring `0`만 사용한다. 탑정호도 outer ring만 사용하고 내부 ring 1개는 제외했다.

- Actors: `ENV_RiverOuterBoundary_DaecheongPrototype_14`, `ENV_RiverOuterBoundary_TapjeongPrototype_14`
- Meshes: `/Game/Environment/River/Meshes/SM_ENV_RiverOuterBoundary_DaecheongPrototype_14`, `/Game/Environment/River/Meshes/SM_ENV_RiverOuterBoundary_TapjeongPrototype_14`
- 외부 위성 preview: 대청호/탑정호 outer shoreline 정합 통과
- Editor 육안 정합: 사용자 승인. 수평 위치와 굴곡이 위성 수역과 일치
- Fresh-load technical validation: 통과
- Forest/Riparian 저장 수량: `308,883 / 34,137`; 당시 v9 및 v13 비교 Actor를 보존했으나 v20 정리에서 제거
- Landscape trace Z range: 대청호 약 `266.74 m`, 탑정호 약 `63.56 m`

높이 범위는 저수지 수위가 아니라 현재 Landscape 표면 높이이며 실제 저수지로 보기에는 너무 크다. 그러므로 v14도 지면을 따라간 진단 ribbon일 뿐이다. 외곽선 육안 승인 뒤에도 flat filled surface를 즉시 만들지 않고, 실제 수면 고도 결정 또는 Landscape/위성 불일치 처리 방식을 먼저 정한다. 최종 Polygon 면을 만들 때는 outer ring에서 내부 ring을 hole로 빼야 한다.

### 전역 수계 입력 및 수면 정책 v15

v14에서 승인한 satellite-registration affine은 전역 Water/Riparian Mask 생성기에 이미 사용되고 있었다. 프로젝트 안의 `SourceData/Environment/River/SatelliteExtent`를 원천으로 다시 생성한 결과, 다음 세 생산 PNG와 픽셀 차이가 모두 `0`이었다.

- `T_ENV_WaterChannel_Daejeon_Extended_VisualMatch`
- `T_ENV_RiparianZones_Daejeon_Extended_VisualMatch`
- `T_ENV_WaterChannel_Daejeon_Extended_PrimaryCalibrated`

따라서 v14 승인은 생산 좌표계를 새로 변경한 것이 아니라 기존 전역 Mask 좌표계를 독립적으로 검증한 결과다. PCG Graph는 Water에 병합된 `Extended_PrimaryCalibrated`, Riparian에 `Extended_VisualMatch`를 참조하며, Texture Sampler는 Landscape 전체 `±3,024,000 cm`, `Density Merge=Set`을 사용한다.

현재 저장된 Forest `308,883`개와 Riparian `34,137`개의 XY를 활성 Mask에 다시 대조한 결과 두 계층 모두 hard-water overlap `0`이었다. 이 검증은 좌표/Mask 변경과 관련된 항목만 대상으로 했으며 이미 해결돼 변하지 않은 접지·간격 검사는 반복하지 않았다.

보이는 수면은 전역 단일 Mesh로 자동 생성하지 않는다. registered Water Area 중 raster 면적이 큰 고유 수역 18개를 최대 96개 shoreline 표본으로 Landscape에 trace한 결과:

- 단일 평면 후보: `0`
- 구간별 또는 국소 수면 후보: `3`
- 수동 수면 제작 전까지 Mask 전용 유지: `15`
- 대청호 표본 전체 Z 범위: 약 `233.3 m` (`P90-P10 ≈ 77.2 m`)
- 탑정호 표본 전체 Z 범위: 약 `60.5 m` (`P90-P10 ≈ 22.2 m`)

이 값은 실제 수위가 아니라 과장된 현재 Landscape 표면과 shoreline 벡터가 만나는 높이다. 단일 Water Plane을 사용하면 광범위한 지형 관통/부유가 발생하므로, 생산 구조는 다음처럼 분리한다.

1. Water Area/Centerline raster: Forest/Riparian hard exclusion 및 bank influence의 전역 권위 입력
2. visible water: 필요한 구역만 짧은 구간으로 분할하고 별도 높이 검수 후 제작
3. 대형 저수지: 실제 수면고 또는 수동 보정된 수면 경계가 확보되기 전에는 위성 기반 수면 표현과 Mask를 유지
4. Water/PCGWaterInterop: production 필수 의존성으로 추가하지 않음

## 7. 생성 및 성능 정책

- 수동으로 생성한 결과를 Level에 저장하고 runtime 전체 재생성을 피한다.
- 수목은 Actor spawn이 아니라 `InstancedStaticMeshComponent`를 사용한다.
- Tree mesh는 Nanite가 활성화되어 있으나 simple collision primitive는 없다.
- Grass, shrub, rock은 아직 생산 계층에 추가하지 않았다.
- River 진단/수면 Prototype은 collision/shadow/distance field/Nanite/ray tracing을 사용하지 않는다. v9 수면 Mesh는 `22,892` triangles이며 생산 수목은 여전히 ISM이다.
- 전역 생성 총량과 commandlet 생성 시간은 기록했지만 FPS, Game Thread, GPU, VRAM은 아직 측정하지 않았다.
- World Partition 도입은 현재 단계에서 Level 구조를 크게 바꾸므로 자동으로 활성화하지 않는다.

## 8. 현재 제약과 다음 단계

1. 전역 Water/Riparian 좌표 및 hard exclusion은 승인된 상태로 고정한다. 같은 입력으로 전체 PCG 타일을 다시 생성하지 않는다.
2. 보이는 수면이 필요한 경우 v15에서 `segmented_or_local_surface_candidate`로 분류된 짧은 3개 구역 중 하나만 prototype으로 제작해 높이 정책을 검증한다.
3. Riparian `90 m / 300 m` 영향대는 현재 Forest와 독립된 Black Alder 계층으로 유지하고, 다음에는 관목/초본 자산 유무와 instance budget을 먼저 확정한다.
4. 도로/건물 exclusion data가 확보되면 공용 exclusion subsystem에 추가한다.
5. Shrub/Grass/Rock 자산을 확보한 뒤 계층별 instance budget을 정하고 별도 Graph로 추가한다.
6. live Editor/Standalone 성능 측정 뒤 Windows Development package, packaged Level 실행, 네트워크 차단 offline 실행을 검증한다.

현재 작업은 전역 Forest/Riparian 생성까지 완료됐지만 live performance와 packaged/offline 검증은 아직 완료되지 않았다.

## 9. 전역 River Surface v20 River-only

v17은 기술적인 저장·재로드 검사는 통과했지만 전역 XY 육안 검수에 실패했다. 원인은 다음 두 좌표 처리를 혼용한 구현 결함이었다.

1. 전역 285개 Water Area에 `regional prototype only`로 기록된 v7 world affine을 적용했다.
2. 대청호와 탑정호 두 feature에만 v13 Actor별 Transform을 다시 적용했다.
3. v14에서 검증된 OBJ local Y pre-flip도 v17 타일 importer에는 빠져 있었다.

그 결과 서로 연결된 수계가 서로 다른 좌표 프레임을 사용했고, 각 tile의 OBJ handedness까지 뒤집혀 Actor별 수동 Transform으로 수정할 수 없는 전역 오차가 발생했다. v17 Actor 36개는 Level에서 제거했다. v17 Mesh Asset은 당시 롤백 자료로 남겼지만 v20 검증 완료 후 제거했다.

v18은 Forest/Riparian 생산 Mask와 v14 대표 외곽선에서 이미 검증된 다음 단일 pixel affine을 모든 Water Area에 동일하게 적용했다. v19는 이 좌표계와 메시 생성 구조를 유지하면서 위성 근거가 약한 OSM feature `13`개를 제외했다. 그러나 원본 `VisualMatch Water Area`에는 강뿐 아니라 저수지·호수·운하 Polygon이 함께 들어 있었고, 이들이 산지와 시가지에 보이는 고립된 작은 수면의 주원인이었다.

현재 생산 경로인 v20은 같은 좌표 등록과 타일 생성 구조를 유지하되, visible surface 입력을 `properties.water == "river"`로 제한한다. 저수지·호수·운하는 visible surface에서 전부 제외하며, v19에서 이미 약한 근거로 판정된 river feature 3개도 추가로 제외한다. 원본 GeoJSON은 재현성과 향후 mask/분석 용도로 보존하고, 필터는 개발 시 생성 단계에서 결정적으로 적용한다.

```text
x_sat = 1.1225866717114303 × x_base - 107.90908937682859
y_sat = 1.0082081114061412 × y_base - 7.532280681195135
```

OBJ는 local Y를 import 전에 pre-flip하고 triangle winding도 함께 보정한다. 모든 Level Actor는 identity Transform을 사용한다.

- 원본 입력: `Daejeon_SatelliteExtent_VisualMatch_Water_Area.geojson`의 Water Area polygon `285`개
- 원본 분류: `river 96`, `reservoir 186`, `lake 2`, `canal 1`
- v20 visible surface 입력: `river 93`개
- v20 제외: 분류 기준 `reservoir 186 + lake 2 + canal 1`, 약한 river `3`개, 총 `192`개
- Mesh: `/Game/Environment/River/Production/Meshes/V20RiverOnly`
- Actor folder: `Daejeon_PCG_Work/Environment/River/Production`
- raster/cell: 약 `60 m` cell
- 타일: 최대 약 `7.68 km`, 실제 생성 `24`개
- 총 geometry: `13,524` vertices, `19,106` triangles
- Z: 각 vertex를 현재 Landscape에 trace한 뒤 `18 cm` 위에 배치
- Material: `/Game/Environment/River/Materials/M_ENV_RiverSurface_CorrectedPrototype`
- Collision/Overlap/Shadow/Distance Field/Nanite/Ray Tracing: 사용하지 않음
- 생성 정책: Level에 저장된 Editor-authored 결과이며 runtime PCG generation을 사용하지 않음

외부 preview에서 v19와 v20을 pixel 단위로 비교했다. v20은 새로 추가한 pixel이 `0`이고, 저수지·호수·운하 및 약한 river feature에 해당하는 `85,362` pixel을 제거했다. 독립 Unreal 프로세스의 저장 Map 재로드 검사에서 v20 Actor `24/24`, Mesh 경로, identity Transform, Material, Collision/Shadow 비활성 상태와 `RiverOnlySurfaceV20` tag가 모두 일치했으며 Map Check는 `0 error / 0 warning`이다.

이 표면은 벡터의 `water=river` Polygon을 Level에서 보이게 하는 지형 추종형 visual proxy이다. Unreal `Water Body River`나 수리 시뮬레이션이 아니며, 물리 유속·수심·부력·완전한 수평 수면을 제공하지 않는다. v20은 좌표·저장·재로드·분류 필터에 대한 기술 검증을 통과했지만, 강별 수면 높이와 최종 물 Material은 다음 단계에서 별도로 검증한다.

전환 후 Level의 v19 Actor `62`개는 v20 Actor `24`개로 교체했다. 참조가 없어진 구형 River Static Mesh `173`개와 비교용 진단 Actor `6`개는 Unreal Asset API로 제거했다. 현재 `Content/Environment/River`의 생산 Asset은 v20 Mesh `24`개와 공유 Material `1`개다. 이번 v20 전환에서는 요청 범위 밖인 Forest/Riparian PCG와 constraint mask를 수정하거나 재생성하지 않았다.

## 10. 환경 보조 Static Mesh

CC0 Kenney Nature Kit 2.1에서 관목 2개, grass clump 2개, 수변 초본 대체재 2개, 암석 4개를 가져왔다. 정확한 경로와 라이선스는 `Docs/THIRD_PARTY_ASSETS.md`를 따른다.

이 Mesh들은 저폴리·무 Collision·Nanite 비활성 상태이며 성능 prototype에 적합하다. 다만 기존 사실적 수목과 비교해 stylized한 시각 차이가 있으므로 전역 PCG Graph에는 아직 연결하지 않았다. 특히 `RiparianTallPlant`는 reed의 생물학적 동정 자산이 아니라 수변 초본 배치 규칙 검증용 시각 대체재다.

## 11. 현재 River Surface v23 Smoothed Banks + WaterMaterials

강 표면은 약 `15 m` 간격의 terrain-following vertex를 유지하면서, 30 m 수역 raster의 네모난 계단 경계를 완화한 v23 메시를 사용한다. 생산 경로는 `/Game/Environment/River/Production/Meshes/V23SmoothedBanks`이며, Level에는 공간적으로 유효한 `20`개 `StaticMeshActor`만 저장한다. 제외 타일은 `X02_Y05`, `X04_Y05`, `X05_Y04`, `X05_Y05`로 변함없다. 이전 v22 Mesh는 문제 발생 시 되돌릴 수 있도록 Asset만 보존하고 Level에서는 참조하지 않는다.

경계 보정은 강의 위치나 수역 분류를 다시 계산하지 않는다. 전역 메시 연결 관계에서 실제 바깥 강둑 edge만 찾은 뒤 degree-2 경계 정점에 제한된 Taubin smoothing을 적용한다. 합류점과 끝점은 고정하며, 이동 상한은 source raster의 `0.35 pixel`(약 `10.5 m`)이다. 실제 최대 이동은 약 `9.38 m`, 면적 변화는 `0.112%`, 뒤집힌/퇴화 삼각형은 `0`이다. Landscape 경계를 벗어나는 31개 정점은 안전하게 v22 좌표를 유지한다.

수면 표현은 Fab의 `Water Materials` 패키지를 runtime 다운로드 없이 로컬 Content로 보관하고, 그중 비용이 상대적으로 낮은 `/Game/WaterMaterials/Materials/M_River_Cheaper`를 parent로 사용한다. 프로젝트 조정값은 다음 하나의 공유 Material Instance에만 보관한다.

- Material Instance: `/Game/Environment/River/Materials/MI_ENV_RiverSurface_WaterMaterials_Flow`
- Actor/Mesh material assignment: `20/20`
- Collision/Overlap/Shadow/Distance Field/Decal/Ray Tracing: 비활성 유지
- Generation: Editor-authored Static Mesh, runtime PCG/GIS/network 처리 없음
- Geometry: `135,893` vertices / `241,216` triangles, topology unchanged from v22
- Contact: Landscape trace + mesh `100 cm` + Actor `75 cm` = nominal `175 cm`

수면의 색·투명도·반사와 normal 기반 움직임은 Material Instance에서 공통 조정한다. 항공 시점의 밝은 cyan 띠와 바다처럼 교차하는 큰 물결을 줄이기 위해 Roughness를 높이고 Specular/CubeMap/FakeSpec과 WPO 관련 intensity를 낮췄다. 작은 directional normal 움직임은 남겨 물이 완전히 정지해 보이지 않도록 했다. 이 방식은 모든 굴곡에서 하천 접선 방향을 따라가는 유체 시뮬레이션이 아니다. 현재 목표는 저장된 강 polygon 표면에 저비용의 움직이는 수면 인상을 제공하는 것이며, 굴곡별 정확한 흐름 방향이 필요하면 향후 spline tangent 또는 flow-map/vertex data를 별도 제작해야 한다.

WaterMaterials의 라이선스와 출처 표시는 `Docs/THIRD_PARTY_ASSETS.md`에 기록한다. `DroneSim.uproject`에는 이 Material 적용을 위해 추가한 plugin이 없다.

## 12. X03_Y01 Directional Flow Map Prototype

굴곡마다 흐름 방향이 바뀌는 표현의 기술 가능성을 확인하기 위해 생산 타일 중 `ENV_RiverSurface_Production_X03_Y01` 한 개에만 방향성 Flow Map Prototype을 적용했다. 나머지 생산 타일 `19`개는 기존 공유 Material Instance를 계속 사용하므로 A/B 비교와 안전한 롤백이 가능하다.

- Flow source: `Daejeon_SatelliteExtent_VisualMatch_Centerline.geojson`
- Registration: 현재 수역/위성 정합에 승인된 전역 pixel affine과 동일
- Encoding: `R/G = 정규화된 2D flow tangent`, `B = water mask`, `A = dilated sampling mask`
- Texture: `/Game/Environment/River/Textures/Prototype/T_ENV_RiverFlow_X03_Y01_Prototype`
- Parent Material: `/Game/Environment/River/Materials/Prototype/M_ENV_RiverSurface_DirectionalFlow_Prototype_01`
- Material Instance: `/Game/Environment/River/Materials/Prototype/MI_ENV_RiverSurface_DirectionalFlow_Prototype_01`
- 적용 대상: `ENV_RiverSurface_Production_X03_Y01` component override만 변경

Material은 world position을 타일 UV로 변환하고 Flow Map의 접선 방향으로 두 개의 normal texture를 서로 다른 scale/speed로 이동시킨다. 물리 유체 시뮬레이션, Water Body River, vertex displacement, 수심·부력 계산은 사용하지 않는다. 따라서 생산 Mesh topology, Landscape 접지, Collision/Shadow 비활성 정책과 Editor-authored offline 실행 구조는 그대로 유지된다.

현재 flow 방향의 축은 GeoJSON LineString의 vertex 순서를 따른다. 굴곡을 따라 방향은 변하지만 그 순서가 수문학적 하류 방향이라는 보장은 아직 없다. Editor/PIE에서 실제 움직임을 보고 역방향이면 두 speed parameter의 부호를 함께 바꾸는 것이 올바른 조정이며, Flow Map을 다시 생성할 필요는 없다.

## 13. River Surface V24 Global Directional Flow

한 타일 Prototype에서 확인된 굵은 수평 띠는 공간적으로 변하는 방향 벡터에 무한히 증가하는 `Time`을 직접 곱하면서 굴곡부 UV phase가 압축된 것이 주원인이었다. V24는 이 문제를 전역 Flow Map의 연결성 보정과 bounded two-phase sampling으로 해결한다. v23 지형 추종 Mesh, Transform, 접지, 수역 분류는 변경하지 않았다.

- Source geometry: `/Game/Environment/River/Production/Meshes/V23SmoothedBanks`
- Flow source: `Daejeon_SatelliteExtent_VisualMatch_Centerline.geojson`
- Flow textures: `/Game/Environment/River/Textures/Production/FlowV24`, 타일별 `256×256` RGBA `20`개
- Shared parent: `/Game/Environment/River/Materials/M_ENV_RiverSurface_DirectionalFlow_V24`
- Per-tile instances: `/Game/Environment/River/Materials/Instances/FlowV24`, `20`개
- Actor assignment: 활성 river-only Actor `20/20`; 타일 ID와 Flow Texture/Material Instance를 1:1 대응
- Rendering: Opaque, WPO 없음, texture sample `5`, Collision/Shadow 비활성
- Runtime: 저장된 Static Mesh와 로컬 Texture만 사용하며 PCG/GIS/network 계산 없음

Flow Map 생성기는 각 타일을 독립 처리하기 전에 전체 `2048×2048` 방향장에서 연결된 중심선 tangent의 부호를 연속화하고 masked smoothing을 수행한다. 따라서 feature 순서가 달라 생기는 180도 방향 반전과 타일 경계 smoothing 불연속을 줄인다. 각 Material Instance는 동일 parent를 공유하고 `FlowMap`과 `MapUVBias`만 타일별로 override한다.

Material은 `frac(Time × Speed)`로 제한된 두 phase를 0.5 간격으로 만들고 두 normal sample을 교차 blend한다. 시간값이 계속 증가해도 UV 이동 거리 자체는 `PrimaryTravel`/`DetailTravel` 안에서 반복되므로 굴곡부 phase 압축이 누적되지 않는다. 큰 primary normal은 색 변조에서 제외하고 작은 detail normal만 약하게 색에 반영한다. 이는 물리 유체, 수심, 부력, 실제 수문학적 하류 계산이 아니라 굴곡을 따르는 저비용 시각 흐름이다.

UE 5.7.4의 `DeleteAllMaterialExpressions()`는 배열 순회 중 같은 배열을 수정해 한 번 호출로 일부 expression을 남길 수 있었다. V24 재구축 도구는 expression 수가 0이 될 때까지 반복 삭제하고 감소가 멈추면 실패하도록 만들어, 재실행 시 고아 노드와 shader 오류가 누적되지 않게 했다.

## 14. Riparian Dense Meadow V31 Prototype

하천변 Grass는 생산 수목 Graph의 전역 설정을 올리지 않도록 별도 샘플링 Subgraph를 사용한다.

- Prototype Graph: `/Game/Environment/PCG/Graphs/PCG_ENV_RiparianLowerLayerPrototype`
- Prototype Actor: `PCG_ENV_RiparianLowerLayer_Prototype_Y01_X03`
- Dedicated sampling: `/Game/Environment/PCG/Subgraphs/PCG_ENV_RiparianGroundCoverSampling`
- Preserved hard inputs: `/Game/Environment/PCG/Subgraphs/PCG_ENV_RiparianInfluence`, `/Game/Environment/PCG/Subgraphs/PCG_ENV_WaterChannelMask`
- Source sampling: `/Game/Environment/PCG/Subgraphs/PCG_ENV_ForestRegionSampling`은 읽기 전용 기준으로만 사용하며 밀도를 변경하지 않음
- Output: 두 Static Mesh Spawner가 생성한 `Dense Sward` 4종 + `Tall Seedhead Accent` 4종의 ISM 계층
- Generation: 저장된 단일 셀 Editor 결과; runtime PCG/network 처리 없음

전용 샘플러 분리는 Grass 밀도를 조절해도 Forest와 Black Alder 생산 Actor가 재생성되거나 수량이 바뀌지 않게 한다. Prototype은 하천변 영향 mask 안에서만 생성되고 Water channel data는 `Difference`로 hard reject한다. Grass는 무 Collision·무 Shadow이며 per-instance cull distance를 사용한다. 현재는 전역 생산 단계가 아니라 `Y01_X03` 한 셀의 시각·생성비용 검증 단계다.

V31은 V30의 넓고 낮은 바닥층이 여전히 성기고 균등하게 보이는 문제를 수정한다. 기존 `RiparianInfluence → WaterExclusion → Landscape World Raycast` 제약은 그대로 두고, Water 후단의 point stream만 두 갈래로 나눈다. 두 갈래는 같은 군락 후보 영역을 독립적으로 샘플링하므로 서로 겹칠 수 있다.

1. `Dense Sward`: 비교적 키가 있는 PN Grass 4종, 0.5 m spacing, 비균일 scale `(1.25..1.75, 1.25..1.75, 1.15..1.55)`, 50..350 m cull
2. `Tall Seedhead Accent`: 키 큰 PN Grass 4종, 0.8 m spacing, 1.10..1.55 uniform scale, 80..500 m cull

두 층은 동일한 broad-scale `MeadowPatchNoise`와 높은 threshold `0.68`을 공유한다. 허용 영역을 좁히는 대신 전용 sampler를 `0.75 points/m²`로 높이고 layer별 spacing을 줄여, 군락 내부에는 겹침이 허용되는 조밀한 풀밭을 만들고 군락 사이에는 열린 지면을 남긴다. 각 층은 독립된 deterministic `Select Points`, Bounds, randomized Self Pruning, Ground Trace, World Raycast, Variation, Static Mesh Spawner를 사용한다. 저장된 단일 셀 결과는 Dense `49,200`, Accent `15,133`, 합계 `64,333` instances이며 전역 생산 배포는 아직 하지 않았다. V30보다 총수는 줄었지만 점을 더 적은 군락에 집중해 25 m 셀 최대 `580`, 점유 셀 중앙값 `162` instances를 기록했다.

선택 Mesh 원본 높이와 V31 scale을 합산한 대략적 최종 높이는 약 `0.6..1.5 m`다. 이는 강변 초본이 멀리서도 읽히도록 한 시각 설계값이며 식물학적 실측값은 아니다. Grass는 여전히 무 Collision·무 Shadow이고 근거리/중거리 cull을 사용한다. 단순 64셀 복제 시 약 `4.12 million` instances가 될 수 있으므로 실제 Editor/PIE 드론 경로의 GPU 및 masked overdraw 검수 전에는 전역 배포하지 않는다.

새로 추가된 `PN_GrassLibrary`의 완성 Static Mesh만 사용했다. `Shrub_Greasewood`의 작은 Static Mesh들은 Procedural Vegetation Editor의 완성 식물이 아니라 잎/가지 조립 부품이므로 직접 Spawner에 넣지 않는다. `Shrub_Huckleberry_Oak` 패키지는 현재 프로젝트보다 새로운 Unreal custom version으로 저장되어 UE 5.7.4에서 로드할 수 없으므로 참조하지 않는다.

## 15. Forest Canopy / Near-Bank Layers V32

전역 Forest는 기존 Graph, 8×8 tile, mask, seed, species weight를 유지하고 수관을 드론 시점에서 더 분명하게 읽히게 하는 최소 조정만 적용했다.

- Graph: `/Game/Environment/PCG/Graphs/PCG_ENV_ForestRegion`
- 간격 기준: `TreeSpacingBounds ±750 cm` (`15 m`)
- 균일 scale: `1.22..1.55`
- Aleppo Pine A/B/C/D weight: `5/5/1/1`
- 저장 결과: 64 Actor, `367,702` instances, 빈 tile `0`
- 생성 방식: 기존 PCG ISM, `GenerateOnDemand`, Editor 생성 결과 저장

하천변 하층 Prototype은 V31의 넓은 Riparian 범위를 그대로 채우지 않고, Water hard exclusion 뒤에 `NearBankVegetationOnly` density gate를 추가한다. 저장 Graph의 핵심 흐름은 다음과 같다.

`RiparianGroundCoverSampling → Landscape/patch constraints → Riparian Influence → WaterExclusion → Filter By Type → NearBankVegetationOnly → Dense Sward / Tall Seedhead / Young Alder`

`NearBankVegetationOnly`는 density `0.38..1.0`만 통과시킨다. 현재 Riparian mask의 Outside/Outer/Near/Water 값 `0/85/170/255` 중 Outer 약 `0.333`은 탈락하고 Near 약 `0.667`은 통과한다. Water는 이 gate보다 앞선 `WaterExclusion`에서 hard reject된다.

1. `Dense Sward`: Grass 4종, `0.4 m` spacing, scale `(1.35..1.85, 1.35..1.85, 1.30..1.75)`, 50..350 m cull
2. `Tall Seedhead`: Grass 4종, `0.65 m` spacing, `1.25..1.80` uniform scale, 80..500 m cull
3. `Young Alder`: Black Alder A/B/C/D, `12 m` spacing, `0.35..0.60` uniform scale, 100..1,000 m cull

세 branch는 독립적인 Bounds/Self Pruning/World Raycast/Variation/Static Mesh Spawner를 사용하므로 같은 작은 군락 안에서 풀 종류가 섞일 수 있고 모든 출력은 Landscape에 다시 접지된다. 저장된 `Y01_X03` 단일 셀 결과는 Dense `10,983`, Accent `3,375`, Young Alder `67`, 합계 `14,425` instances다. 25 m 셀 기준 최대 `580`, 점유 셀 중앙값 `139`이므로 넓게 균등 분산된 결과가 아니라 좁은 내부에 조밀한 군락이다.

고밀도 Grass는 Collision과 Shadow를 끄고 근·중거리 cull을 사용한다. Young Alder만 Shadow를 켜 하천변 입체감을 만들며 Collision은 끈다. 이 결정은 14,000개 이상 masked Grass의 shadow 비용을 피하면서 사용자가 요청한 그림자를 낮은 수량의 교목 계층으로 제공하기 위한 것이다. 현재 구현은 단일 셀 Prototype이며 64셀 전역 배포는 아직 하지 않았다.

## 16. Forest Density / Grass Visibility V33

V33은 기존 전역 Forest 구조와 단일 셀 하천변 하층 구조를 그대로 사용한 시각 조정이다. 새 Graph, C++, plugin, runtime generation 경로는 추가하지 않았다.

### Forest

- `PCG_ENV_ForestRegionSampling` density: `0.0016 → 0.0020 points/m²`
- `TreeSpacingBounds`: `±750 cm` 유지, 목표 간격 `15 m` 유지
- `TreeVariation`: uniform `1.22..1.55` 유지
- Aleppo Pine A/B/C/D weight: `5/5/1/1` 유지
- 저장 결과: 64 Actor, 256 ISM component, 빈 tile `0`, 총 `419,965` instances
- Mesh별 수량: A `175,367`, B `174,881`, C `34,892`, D `34,825`

즉, 나무 크기와 최소 spacing을 다시 바꾸지 않고 후보 sampling density만 높여 V32의 `367,702`보다 `52,263`개, 약 `14.22%` 증가시켰다. 같은 군락 모양과 종 비율을 유지하면서 숲 내부의 빈 인상을 완화하는 조정이다.

### Near-Bank Grass

- Dense Sward scale: `(1.55..2.10, 1.55..2.10, 1.50..2.05)`
- Tall Seedhead scale: uniform `1.45..2.05`
- Dense/Accent spacing: `0.4 m / 0.65 m` 유지
- Dense/Accent/Young Alder 수량: `10,983 / 3,375 / 67`, 총 `14,425` 유지
- Dense cull: `50..500 m`; Accent cull: `80..700 m`
- Grass Collision/Shadow: Off/Off 유지; Young Alder: Collision Off, Shadow On 유지

Grass는 점 분포나 군락 면적을 늘리지 않고 Mesh scale과 중거리 cull만 높였다. 작은 masked Grass를 수 km 항공뷰까지 유지하는 계층으로 사용하지 않으며, 현재 V33 역시 `Y01_X03` 단일 셀 Prototype이다.

Forest 재생성 과정에서 생산 Riparian tree Graph Asset은 수정하지 않았으나, 저장된 Level의 Black Alder ISM 수량은 fresh reload에서 `35,523`으로 확인됐다. 이는 이전 기록 `34,137`보다 `1,386`개 많다. 원인에 대한 추측은 확정하지 않으며, 향후 전역 성능 측정은 현재 저장값 `35,523`을 기준으로 한다.

## 17. Selective Riparian Lower-Layer Production V34

V34는 승인된 V33 하층 식생 Graph를 64개 Landscape 셀 전체에 복제하지 않는다. 저장된 River Surface와 실제로 교차하는 활성 수계 셀 `20`개만 입력 후보로 삼았고, 그중 하층 식생 적합도가 실제로 존재하는 `19`개 영역만 생산 Actor로 저장했다.

- Graph: `/Game/Environment/PCG/Graphs/PCG_ENV_RiparianLowerLayerPrototype`
- Sampling: `/Game/Environment/PCG/Subgraphs/PCG_ENV_RiparianGroundCoverSampling`
- Actor folder: `Environment/PCG/Production/Riparian/LowerLayer`
- Generation: `Generate On Demand`, Editor에서 생성한 ISM 결과 저장
- Runtime: PCG 재생성, GIS, network 또는 Fab 접근 없음
- Production coverage: River 활성 셀 `20`개 중 하층 적합 영역 `19`개
- Stored result: Actor `19`, instance `642,794`

일반 셀 `18`개는 정렬된 `PCG_ENV_Riparian_Ynn_Xnn` Volume의 위치와 크기를 그대로 재사용한다. `X04_Y01`은 전체 셀 생성이 UE 5.7.4에서 비정상적으로 장시간 진행되는 현상이 있어 2×2 quadrant를 독립 검사했다. `Q01`만 `7,826`개를 생성했고 나머지 세 quadrant는 `0`개였으므로, 저장 Actor는 `PCG_ENV_RiparianLowerLayer_Y01_X04_Q01` 하나만 사용한다. `X05_Y00`도 전체 생성 결과가 `0`이어서 빈 PCG Actor를 저장하지 않았다.

출력 계층은 Dense Sward `490,853`, Tall Accent `149,289`, Young Alder `2,652`다. Grass는 두 계층 합산 `640,142`이며 모두 Collision/Shadow를 끄고 각각 `50..500 m`, `80..700 m` cull을 사용한다. Young Alder는 Collision을 끄고 Shadow를 켜며 `100..1,000 m` cull을 사용한다. 따라서 642,794개를 항상 렌더링하는 구조가 아니라, 하천 근처의 현재 카메라 주변 인스턴스만 가시화되는 Editor-authored ISM 계층이다.

전역 Forest 밀도는 V34에서 변경하지 않았다. 다음 산림 조정은 `PCG_ENV_ForestRegionSampling`의 전역 density를 다시 올리는 방식이 아니라, 기존 Forest suitability/patch 결과 중 높은 구간에만 추가 수관점을 통과시키는 조건부 branch로 설계한다. 구현 전에는 현재 Forest의 cull/LOD/HLOD 상태와 실제 드론 경로의 frame time을 먼저 측정한다.
## 18. Forest High-Suitability Densification V35 Prototype

V35는 전체 산림 밀도를 일괄 증가시키지 않고, 기존 Forest Suitability가 높은 지점에만 추가 후보를 허용하는 시험 구성이다.

- 생산 Graph `/Game/Environment/PCG/Graphs/PCG_ENV_ForestRegion`과 생산 Sampling Subgraph는 변경하지 않았다.
- 시험 전용 Graph `/Game/Environment/PCG/Graphs/PCG_ENV_ForestHighSuitabilityPrototype`을 추가했다.
- 시험 전용 Sampling Subgraph `/Game/Environment/PCG/Subgraphs/PCG_ENV_ForestHighSuitabilitySamplingPrototype`을 추가했다.
- 시험 대상은 `PCG_ENV_Forest_Y00_X00` 한 셀뿐이다.
- 시험 Sampling 밀도는 `0.004 point/m²`이고, 기존 생산 Sampling 밀도 `0.002 point/m²`는 유지했다.
- 기존 exclusion을 통과한 Point를 Suitability Density `0.65` 기준으로 나눴다.
- Density가 `0.65` 미만인 영역은 기존 15m 간격을 유지한다.
- Density가 `0.65` 이상인 영역만 12m 간격을 허용한다.
- 두 Stream은 기존 공용 `Self Pruning` 전에 다시 합쳐지므로, 서로 다른 Stream에서 생성된 나무끼리도 중복 제거 대상이 된다.
- 기존 수종 A/B/C/D, Scale, 회전, Ground Trace, Collision 정책은 변경하지 않았다.

시험 셀은 `7,258 → 8,272` 인스턴스로 증가했다. 증가량은 `1,014`, 증가율은 `13.9708%`이다. 최소 XY 중심 간격은 `1,215.388cm`였고 10m 미만 인스턴스 쌍은 없었다. 저장 후 두 차례 재생성 결과도 동일했다.

이 단계에서는 64개 전역 셀에 배포하지 않는다. 시험 셀을 실제 드론 시점에서 확인하고 성능 기준을 기록한 뒤, 같은 규칙을 생산 Graph로 승격할지 결정한다.

## 19. Drone-View Distance Optimization Audit V35

실제 `Daejeon_PCG_Work` WorldSettings의 Pawn은 `/Game/Characters/Drone/Flying_drone_/Blueprints/BP_ThirdPersonCharacter`이다. 이 Blueprint에서 확인한 카메라 기준은 다음과 같다.

- `FollowCamera` FOV: `90°`
- `SpringArm` Target Arm Length: `10cm`
- Projection: Perspective
- Post Process Blend Weight: `1.0`

Level은 World Partition, Data Layer, HLOD Actor를 사용하지 않는다. 산림은 8×8 PCG Volume Tile과 ISM Component로 저장되어 있다. 현재 산림 ISM의 Start/End Cull Distance는 모두 `0/0`이고 Shadow는 켜져 있다. A/B/C/D Tree Mesh는 Nanite가 켜져 있으나 각 Mesh의 전통적 LOD 수는 1개다.

따라서 최적화는 화면 전체를 후처리로 흐리게 만드는 방식보다 다음 순서를 사용한다.

1. 실제 드론 고도와 대표 비행 경로에서 CPU/GPU Frame Time을 기록한다.
2. 시험 셀 하나에서 ISM End Cull Distance와 원거리 Shadow 비용을 조정한다.
3. 가까운 산림의 외형과 실루엣이 유지되는지 확인한다.
4. 유효한 거리값이 확정된 뒤 동일 Category에만 전역 적용한다.
5. Grass와 작은 Riparian Asset은 Tree보다 짧은 Cull Distance를 사용한다.
6. 필요할 때만 Material 복잡도와 Nanite/Foliage 설정을 추가 조정한다.

후처리 Blur는 멀리 있는 Geometry, Shadow, Material 처리 비용을 직접 제거하지 않으므로 1차 최적화 수단으로 사용하지 않는다. 현재 Level을 World Partition/HLOD 구조로 전환하는 것도 이번 단계에는 적용하지 않는다. 이는 단순 Parameter 조정이 아니라 Level Architecture 변경이며, 현재의 8×8 Tile 구조를 먼저 측정하고 최적화하는 편이 더 작고 안전한 변경이다.

## 20. Forest Open-Water Exclusion V36 Prototype

V35의 산림 덩어리 표현을 유지하면서 `PCG_ENV_Forest_Y00_X00` 안의 누락된 정안저수지만 hard reject하는 추가 시험 경로다. 기존 공용 `PCG_ENV_WaterChannelMask`와 생산 Forest/Riparian Graph는 변경하지 않았다.

- OSM Full Water Area와 위성 수면 후보를 교차검증해 `정안저수지`, `water=reservoir`, OSM way `119122441` 하나만 선택했다.
- 원래 수역 폴리곤에 `2 pixel`, 약 `60m`의 수목 안전 여유를 적용했다.
- 원본/출처 보존: `SourceData/Environment/Forest/OpenWater/Daejeon_ForestOpenWaterExclusion_Y00_X00_v36.geojson`
- 시험 Texture: `/Game/Environment/PCG/Data/Mask/T_ENV_ForestOpenWaterExclusion_Daejeon_Prototype`
- 시험 Mask Subgraph: `/Game/Environment/PCG/Subgraphs/PCG_ENV_ForestOpenWaterMaskPrototype`
- 시험 Forest Graph: `/Game/Environment/PCG/Graphs/PCG_ENV_ForestHighSuitabilityOpenWaterPrototype`

V36 Graph는 V35를 복제하고 기존 `WaterExclusion` 직후, V35의 `StandardSuitabilitySpacing`/`HighSuitabilitySpacing` 분기 전에 다음 체인을 한 단계만 추가한다.

`WaterExclusion → ForestOpenWaterMask / ForestOpenWaterExclusion → 기존 V35 suitability spacing`

따라서 Sampling density, suitability threshold, 15m/12m 간격, A/B/C/D 수종 가중치, Scale, Ground Trace, Collision/Shadow 정책은 V35와 같다. `Y00_X00`만 V36을 사용하며 다른 63개 Forest Tile은 그대로다. 위성에서 어둡게 보이지만 OSM 수역과 교차 근거가 없는 두 번째 표시 구역은 산림 오삭제를 피하기 위해 제외하지 않았다.

## 21. Riparian Lower-Layer Cost Reduction V37 Prototype and Production

V37은 먼저 `PCG_ENV_RiparianLowerLayer_Prototype_Y01_X03` 한 구역에서 저비용 구성을 시험했다. Prototype의 시각 구조와 결정성을 검증한 뒤 고부하 생산 셀 네 개로 작은 batch를 확인했고, 같은 acceptance를 통과한 나머지 15개 생산 영역까지 순차 적용했다.

- 시험 Graph: `/Game/Environment/PCG/Graphs/PCG_ENV_RiparianLowerLayerOptimizedPrototypeV37`
- 비교 원본: `/Game/Environment/PCG/Graphs/PCG_ENV_RiparianLowerLayerPrototype`
- 시험 Actor: `PCG_ENV_RiparianLowerLayer_Prototype_Y01_X03`
- 원본 Graph는 보존한다. 현재 생산 Actor `19`개와 Prototype `1`개는 모두 V37을 사용하며, 원본 Graph를 참조하는 저장 생산 Actor는 `0`개다.
- 생산 Actor Outliner 경로: `Environment/PCG/Production/Riparian/LowerLayer`
- Prototype Outliner 경로: `Environment/PCG/Riparian/Prototype`

현재 Grass 자산은 이미 여러 잎을 한 Static Mesh로 묶은 clump이고 PCG는 이를 ISM으로 저장한다. 따라서 여러 clump를 다시 하나의 대형 Mesh로 합치는 방식은 instance 관리 비용은 줄일 수 있어도 masked polygon/overdraw를 직접 제거하지 못하고, culling 단위와 자연스러운 변형을 지나치게 크게 만든다. V37은 별도 C++/Blueprint/합본 Mesh 없이 다음 최소 변경을 사용한다.

1. Dense와 Accent의 point 선택 비율을 낮춘다.
2. Dense 4종 중 footprint 대비 LOD0 triangle 비용이 큰 2종만 같은 로컬 Grass library의 더 넓고 가벼운 4-LOD clump로 교체한다.
3. 줄어든 인스턴스 사이의 시각적 공백을 보완하기 위해 Dense XY scale만 소폭 높이고 Z scale, Ground Trace, Water exclusion과 군락 mask는 유지한다.
4. Grass Collision/Shadow와 기존 category별 cull distance는 유지한다.
5. Young Alder 계층은 수량, Mesh, Scale, Collision/Shadow와 cull을 모두 유지한다.

단일 시험 셀은 `14,425 → 9,558`개로 감소했다. 같은 셀의 `instance × LOD0 triangle` 비용 추정치는 `6,078,272 → 3,208,107`로 감소했다. 이는 실제 GPU Frame Time이 아니라 동일 조건 비교용 정적 비용 proxy이다. 전역 생산 승격 전에는 실제 드론 경로에서 Frame Time과 masked overdraw를 별도로 측정해야 한다.

UE 5.7.4의 큰 PCGVolume을 한 Editor process 안에서 즉시 두 번 `Cleanup → Generate`하면 Surface Sampler safeguard가 두 번째 생성을 중단하는 경로가 확인됐다. 저장 결과는 비우지 않았으며, 서로 다른 fresh Editor process 두 번에서는 인스턴스 수와 Transform hash가 정확히 일치했다. 따라서 생산 재생성은 셀별 단일 pass와 fresh reload 검증을 사용한다.

고부하 네 셀의 첫 batch는 `374,334 → 250,154` instances였고, 후속 15개 생산 영역도 같은 셀별 acceptance를 통과했다. 생산 하층 식생 전체는 `642,794 → 429,498` instances로 감소했다. Prototype `14,425 → 9,558`까지 포함한 전체는 `657,219 → 439,056`, 즉 `218,163 / 33.19%` 감소했다. 같은 전체의 `instance × LOD0 triangle` 비용 추정치는 `277,007,269 → 147,103,423`, 즉 `46.90%` 감소했다.

Mask, 군락 영역, Ground Trace, Water exclusion, seed, Young Alder, Collision/Shadow/Cull 정책은 유지했다. 모든 생산 Actor는 `Generate On Demand`이고 Editor에서 생성한 ISM을 Level에 저장하므로 packaged runtime에서 PCG를 재생성하지 않는다. 실제 드론 비행 Frame Time과 masked overdraw는 아직 측정하지 않았으므로 이 수치를 실시간 성능 향상률로 해석하지 않는다.

## 22. River Surface V38 Short-Gap Repair

`ENV_RiverSurface_Production_X00_Y00`에서 보인 단절은 타일 clipping이나 Material이 아니라 입력 `Daejeon_SatelliteExtent_VisualMatch_Water_Area.geojson`의 River Area 폴리곤 사이에 남은 짧은 공백이었다. 같은 VisualMatch `waterway=river` 중심선이 양쪽 유지 폴리곤을 연속 통과하는지 전역 검사했고, `97.5m` 미만 공백 네 곳만 보수적으로 연결했다.

- 대상 타일: `X00_Y00`, `X04_Y04`
- 연결 공백: `4`곳 (`X00_Y00` 2, `X04_Y04` 2)
- Bridge 폭: 원본 mask `3 pixel`
- 추가 mask cell: `23` (`X00_Y00` 10, `X04_Y04` 13)
- 연결 component: `24 → 20`
- Mesh: V38 두 타일만 교체, 나머지 18타일 V23 유지
- Material: 20타일 모두 기존 V24 per-tile Directional Flow Material Instance와 Flow Map 유지
- Runtime 정책: Collision Off, Shadow Off, Nanite Off, Editor-authored Static Mesh

긴 공백이나 중심선 근거가 없는 Water Area는 임의로 연결하지 않는다. V38은 좌표계·전체 scale·actor transform을 바꾸지 않고, 위성 해상도에서 입증 가능한 짧은 입력 누락만 보정한다.

## 23. V46 Riparian Coverage and Global Forest Suitability

`PCG_ENV_RiparianLowerLayer_Y01_X03`은 검증된 V45의 point 위치, bank mask, sampling density, water exclusion, Z scale과 instance 수를 유지하면서 Grass Mesh의 XY footprint만 `1.42배` 확대한다. 면적 기준 피복 proxy는 `1.42² = 2.0164배`다. 이 변경은 새 Actor나 추가 Grass Instance를 만들지 않으며 저장 Graph는 `/Game/Environment/PCG/Graphs/PCG_ENV_RiparianUltraLushPrototypeV46`이다.

산림은 V35에서 단일 셀로 검증한 suitability 분기를 생산 Graph `/Game/Environment/PCG/Graphs/PCG_ENV_ForestHighSuitabilityProductionV46`으로 승격했다. 공용 Forest mask와 Water exclusion을 통과한 후보를 suitability density `0.65`에서 분기해 일반 구역은 기존 `15m`, 고적합도 구역은 `12m` 중심 간격을 사용한다. `PCG_ENV_Forest_Y00_X00`은 정안저수지 전용 V36 수면 제외가 있으므로 기존 `/Game/Environment/PCG/Graphs/PCG_ENV_ForestHighSuitabilityOpenWaterPrototype`을 유지한다. 나머지 `63`개 Forest Actor가 공용 V46 Graph를 사용한다.

두 시스템 모두 `Generate On Demand`로 Editor에서 생성한 ISM을 Level에 저장한다. Grass는 Collision/Shadow Off, Tree는 Collision Off/Shadow On 정책을 유지한다. Runtime PCG, C++, Blueprint 또는 외부 네트워크 의존성은 추가하지 않았다.

## 24. V47 Local River Contact and Taller Riparian Prototype

`X03_Y01`의 수면은 V42에서 이미 Landscape raycast를 한 단계 더 세분화한 Static Mesh다. 남은 검은 경계 틈은 입력 좌표나 Mesh 접지 실패가 아니라 V42가 보수적으로 남긴 약 `69.32cm` 중앙 clearance의 시각적 결과였다. 따라서 Mesh를 다시 생성하거나 전체 River를 내리지 않고 해당 Actor의 Z만 `-35cm`로 조정했다. 예측 contact gap은 최소 `-10cm`, p01 `19.79cm`, 중앙 `34.32cm`이며 다른 River Actor, Material, Flow Map, Collision/Shadow 정책은 바뀌지 않는다.

강변 하층은 V46 Graph를 복제한 `/Game/Environment/PCG/Graphs/PCG_ENV_RiparianUltraLushTallPrototypeV47`을 사용한다. V46의 point 위치, bank mask, water exclusion, sampling, seed, culling과 `176,583`개 instance를 그대로 두고 Grass transform의 XY만 `1.15배`, Z만 `1.20배` 확장한다. 별도 Actor, C++, Blueprint, runtime generation 또는 외부 의존성은 추가하지 않는다.

## 25. Global Riparian Production V49

생산 River Actor `20`개마다 LowerLayer PCG Actor를 하나씩 둔다. 승인 기준인 `Y01_X03`은 V47을 보존하고 다른 `19`개 셀은 전역 River mask와 공용 V49 Graph를 사용한다. 일반 셀 `18`개는 `PCG_ENV_RiparianRiverBankProductionV49`, 최대 셀 `X04_Y04`는 같은 규칙의 `PCG_ENV_RiparianRiverBankProductionBudgetV49`를 사용한다. 모든 결과는 Generate On Demand로 Editor에서 ISM으로 생성해 Level에 저장한다. 상세 구조와 셀별 수량은 `Docs/PCG_RIPARIAN_GLOBAL_V49.md`에 기록한다.

## 26. Conditional Far-Bank Silhouette V50

V50은 V49 근거리 계층을 교체하지 않는 조건부 LOD 성격의 보조 PCG 계층이다. River bounds 중심에서 기존 Grass 종료 Cull `700m`를 넘는 세 셀과 전역 연속성 누락이 확인된 한 셀에만 배치한다. 공용 Graph 하나와 Sampling Subgraph 하나를 네 Actor가 공유한다.

`PCG_ENV_RiparianFarBankSamplingV50`은 후보를 `0.015 point/m²`만 만든다. 공용 V49 bank mask의 density `0.60` 이상만 유지해 10–60m bank band에 국한하고, V49 Water hard exclusion과 Landscape Ground Trace를 그대로 통과시킨다. Dense/Accent Grass만 큰 XY scale과 `1.8–2.2km` Cull로 저장하며 Young Alder는 생성하지 않는다. 결과는 `73,764` ISM으로 기존 강변 전체의 `4.3484%`다. 전체 V49 Cull 확장과 합본 Mesh를 사용하지 않은 근거와 검증은 `Docs/PCG_RIPARIAN_FAR_BANK_V50.md`에 기록한다.

## 27. X02-Only Dense Far-Bank V51

공용 V50은 `X03_Y01`, `X04_Y04`, `X07_Y05`에서 그대로 유지한다. 넓은 bank 때문에 공용 `0.015 point/m²`가 성겨 보인 `X02_Y01`만 `/Game/Environment/PCG/Graphs/PCG_ENV_RiparianFarBankDenseX02V51`을 사용한다.

전용 Graph는 공용 V50의 bank/water/slope/grounding/scale/cull 구조를 그대로 복제하고 Sampling reference만 `/Game/Environment/PCG/Subgraphs/PCG_ENV_RiparianFarBankDenseSamplingV51`로 교체한다. Sampling은 `0.06 point/m²`다. 이 구조는 전역 V50을 무겁게 만들거나 합본 Mesh·Runtime PCG를 추가하지 않고, 문제가 확인된 한 셀에만 고밀도 비용을 한정한다.

`X02_Y01` FarBank는 `7,296 → 29,563`, 전체 FarBank는 `73,764 → 96,031`로 증가했다. 기존 LowerLayer `20 / 1,696,360`과 Forest `64 / 475,313`은 변경하지 않았다. 상세 근거는 `Docs/PCG_RIPARIAN_FAR_BANK_DENSE_X02_V51.md`를 참조한다.

## 28. Unified Riparian Bank V52

V52는 V50/V51의 `FarBank`를 별도 거리 LOD로 취급하지 않는다. ISM의 Start Cull Distance가 최소 표시 거리가 아니라 fade 시작 거리라서 가까운 시점에도 기존 LowerLayer와 동시 렌더링된다는 점을 확인했고, 네 중복 Actor를 저장 맵에서 제거했다.

`X02_Y01`, `X03_Y01`, `X04_Y04`, `X07_Y05`는 각각 하나의 `PCG_ENV_RiparianLowerLayer_*` Actor만 유지한다. 세 일반 셀은 `/Game/Environment/PCG/Graphs/PCG_ENV_RiparianUnifiedBankV52`, 넓은 `X02_Y01`은 `/Game/Environment/PCG/Graphs/PCG_ENV_RiparianUnifiedBankDenseX02V52`을 사용한다. 공용 V49 Bank/Water hard exclusion과 Landscape Ground Trace를 유지하며 Grass 8종만 ISM으로 저장한다.

다른 16개 LowerLayer와 Forest 64개 Actor는 재생성하지 않았다. 생산 강변 Actor 구조는 다시 River 20셀 : LowerLayer 20셀의 1:1 구성이며 `FarBank` 생산 Actor는 0개다. 상세 구조와 검증은 `Docs/PCG_RIPARIAN_UNIFIED_BANK_V52.md`를 참조한다.

## 29. Production Vegetation Rendering Policy V55

V55는 PCG 위치나 수량을 다시 생성하지 않고, 저장된 ISM Component와 현재 사용 중인 다섯 PCG Graph의 `FISMComponentDescriptor`에 같은 렌더 정책을 기록한다. 따라서 현재 화면의 배치와 향후 수동 재생성 결과가 같은 정책을 사용한다.

- Forest tree `475,313` instances: Start/End Cull `3,500/5,000m`
- Forest/young alder WPO disable distance: `1,000m`
- Riparian grass `1,453,896` instances: 기존 Cull 유지, WPO disable distance `250m`
- Riparian grass: `Visible in Ray Tracing = False`
- Forest tree와 young alder: `Visible in Ray Tracing = True` 유지
- Collision, overlap, shadow, distance-field lighting, instance transform/count: 변경 없음

나무는 Nanite를 유지하므로 최종 fade 전까지 화면 크기에 따른 geometry simplification을 엔진이 처리한다. Grass는 가까운 드론 시점에서 기존 material WPO를 유지하되, 움직임을 식별하기 어려운 250m 밖에서는 WPO 평가를 중단한다. Hardware Ray Tracing 장면에서는 non-Nanite Grass 145만 개를 제외하고, 일반 raster main pass는 그대로 유지한다. 이는 반사·간접광 속 Grass 정밀도보다 RT acceleration structure 비용을 줄이는 쪽을 선택한 정책이다.

Fresh-load에서 UE 5.7.4의 기존 Ray Tracing instance culling도 실제 활성 상태임을 확인했다. `r.RayTracing.Geometry.InstancedStaticMeshes.Culling=1`, cluster radius `100m`, low-scale radius `10m`, 전역 `r.RayTracing.Culling=3`, per-instance culling `1`, radius `300m`이다. 따라서 Tree용 추가 RT cvar override는 중복되거나 과도한 pop-in을 만들 가능성이 있어 넣지 않는다.

현재 Tree material은 `/ProceduralVegetationEditor/SampleAssets/Materials/MasterMaterials/MA_Foliage_Trees` 기반이며, 저장된 Material Instance에 확인 가능한 wind parameter가 없다. `DynamicWind`는 현재 Static Mesh ISM tree 경로에 적용되어 있지 않다. 전역 Tree wind는 검증 없이 활성화하지 않으며, project-local leaf-only WPO material을 한 셀에서 시험한 뒤 1,000m WPO 제한과 GPU Frame Time을 함께 통과할 때만 생산 승격한다.

## 30. Y00_X02 Local Riparian Density V56

`PCG_ENV_RiparianLowerLayer_Y00_X02`는 공용 V49 bank/water/grounding 규칙을 유지하면서 Sampling만 전용 `PCG_ENV_RiparianGroundCoverSamplingDenseY00X02V56`으로 분리한다. 공용 `0.10 point/m²`를 해당 셀에서만 `0.20 point/m²`로 올렸고, bank mask 폭과 scale·collision·shadow·cull·WPO·Ray Tracing 정책은 바꾸지 않았다.

Grass는 `31,070 → 62,164`, Young Alder는 `87 → 133`이다. 다른 19개 LowerLayer, Forest 64개, River 20개는 exact unchanged다. 전체 LowerLayer는 `1,487,653` instances이며, 추가 비용은 기존 LowerLayer 대비 `31,140 / 2.14%`다.

## 31. X02_Y01 Main-Channel Flow Axis V57

`ENV_RiverSurface_Production_X02_Y01`의 V24 Flow Map에는 서로 분리된 두 수로가 있다. 작은 수로의 벡터는 정상이나 큰 본류는 외부 중심선의 최근접 선분을 잘못 선택해 물길 장축과 거의 직각인 방향을 사용했다. V57은 공용 Parent Material, 속도, UV registration, River Mesh와 Actor Transform을 바꾸지 않고 큰 본류 sampling component의 `(x, y)` 벡터만 `(-y, x)`로 90도 회전한다.

- Texture: `/Game/Environment/River/Textures/Production/FlowV57/T_ENV_RiverFlow_Daejeon_V57_X02_Y01`
- Material Instance: `/Game/Environment/River/Materials/Instances/FlowV57/MI_ENV_RiverSurface_DirectionalFlow_V57_X02_Y01`
- Parent: 기존 `/Game/Environment/River/Materials/M_ENV_RiverSurface_DirectionalFlow_V24`
- 적용 Actor: `ENV_RiverSurface_Production_X02_Y01` 한 개
- Water/Sampling mask: V24와 pixel-exact 동일
- 다른 19개 River Actor: 기존 V24 Material Instance 유지

이 수정은 새 Shader branch나 runtime 계산을 추가하지 않는다. 동일 해상도 256×256 Flow Texture 한 장과 기존 Material Instance 복제본 한 장만 추가하므로 runtime rendering 구조와 비용은 사실상 유지된다.

## 32. X02_Y00 Single-Component Long-Axis Flow V58

사용자가 표시한 화면은 저장된 Editor viewport의 Camera transform과 수면 평면 교차점을 역산해 `ENV_RiverSurface_Production_X02_Y00`으로 식별했다. 따라서 V57의 `X02_Y01` 교정은 유효한 별도 본류 보정으로 유지하고, 실제 표시 구간인 `X02_Y00`에는 독립적인 V58 예외를 적용한다.

`X02_Y00` Flow Map은 하나의 연속 수로 component다. V58은 이 component의 PCA 장축 `(-0.13526456, 0.99080952)`를 구하고 기존 평균 부호를 유지한 채 sampling pixel의 RG 벡터만 해당 장축으로 통일한다. B Water mask, A Sampling mask, 비-sampling RG, River Mesh, Actor Transform, UV registration과 속도는 변경하지 않는다.

- Texture: `/Game/Environment/River/Textures/Production/FlowV58/T_ENV_RiverFlow_Daejeon_V58_X02_Y00`
- Material Instance: `/Game/Environment/River/Materials/Instances/FlowV58/MI_ENV_RiverSurface_DirectionalFlow_V58_X02_Y00`
- Parent: 기존 `/Game/Environment/River/Materials/M_ENV_RiverSurface_DirectionalFlow_V24`
- 적용 Actor: `ENV_RiverSurface_Production_X02_Y00` 한 개
- Material 분포: V24 `18`, V57 `1`, V58 `1`
- Runtime 구조: 기존 Opaque Static Mesh + Material Instance, Collision/Shadow Off

기존 `T_River_Waves01_Normals`과 Detail Normal, `PrimarySpeed=-0.024`, `DetailSpeed=-0.052`는 그대로 사용한다. 따라서 새 Texture sample, Shader branch, Actor, tick 또는 Runtime PCG를 추가하지 않으며 방향 데이터용 256×256 Texture와 MI 하나만 추가된다.

## 33. X02 Fixed-Axis Visible Motion V59

V57/V58은 Flow Map 벡터를 장축에 정렬했지만, 최종 화면에서는 `961×63` 비등방성 `T_River_Waves01_Normals`의 독립적인 Primary/Detail panning이 더 강하게 보였다. 따라서 Flow Map만 반복 회전하는 방식은 중단하고, 사용자가 표시한 `X02_Y00`과 인접 `X02_Y01`에만 화면상 흐름을 결정하는 단일 world-space 축을 명시했다.

- Parent: `/Game/Environment/River/Materials/M_ENV_RiverSurface_FixedAxisMotion_V59`
- Instances: `/Game/Environment/River/Materials/Instances/FlowV59/MI_ENV_RiverSurface_FixedAxisMotion_V59_X02_Y00`, `..._X02_Y01`
- 적용 Actor: `ENV_RiverSurface_Production_X02_Y00`, `ENV_RiverSurface_Production_X02_Y01`
- 장축: World XY `(-0.10452846, 0.99452190)`
- 다른 River Actor: `18`개 모두 기존 V24 Parent 유지

V59은 V24 Parent를 복제하고 두 대상 MI의 Primary/Detail normal panning과 기존 moving highlight만 정지한다. 기존 Normal Texture와 Flow Map은 그대로 보존한다. 가시적 이동 신호는 다음 한 경로로 제한한다.

`WorldPosition.XY · LongitudinalAxis → Tiling → Time × Speed → Sine → BaseColor/Roughness 저강도 Lerp`

새 Texture sample, Dynamic Branch, Actor, Tick, Blueprint 또는 Runtime PCG는 추가하지 않는다. Parent expression은 `87 → 111`개지만 추가 비용은 world-position projection, 산술, Sine와 두 Lerp의 ALU이며 두 수면 Actor에만 한정된다. River Mesh/Transform, UV registration, Collision/Shadow, Riparian/Forest PCG는 변경하지 않았다.

`LongitudinalPhaseOffset`은 headless Editor에서 방향을 결정론적으로 검증하기 위한 진단 파라미터다. 생산값은 항상 `0`이고 시간 진행은 `LongitudinalSpeed` 하나만 사용한다.

## 34. Global Local-Tangent River Flow V60

V60은 V57/V58/V59의 타일별 예외를 현재 생산 경로에서 대체한다. 전역 감사 결과 V24 Flow Map은 각 water pixel의 국소 수로 형상이 아니라 외부 centerline의 최근접 선분 tangent를 사용했고 smoothing도 세 번의 소규모 반복에 그쳤다. 이 때문에 분기나 굴곡에서 다른 선분을 선택하거나, 이전 진행 방향을 다음 곡선까지 끌고 가는 현상이 발생했다. 사용자 제안대로 변곡점에서 국소 방향을 갱신하지 못한 것이 핵심 원인이었다.

V60 생성기는 각 타일의 연결된 water mask 자체에서 반경 `14/24/36px`의 bounded local PCA tangent를 계산한다. 형상만으로는 진행 부호를 알 수 없으므로 기존 Flow Map과의 dot product로 부호를 고정해 갑작스러운 역전을 방지한다. Water mask(B)와 Sampling mask(A), World UV registration, 속도, River Mesh와 Actor Transform은 변경하지 않는다.

- Texture: `/Game/Environment/River/Textures/Production/FlowV60/T_ENV_RiverFlow_Daejeon_V60_X##_Y##` 20개
- Material Instance: `/Game/Environment/River/Materials/Instances/FlowV60/MI_ENV_RiverSurface_DirectionalFlow_V60_X##_Y##` 20개
- Parent: 기존 `/Game/Environment/River/Materials/M_ENV_RiverSurface_DirectionalFlow_V24`
- 적용 Actor: `ENV_RiverSurface_Production_*` 20개 전부
- V24 Parent expressions: `87`, 변경 없음
- 새 Parent, Texture sample, Actor, Tick, Blueprint, Runtime PCG: 추가 없음

결과적으로 X02 두 타일에만 고정 world axis를 적용하던 V59은 활성 맵 참조에서 제외됐다. 다만 V60은 입력 tangent의 국소 정확도만 개선했고, 최종 화면에서 더 강하게 보이던 비등방성 Primary/Detail normal panning은 V24 Parent에 그대로 남았다. 따라서 V60의 RG 벡터 감사 통과는 입력 데이터 검증일 뿐, 실제 보이는 흐름 방향의 수정 완료를 의미하지 않는다. V60은 현재 V61의 방향 원본으로만 사용하며 활성 Level Material은 아니다.

## 35. Global Longitudinal Phase River Flow V61

V61은 V60에서 확인된 국소 tangent를 시간에 직접 곱하지 않는다. 20개 타일을 하나의 연결된 sampling domain으로 합쳐 각 연결 성분에서 `∇phase`가 V60 tangent와 일치하도록 전역 위상장을 적분하고, 그 결과를 `R=cos(phase)`, `G=sin(phase)`, `B/A=기존 water/sampling mask`로 저장한다. Material은 이 위상에 `Time × LongitudinalSpeed`만 더하므로 굴곡 위치마다 위상 기울기가 즉시 바뀌며, 타일 경계에서도 같은 연결 성분의 진행이 이어진다.

- Phase Texture: `/Game/Environment/River/Textures/Production/FlowV61/T_ENV_RiverFlowPhase_Daejeon_V61_X##_Y##` 20개
- Parent Material: `/Game/Environment/River/Materials/M_ENV_RiverSurface_LongitudinalPhase_V61`
- Material Instance: `/Game/Environment/River/Materials/Instances/FlowV61/MI_ENV_RiverSurface_LongitudinalPhase_V61_X##_Y##` 20개
- 적용 Actor: `ENV_RiverSurface_Production_*` 20개 전부
- 방향 원본: V60 RG, read-only
- Static normal: `/Game/WaterMaterials/Textures/T_Water_Normal_Subtle` 두 정적 scale
- Texture sample: V24 `5`개에서 V61 `3`개로 감소
- Animated normal sample: `0`
- Parent expressions: `56`
- Rendering: Opaque, WPO 없음, Collision/Shadow Off
- Runtime Actor/Tick/PCG/GIS/network: 없음

화면상 횡방향을 지배하던 `T_River_Waves01_Normals`의 animated panning은 생산 경로에서 제거했다. 표면의 미세 질감은 두 정적 isotropic normal scale로 유지하고, 움직이는 신호는 위상장 기반 BaseColor/Roughness 저강도 변조 한 경로로 제한한다. 이 선택은 물결 디테일 일부를 줄이는 대신 방향 오판 가능성과 Texture sample 비용을 함께 낮춘다.

오프라인 위상 감사에서 sampling pixel `75,239`, 연결 성분 `16`, 위상 gradient의 V60 tangent 평균 오차 `1.866°`, P90 `3.616°`, 정방향 비율 `99.943%`, 30도 이내 축 정렬 `99.572%`를 확인했다. 저장 맵을 별도 UE 프로세스에서 다시 읽은 X02 결정론적 8-frame 검사에서는 화면 장축 신호가 횡방향보다 생산 설정 `3.615×`, 진단 설정 `3.271×` 우세했다. 이 진단은 Phase Offset만 일시 변경하고 맵과 Material Asset을 저장하지 않으며, 종료 시 생산값으로 복구한다.

## 36. Targeted River Surface Contact V62

V62는 전체 수면 생성기를 다시 배포하지 않고 사용자가 표시한 네 구역만 수정한다. `X03_Y00`의 source mask 내부에 남아 있던 단일 `30m` 셀을 채운 뒤 V23과 동일한 Taubin smoothing을 재적용했다. 이 수정은 active cell `1157 → 1158`, triangle `9256 → 9264`의 한 셀 변화이며, 나머지 수면 평면 형상은 유지한다.

`X02_Y00`, `X02_Y01`, `X03_Y01`은 accepted V23 XY topology를 두 번 세분화해 약 `3.75m` terrain-contact resolution으로 Landscape 높이를 다시 굽는다. 세 구역이 서로 다른 baked clearance를 갖지 않도록 계산된 최대 필요값 `28.8358cm`를 공통 적용한다. 따라서 `X02_Y01–X03_Y01` 경계의 공유 refined point `178`개는 world-space 수면 높이가 정확히 일치한다. Actor XY, Material, Collision/Shadow 정책은 보존하며 세 grounded Actor Z는 `0`이다.

- Mesh folder: `/Game/Environment/River/Production/Meshes/V62TargetedContact`
- Changed River Actors: `X02_Y00`, `X03_Y00`, `X02_Y01`, `X03_Y01`
- Unchanged River Actors: 나머지 `16/16`
- PCG Graph/Actor/instance mutation: 없음
- Flow direction/phase data: 불변
- `FlowV61` 속도만 전역 `-0.16 → -0.14 cycle/s`

이 구조는 런타임 trace나 deformation을 추가하지 않는다. 결과는 Editor에서 생성·저장된 Static Mesh 네 개와 기존 Material Instance scalar 변경으로만 구성된다.

## 37. X02_Y00–X03_Y00 Seam Continuity V63/V64

V62는 `X02_Y00`, `X02_Y01`, `X03_Y01`만 공통 `28.8358cm` clearance로 재접지하고 `X03_Y00`에는 이전 유효 clearance `175cm`를 유지했다. V62 검증도 Y01 경계만 검사했기 때문에, X02_Y00–X03_Y00의 exact shared XY/UV 경계가 world Z에서 `146.1642cm` 어긋나는 회귀를 발견하지 못했다. V63은 인접 관계를 전수 확인한 뒤 직접 이웃이 `X02_Y00` 하나뿐인 `X03_Y00` Mesh만 같은 terrain-contact 정책으로 다시 만들었다.

- Mesh: `/Game/Environment/River/Production/Meshes/V63SeamContinuous/SM_ENV_RiverSurface_Daejeon_v63_X03_Y00`
- Refined geometry: `76,641` vertices, `148,224` triangles
- Shared refined boundary: `65` points, maximum XY/UV/Z delta `0/0/0`
- Sampled Landscape gap: minimum `15.3592cm`, median `28.8358cm`
- Actor Z: `X02_Y00`, `X03_Y00` 모두 `0`

지오메트리를 맞춘 뒤에도 top-down에서 직선형 명암 경계가 남았다. V61 `FlowMapPhase`는 타일별 `256×256` Clamp Texture이며 같은 world 경계에서 X02의 마지막 texel과 X03의 첫 texel을 각각 읽었다. 해당 active water row의 phase 차이는 평균 `43.008°`, 최대 `49.248°`였다. 한쪽에만 gutter를 둔 시험은 X03만 이웃과 평균화해 여전히 서로 다른 값을 만들었으므로 생산 최종 방식으로 채택하지 않았다.

최종 V64는 양쪽 Texture에 reciprocal 1px gutter를 둔다. X02의 right gutter는 X03 원본 첫 열, X03의 left gutter는 X02 원본 마지막 열이다. 두 MI 모두 `new_uv = old_uv × (256/258) + 1/258`을 사용해 공유 경계가 X02 `256.5`, X03 `0.5` texel coordinate를 보게 한다. 따라서 bilinear filter는 양쪽에서 동일한 두 픽셀의 평균을 반환한다.

- X02 Texture/MI: `FlowV64PairedGutter/T_ENV_RiverFlowPhase_Daejeon_V64_Gutter_X02_Y00`, `FlowV64PairedGutter/MI_ENV_RiverSurface_LongitudinalPhase_V64_Gutter_X02_Y00`
- X03 Texture/MI: 기존 `FlowV63SeamGutter/T_ENV_RiverFlowPhase_Daejeon_V63_Gutter_X03_Y00`, `FlowV63SeamGutter/MI_ENV_RiverSurface_LongitudinalPhase_V63_Gutter_X03_Y00`
- Parent: V61 exact retained
- Interior `256×256` phase data: 양쪽 pixel-exact retained
- 추가 runtime Texture sample/Shader branch/Draw call/Actor/Tick/PCG: `0`
- Runtime에 사용되는 두 phase texture의 source pixel 증가: 합계 `2,056`, 각 `1.5686%`

이 보정은 전역 Parent shader를 복잡하게 만들지 않고 문제 경계 두 MI에만 한정한다. 이후 타일 접합 검증은 공유 XY/UV뿐 아니라 world Z와 Clamp Texture의 bilinear edge sample까지 함께 검사해야 한다.

## 38. Global River Contact V65

### 38.1 문제의 구조적 원인

V23 River mesh 생성기는 Landscape 위에 약 `100 cm`의 clearance를 mesh vertex에 미리 포함했고, Level의 River Actor는 다시 `Z = +75 cm`로 배치되었다. 따라서 초기 20개 River tile은 명목상 Landscape보다 약 `175 cm` 높은 구조였다.

V62/V63에서 `X02_Y00`, `X02_Y01`, `X03_Y00`, `X03_Y01` 네 tile만 별도의 접지 mesh와 `Actor Z = 0`으로 교체되면서, 수정된 네 tile과 기존 16개 tile이 서로 다른 접지 기준을 사용하게 되었다. V65는 이 부분 수정 상태를 제거하고 20개 River tile 전체에 동일한 접지 규칙을 적용한다.

### 38.2 V65 정적 접지 방식

V65 production mesh는 `/Game/Environment/River/Production/Meshes/V65GlobalContact`에 저장된다.

- 20개 source River mesh를 각각 1회 subdivision한다.
- 공통 기본 clearance는 `28.835796991983898 cm`이다.
- 모든 삼각형 중심에서 Landscape와의 간격을 검사한다.
- 삼각형 중심 간격이 `8 cm` 미만인 경우에만 해당 삼각형의 incident vertex에 국소 lift를 적용한다.
- tile 경계에서 같은 world XY를 공유하는 vertex는 동일한 lift 값을 사용한다.
- River Actor의 XY, rotation, scale은 보존하고 Z는 전부 `0`으로 통일한다.

이 방식은 Landscape 형상을 Static Mesh에 bake하므로 runtime trace, Tick, Blueprint 보정, World Position Offset 기반 지형 추종을 추가하지 않는다. 정적 환경과 offline 패키징 요구에 맞는 최소 구조이다.

### 38.3 보존된 시스템

- 기존 River Actor 20개의 이름과 Level 배치는 유지한다.
- 기존 material assignment와 flow parameter는 유지한다.
- `LongitudinalSpeed`는 모든 tile에서 `-0.14`로 유지한다.
- collision과 shadow는 기존 production 정책대로 비활성화한다.
- Forest/Riparian PCG Actor와 instance data는 변경하지 않는다.
- 이전 River mesh asset은 삭제하지 않고 rollback용으로 보존한다.

### 38.4 성능 특성

V65의 총 LOD0 triangle 수는 `965,632`이다. 직전 production의 `1,163,608`보다 `197,976`개, 약 `17.01%` 감소했다.

전체 mesh를 2회 subdivision하는 단순 방식은 약 `3,862,528` triangle이 필요하므로 채택하지 않았다. V65는 1회 subdivision과 문제 지점의 국소 lift만 사용해 접지 안정성과 geometry 비용을 함께 관리한다.

## 39. River Topology V66 / Shared Phase Atlas V67

V66은 V65의 접지 방식을 유지하면서 source-raster topology만 정리한다. 기존 생성기가 8-neighbor 연결을 허용해 모서리 한 점으로만 닿은 셀도 하나의 강으로 취급했고, 작은 폐공과 약한 OSM 지류가 그대로 메시가 되어 점·홀·사슬 형태를 만들었다.

- OSM `968820568`, `968820569`, `968820570`: 위성 근거가 약하고 `X04_Y04`의 꼬인 사슬을 구성하므로 완전 제외
- OSM `530929919`: 사용자가 표시한 북쪽 분리 component만 제외하고 남쪽 component는 유지
- 면적 8 source-cell 이하의 폐공: 19 cells를 수면으로 채움
- diagonal-only point contact: 인접 support cell 55개를 추가해 edge contact로 변환
- 폭이 있는 Geum-river island: 47 cells, bbox `[172,582,184,589]`를 의도된 육지로 유지

그 결과 4-neighbor/8-neighbor component 수가 모두 19개가 되었고 diagonal-only contact는 0개다. V66 mesh는 V65와 같은 Landscape bake, `28.835796991983898cm` 기본 clearance, triangle-center 최소 gap `8cm`, Actor Z `0` 규칙을 사용한다. 활성 경로는 `/Game/Environment/River/Production/Meshes/V66TopologyClean`이다.

V67은 `X02_Y00–X03_Y00`의 남은 직선형 명암선을 geometry가 아닌 phase sampling seam으로 확정한 뒤 두 타일을 하나의 `514×258` atlas로 합쳤다. 두 MI는 공유 경계를 동일한 normalized U `0.5000000117956718`에서 읽는다. Parent Material, texture sample 수, draw call, flow speed는 바꾸지 않는다.

`PCG_EXCL_Manual_Central_01`은 삭제 대상이 아니다. 현재 생산 PCG에서 reachable한 7개 graph가 `PCG_Exclude_Vegetation` tag를 `PCGGetVolumeSettings`로 직접 조회한다. Actor 자체는 렌더 geometry나 instance를 만들지 않는 `PCGVolume`이며 중앙 `400m × 400m` 수동 vegetation exclusion을 제공한다. 삭제하면 해당 영역이 다음 PCG 재생성 때 다시 식생 후보가 될 수 있다.

## 40. River Connectivity V68

V68은 V66/V67의 topology 정리, 전역 Landscape 접지, 공유 phase atlas를 유지하면서 수면이 실제 수계 안에서 드문드문 끊겨 보이던 구간만 연결한다. V66은 작은 hole과 diagonal-only point contact를 제거하는 데 목적이 있었기 때문에 서로 다른 4-neighbor component 사이의 간격은 의도적으로 손대지 않았다. 그런데 원본 OSM water-area 데이터에는 하나의 수계가 여러 polygon으로 나뉘거나 폭이 `30m` source raster 한 셀보다 좁아지는 지점이 있어, rasterization 이후 같은 강이 별도 component로 남았다.

연결 판단은 화면상의 거리만으로 하지 않는다. 다음 근거 중 하나를 만족하는 후보만 승인한다.

- 동일한 authoritative centerline network에 속하고 서로 가장 가까운 component를 연결하는 minimum-spanning edge
- 동일한 OSM water-area feature 안에서 끊어진 구간
- source-area corridor가 간격을 관통하며 기존 topology의 짧은 fracture로 확인된 구간
- centerline이 없는 경우에도 방향과 폭이 일치하는 매우 짧은 collinear water-area chain

승인된 15개 bridge는 기존 component 사이의 최단 corridor를 source-cell 단위로 rasterize한다. 하나의 component pair에 여러 bridge를 중복 생성하지 않으며, 이미 연결된 graph에는 추가 edge를 넣지 않는다. 그래서 외형을 메우기 위해 별도 Actor, decal, spline, runtime Tick 또는 Runtime PCG를 추가하지 않는다.

의도적으로 하나로 합치지 않은 수계도 있다.

- `X00_Y02`의 사용자 제거 대상이었던 북쪽 Jemin fragment
- `X05_Y00`의 독립된 짧은 도시 수로 계통; 내부의 collinear 조각만 서로 연결
- `X07_Y04/Y05`의 Geum 계통; 비활성 River tile과 Daecheong reservoir를 가로지르는 추정 bridge는 생성하지 않음

최종 source topology는 `30,052 → 32,210` water cells, 4-neighbor/8-neighbor component `19/19 → 4/4`, diagonal-only contact `0`이다. 추가된 `2,158` cells는 기존 육지를 임의로 수면화한 전역 dilation이 아니라 위 15개 근거 기반 corridor의 합이다.

V68 production mesh는 `/Game/Environment/River/Production/Meshes/V68Connectivity`에 저장된다. 20개 활성 River tile 전체를 V65/V66과 같은 방식으로 1회 subdivide하고 Landscape에 bake한다. 공통 기본 clearance `28.835796991983898cm`, triangle-center 최소 gap `8cm`, shared-boundary 동일 높이, Actor Z `0`, Collision/Shadow Off 정책을 유지한다. 전체 LOD0 triangle은 `961,664 → 1,030,720`으로 `69,056`개(`7.180886%`) 증가했으며, 새로운 runtime 시스템이나 draw-call 단위는 추가하지 않는다.

## 41. River Cleanup and Dense Contact V69

V69은 V68 구조를 교체하지 않고 두 가지 확인된 문제만 수정한다.

첫째, `X00_Y02`에 남아 있던 `67` source-cell component는 V68에서 의도적으로 보존된 독립 수로였다. bbox `[130,660,152,694]`, 면적 약 `60,300m²`이며 주 수계와 연결되지 않는다. 사용자가 연결보다 삭제를 선택했으므로 이 component의 cell만 제거했다. 다른 V68 cell은 추가하거나 삭제하지 않았다. 최종 water cell은 `32,210 → 32,143`, 4-neighbor/8-neighbor component는 `4/4 → 3/3`이다.

둘째, `X04_Y04`의 바닥 노출은 water mask hole이나 non-manifold mesh가 아니었다. 해당 tile은 enclosed hole `0`, degenerate triangle `0`, non-manifold edge `0`이었지만 V68 접지 계획기가 refined triangle의 중심만 검사했다. Landscape의 bilinear surface가 triangle vertex/center 사이에서 더 높아질 수 있어, V68 실제 mesh의 quarter-grid 표본에서 최소 gap `-13.1189cm`, water보다 높은 Landscape 표본 `396`개가 발견됐다.

V69은 기존 1회 subdivision과 공통 clearance를 유지하면서 `X04_Y04`에만 denominator `4`의 barycentric dense contact 검사를 추가한다. 각 triangle의 꼭짓점을 제외한 내부·edge 표본 `12`개를 Landscape에 trace하고, `8cm` 미만 표본이 있는 triangle의 세 vertex에 필요한 최소 local lift를 적용한다. 같은 world XY를 공유하는 tile 경계 vertex는 기존 방식대로 최대 lift를 동기화한다. 이 계산은 Editor에서 Static Mesh Z에 bake되며 runtime trace, WPO, Tick 또는 Blueprint는 추가하지 않는다.

- Active mesh folder: `/Game/Environment/River/Production/Meshes/V69Cleanup`
- River Actors: 기존 `20`개 유지
- Actor transforms/material/flow: 유지
- Forest/Riparian PCG: graph, actor, instance 모두 유지
- Collision/Shadow/Nanite: 기존 production 정책대로 비활성
- LOD0 triangles: `1,030,720 → 1,028,576` (`-2,144`, `-0.2080%`)
- `X04_Y04` dense samples: `499,968`, unique Landscape traces `321,408`
- dense-contact final minimum gap: `7.99999957cm`; OBJ 정밀도 허용 오차 `0.01cm` 기준 통과
- shared cross-tile maximum height delta: `0cm`

### 41.1 추가 수로 후보 검토

현재 water-area mask 밖의 OSM `waterway=river/stream` centerline을 전역 비교한 결과 uncovered feature `146`개가 존재한다. 그러나 이 자료는 선 중심만 제공하며 생산 수면에 필요한 폭, bank extent, 안정적인 Landscape 접지 범위를 확정하지 않는다. 사용자 이미지의 넓은 공백도 여러 centerline 후보와 겹칠 수 있어, 특정 선을 임의 buffer하면 이전에 제거했던 사슬형 수면, 능선 횡단, 고립 fragment 또는 접지 문제를 다시 만들 수 있다.

따라서 V69 생산 맵에는 새 수로를 추가하지 않았다. 향후 추가 시에는 정확한 centerline feature 선택, 폭/양안 근거, longitudinal phase 생성, 전 구간 dense contact, 활성 production tile, 양안 Riparian mask/PCG 및 fixed-route 성능 측정을 하나의 bounded prototype에서 먼저 통과시켜야 한다. 단순한 전역 centerline buffer나 별도 spline Actor 복제는 production 경로로 사용하지 않는다.

## 42. River Water Exclusion / PCG Mask Sync V70

V70은 V69 구조를 교체하지 않고 사용자가 지정한 두 문제를 같은 수면 authority에서 해결한다.

첫째, `X00_Y02`의 폭이 있는 중앙 육지 hole은 V66에서 의도적으로 보존했던 `47` source-cell island였다. 사용자 확인 뒤 이 정확한 component만 수면으로 채웠다. 면적은 약 `42,300m²`, bbox는 `[172,582,184,589]`이며 다른 cell은 추가하거나 제거하지 않았다. 최종 water cells는 `32,143 → 32,190`, 4-neighbor/8-neighbor component는 `3/3`, enclosed hole과 diagonal-only contact는 각각 `0`이다.

둘째, `X04_Y04`를 포함한 수면 위 식생의 원인은 PCG graph 자체의 분포 노이즈가 아니라 spatial authority version drift였다. River mesh는 V69까지 갔지만 Lower Layer shared graph는 V40 water와 V49 bank mask를, Forest/기존 Riparian은 더 오래된 global water mask를 계속 읽고 있었다. 따라서 새 River Surface와 PCG exclusion의 경계가 달라져 수면 위 instance가 남았다.

V70은 기존 graph 이름과 참조 구조를 유지하고 texture 입력만 다음 세 자산으로 동기화한다.

- `/Game/Environment/PCG/Data/Mask/T_ENV_RiverWater_Daejeon_ProductionV70`: `6048×6048`, 약 `10m/pixel`; Lower Layer hard water exclusion
- `/Game/Environment/PCG/Data/Mask/T_ENV_RiverBankBand_Daejeon_ProductionV70`: `6048×6048`; 수면과 추가 1-pixel safety를 제외한 bank density
- `/Game/Environment/PCG/Data/Mask/T_ENV_WaterChannel_Daejeon_Extended_RiverV70`: `2017×2017`, 약 `30m/pixel`; Forest/기존 Riparian의 global water union

graph asset 이름의 `V40`/`V49`는 downstream reference를 깨지 않기 위해 유지한다. 실제 texture sampler 입력은 V70이며 transform, filter, absolute transform 등 texture 외 설정은 바꾸지 않는다. 새 graph, Actor, Runtime PCG, Tick, Blueprint 또는 외부 의존성은 추가하지 않았다.

PCG 재생성 범위는 20개 Lower Layer Actor 전부와 global water가 실제로 바뀐 Forest 4개/Riparian 2개, 총 `26`개다. 재생성 뒤 저장된 instance 총량은 Forest `475,182`, 기존 Riparian `35,467`, Lower Layer `1,623,247`이다. Lower Layer는 정확한 `10m` water mask로 판정하며 실제 수면 침범은 `0`이다. `30m` coarse cell 판정에서 보이는 `13`개는 10m 경계 안쪽의 육상 instance가 같은 coarse cell에 포함된 false positive라 삭제하지 않는다.

고해상도 texture를 사용하는 PCG mutation은 `RenderOffscreen` RHI에서 실행한다. `NullRHI`는 Lower Layer texture sampling 결과가 0개가 될 수 있으므로 read-only audit 전용이다. 저장 전에는 20개 Lower Layer가 모두 non-empty인지, 결정론적 총량이 위 기준값과 일치하는지, 세 계층의 유효 수면 침범이 모두 0인지 검사한다. 실패 복구는 고정 V40/V49 경로가 아니라 실행 시작 시점의 graph 입력을 캡처해 복원한다.

River Surface V70은 `/Game/Environment/River/Production/Meshes/V70WaterExclusion`의 20개 mesh를 사용한다. V69의 contact bake, shared boundary height, V61 flow material과 V67 shared phase atlas를 유지한다. 전체 LOD0 triangle은 `1,030,080`, Actor Z는 모두 `0`, Collision/Shadow는 모두 Off다.

### 42.1 주황색 신규 수로 사전 분석

현재 production river 생성은 단순히 JSON 선을 바로 그리는 방식이 아니다. 로컬 GeoJSON water area/centerline과 저장된 affine registration을 읽어 source raster를 만들고, satellite annotation·기존 mask·topology를 교차 검증한 뒤 Landscape 접지, flow phase, active tile, PCG bank mask를 함께 생성한다.

사용자 주황색 annotation의 주 component는 `24,066` source pixels, bbox `[1324,409,2017,1121]`이다. 후보 centerline 중 OSM `34834591`, `388811980`, `1524242671`이 실질 근거이며, annotation의 `69.82%`가 centerline 8-pixel 이내에서 지지된다. 그러나 source area polygon coverage는 `0.34%`이고 후보에 폭 tag가 없으므로 양안 폭을 authoritative source로 확정할 수 없다.

따라서 신규 수로는 V70 production에 적용하지 않았다. annotation과 GeoJSON을 결합해 원하는 route를 분리하는 것은 가능하지만, 폭·합류 접점·flow phase·접지·양안 PCG를 production 품질로 확정하려면 기존에 없던 10개 tile(`X05_Y01`, `X05_Y03`, `X05_Y04`, `X06_Y01`–`X06_Y04`, `X07_Y01`–`X07_Y03`)의 격리 prototype과 검증이 먼저 필요하다.

## 43. Approved Orange Route V71

사용자 승인 후 주황색 경로만 추가했다. V70 사전 검토의 10개 후보 타일 전체를 생성하지 않고, annotation corridor와 등록된 satellite water evidence 및 선택한 GeoJSON centerline을 교차한 최종 범위만 사용한다. 폭 tag/area polygon이 충분하지 않으므로 이는 시각적 재구성이며 측량 기반 수면·수리 모델이 아니다.

- 신규 River: `X05_Y01`, `X05_Y03`, `X06_Y01`, `X06_Y02`, `X06_Y03`, `X06_Y04`, `X07_Y03`.
- 연결부만 재생성: `X05_Y02`, `X07_Y04`. 나머지 18개 River의 mesh/material/transform은 보존한다.
- mesh 경로: `/Game/Environment/River/Production/Meshes/V71OrangeRoute`.
- 기존 V66 contact bake와 V61 longitudinal material을 재사용한다. 하나의 V71 phase atlas/MI를 공유하며 기존 V67 특수 seam pair는 변경하지 않는다.
- V71 최종 수면에서 6048² water/bank mask와 2017² global-water union을 함께 만든다. 기존 shared graph 이름은 유지하고 texture 참조만 V71로 갱신한다.
- 실제로 변한 9개 PCG 공간 셀의 Forest/Riparian/LowerLayer를 갱신하고, 없던 LowerLayer PCGVolume 7개만 추가한다. Runtime PCG나 새로운 Tick 시스템은 도입하지 않는다.
- 저장 결과는 River 27개, 전체 Actor 192개, 식생 3,384,931개다. 수면 내부 식생은 계층별 실제 입력 mask 기준 0개이며, 대상 외 PCG 수량은 변하지 않았다.

V71 자동화는 Slate callback 재진입을 막고, PCG component 참조를 완료까지 유지하며, cleanup 도중 발생하는 이전 generation event를 이번 완료로 오인하지 않도록 phase를 검사한다. 모든 대상은 cleanup 후 generate하며 빈 LowerLayer/수면 침범 결과는 저장하지 않는다.

상세 과정과 복구 경로는 작업 폴더의 `work/ORANGE_RIVER_V71.md`, `orange_river_v71_apply_report.json`, `orange_pcg_v71_generation.json`에 기록한다. V70은 복구 기준으로 보존한다.

## 44. River Critical Contact / Two Inlets V72

V72는 V71의 수계와 PCG architecture를 유지하면서 수면 내부의 작은 지면 노출과 사용자 지정 두 inlet 누락을 수정한다. V71의 corridor 바깥에 있던 두 inlet만 등록된 satellite water evidence로 확장한다. 기존 water cell 삭제는 0이며 XY 변경은 `X05_Y02`, `X05_Y03`, `X06_Y02`에 한정한다. 나머지 24개 River는 XY와 material/phase를 유지하고 접지 높이만 보정한다.

접지의 근본 변경은 sample 개수 증대가 아니라 검사 위치다. 이전 중심점/등간격 barycentric 검사로는 Landscape grid/diagonal과 water triangle edge의 교차점에서 생기는 높이 극값을 놓칠 수 있었다. `river_contact_geometry_v72.py`가 모든 교점 및 삼각형 내부 Landscape grid vertex를 계산하고 실제 collision MIP 0 Landscape trace로 검증한다. 부족한 triangle의 vertex만 올리고 공유 XY 높이를 동기화한다. 이 과정은 Editor-baked Z이며 새 triangle, WPO, runtime trace, Tick 또는 물리 시스템을 추가하지 않는다.

27개 현재 Mesh는 `/Game/Environment/River/Production/Meshes/V72ContactInlets`에 있다. 수역을 추가한 3개 타일만 V71 MI를 상속하는 `FlowV72/MI_ENV_RiverSurface_InletsV72`를 사용하며, 기존 물의 phase texel과 속도는 그대로다. V67 특수 shared-seam pair 역시 유지한다.

PCG는 기존 세 shared mask graph의 texture만 ProductionV72로 동기화한다. 수면과 bank가 변한 4개 cell(`X05_Y02`, `X06_Y02`, `X05_Y03`, `X06_Y03`)의 기존 Forest/Riparian/LowerLayer 12개 component를 cleanup 후 generate한다. 새 Actor/PCGVolume은 없다. 대상 외 instance count는 유지하고 전 계층을 각자의 실제 water input mask로 검사한다.

검증은 제안 OBJ만 확인하는 것으로 끝내지 않는다. native `StaticMeshExporterOBJ`로 저장된 LOD0를 다시 추출해 동일 critical-point 검사를 수행한다. import/trace 정밀도로 안전 여유가 부족하면 저장 vertex와 원본을 대응시켜 해당 triangle만 sub-centimeter 보정하고 재검사한다. 공유 tile edge를 바꾸는 residual 보정은 허용하지 않는다. 상세 이력·복구·검증 한계는 작업 폴더 `work/RIVER_V72.md`에 기록한다.

</details>
