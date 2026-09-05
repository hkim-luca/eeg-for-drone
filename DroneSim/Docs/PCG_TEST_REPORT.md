# PCG Natural Environment Test Report

## Current validation — rendering-only performance trial (2026-09-05)

- Pre-trial rollback point: 137 files / 1,110,800,677 bytes copied and SHA-256 verified in external `work/performance_trial_20260905/baseline/`; native Editor quality values backed up separately. This supersedes the older cleanup backup as the rollback point for this trial.
- Native apply and fresh-process reload validation pass: 192 actors, 27 rivers, all 3,475,293 vegetation transforms identical to the pre-trial snapshot. No PCG regeneration. Seven graph spawner policies and 616 saved component policies are synchronized.
- Affected rendering policies: 516,675 trees have WPO disable at 300 m rather than 1000 m; 318,202 grass instances in four regions have 1000 m rather than 2200 m end-cull. Counts, positions, scales, rotations, materials, collision and shadow flags are unchanged.
- Local Editor Shadow/GI/Reflection/PostProcess quality changed from Epic (3) to High (2). Foliage quality remains 3, hardware ray tracing remains enabled, t.MaxFPS remains 0. These are shared UE 5.7 user settings, not packaged quality defaults.
- Native Map Check: 0 errors / 0 warnings. Five before/after fixed-camera pairs were visually inspected at X07_Y05, X04_Y04 (near/mid/far), and X02_Y01. Near grass patterns are retained, far grass is reduced, and no new visible river breaks/holes were identified in these views. The before captures replay the baseline rendering properties in memory without saving. Animated water/sky prevent pixel-equality comparison; captures do not validate continuous movement or the entire map.
- Native river LOD probe was not saved: 71,232 to 35,615 triangles changed boundary geometry; sampled boundary deviation reached 0.8292 cm despite a requested 0.5 cm MaxDeviation. This sample is not a global maximum. Boundary/contact preservation was not established, so all river assets and LODs remain unchanged.
- Evidence and limitations: see PERFORMANCE_TRIAL.md and external `native_before.json`, `native_apply.json`, `native_validate.json`, `apply_result.json`, `validate_result.json`, `river_lod_probe.json`, `capture_before.json`, `capture_after.json`, and `changed_files.json`. No controlled FPS/GPU-time/RAM/VRAM improvement measurement, full restore rehearsal, Cook/Package/Standalone or packaged offline test has been completed. Existing supplied Huckleberry Oak custom-version errors are outside this trial's scope.

## Previous accepted validation — pre-commit consolidation (2026-09-04)

Scope: preserve the accepted V72 result while removing only our unused, Git-untracked PCG/river work and consolidating active asset paths. Original code, maps, libraries and already-tracked files are excluded. The history below is preserved; its former package names and counts are not current.

| Check | Actual result |
|---|---|
| Verified external backup | 195 files, 1,151,515,354 bytes; copied SHA-256 matches |
| Unused assets removed | 57: 29 meshes, 15 textures, 9 PCG graphs, 3 material instances, 1 material |
| Active assets renamed | 87; no geometry/texture resampling or graph duplication |
| Redirectors removed after reference checks | 53 |
| Fresh-process Environment inventory | 118 assets; no unexpected or missing packages |
| All surviving Environment dependencies | Match original dependency graph after applying rename mapping |
| Fresh map actor / river count | 192 / 27; no Actor deleted |
| All PCG actor counts | Exactly match accepted saved output; 3,475,293 total |
| River LOD0 geometry, UV and normals | Native OBJ payload SHA-256 unchanged for all 27 meshes |
| River triangles | 2,746,176, unchanged |
| Flow texture exports / material values | Same pixel-export hashes and scalar/vector/texture/switch values, adjusted for path mapping |
| Actor/component layout, mesh/material assignments, transforms, collision/shadows | No non-path differences |
| Tracked files / Git index checkpoint | 526 tracked-file hashes unchanged; staging index unchanged |
| Final protected external inventory | 5,198 file paths/sizes/mtime unchanged; 25 preserved Environment imports also SHA-256 unchanged |
| Final native Map Check | 0 errors / 0 warnings |
| Final rendered QA | 5 accepted viewpoints captured after fresh load; no new visible holes/disconnections in these views |
| Legacy riparian optimization persistence | 237 ISMs and 4 graph descriptors retain 3500/5000 m cull metadata and 1000 m WPO cutoff after reload |
| All affected tree transforms | 35,442 position/rotation/scale payload hashes identical before, after and after reload |
| Unbounded end-cull vegetation | 35,442 → 0 instances; no vegetation removed |
| Renamed raw mask import sources | Both resolve to existing canonical PNG files; no reimport/resampling |
| Follow-up PCG-only audit | 26/26 assets reachable from seven live production graphs; all 29 files accounted for; unused assets/redirectors/empty directories 0 |

Evidence: external work files `precommit_audit.json`, `precommit_plan.json`, `precommit_cleanup_result.json`, `precommit_integrity_before.json`, `precommit_integrity_after.json`, `precommit_validation.json`. Native Editor save/reload was used; binary assets were not edited as text. A guarded first redirector pass stopped on remaining old-package references. A fresh process resolved and saved the owned references; leaf-first removal then succeeded without force-removing a live dependency.

The earlier accepted contact validation remains applicable to unchanged LOD0 geometry: 17,970,194 critical sample entries, minimum clearance 7.9986778103 cm, no entry below 7.99 cm, 1,660 shared groups with 0 cm height difference. That expensive terrain trace was not rerun merely for asset renaming; the exact geometry payload was compared instead.

Final evidence additionally includes `precommit_final_native.json`, `precommit_final_files.json`, `precommit_tree_distance.json`, `precommit_performance_before.json`, `precommit_performance.json`, and `river_v72_capture_final.json`. Expected rendering-policy changes are checked separately from the path-only geometry/layout comparison. Captures use the same cameras but are not pixel-equality tests because water/sky animate. They do not constitute exhaustive fly-through validation.

Limitations: this cleanup is Editor/static-asset/rendered-view validation, not a new FPS/VRAM profile or Cook/Package/Standalone/offline packaged test. The pre-existing incompatible Huckleberry Oak library custom-version errors remain outside the authorized scope and were not fixed by changing the supplied library. See PRECOMMIT_CLEANUP.md for the final filesystem checks and backup location.

<details>
<summary>Historical validation reports — prior versions, not the current manifest</summary>

## 1. 문서 상태

- Project: `DroneSim`
- Engine: `5.7.4-51494982+++UE5+Release-5.7`
- Level: `/Game/Maps/Daejeon_PCG_Work`
- Last Updated: `2026-09-04`
- 현재 판정: River Surface/PCG spatial authority V70 동기화, X00_Y02 hole fill, 전역 수면 식생 exclusion 및 fresh-load 검증 통과. packaged runtime 성능 검증은 별도 필요

### Riparian Lower Layer V40 최종 결과

| Test | Result | 측정값 |
|---|---|---:|
| Prototype visual validation | Pass | 지상/드론 캡처에서 강 바로 옆 연속 식생 띠 확인 |
| Bank raster source | Pass | V38 Production river raster와 동일 |
| Bank/water raster size | Pass | `2017 × 2017` / `2017 × 2017` |
| Water pixels | 기록 | `30,547` |
| Inner-bank pixels | 기록 | `11,055` |
| Outer-transition pixels | 기록 | `10,908` |
| Bank/water overlap | Pass | `0` |
| Production deployment | Pass | `19/19` 셀 |
| Empty river tile handling | Pass | `X05_Y00`은 후보 `0`, 따라서 Lower Layer Actor를 만들지 않음 |
| Production instance count | Pass | `429,498 → 224,456` (`-47.74%`) |
| Prototype actor cleanup | Pass | `1 → 0` |
| River Surface actors preserved | Pass | `20/20` |
| Graph references | Pass | Production `19/19`가 V40 사용 |
| Grass policy | Pass | 4+ LOD, collision/shadow off, layer별 culling 일치 |
| Young alder policy | Pass | collision off, shadow on, `100–1,000 m` culling |
| Unapproved mesh | Pass | `0` |
| Cell isolation | Pass | 각 배포 시 대상 외 Lower Layer Actor 변화 `0` |
| Generation callback total | 기록 | 약 `521.93 s` / 19개 별도 commandlet |
| Fresh-load audit | Pass | UE `5.7.4`, Production 19, Prototype 0, River 20 |
| Obsolete asset cleanup | Pass | 외부 referencer `0` 확인 후 `32`개 삭제 |

Prototype Y01_X03은 V37 `9,558`개에서 V40 `26,321`개로 생성해 양안 가시성을 먼저 검증했다. 전역에서는 강이 긴 셀은 인스턴스가 증가하고 강이 거의 없는 셀은 크게 감소했다. 최대 과밀 셀 중 `X05_Y02`는 `108,746 → 5,518`로 줄어 기존 렉 원인의 큰 부분을 제거했다. 최종 합계는 Prototype을 포함하지 않는다.

Map Check는 prototype 저장, 각 production 셀 저장, cleanup 직전/직후 전역 감사에서 호출했다. 명령은 오류 없이 종료했고 최종 read-only 감사의 모든 구조/참조/정책 check가 통과했다. 이는 packaged build FPS/GPU/VRAM 검증을 대신하지 않는다.

## 2. 검사 방식

- `.uasset`과 `.umap`은 텍스트로 해석하지 않았다.
- UE 5.7.4 `UnrealEditor-Cmd.exe -RenderOffscreen`과 Unreal Python으로 Asset/Graph/Actor를 로드했다.
- PCG를 tile별 생성하고 생성 결과를 Level에 저장했다.
- 새 Editor 프로세스에서 Level을 다시 로드해 Actor, Graph reference, generation trigger, ISM 수량을 재검사했다.
- 전역 instance origin은 CSV로 수집해 mask, spacing, duplicate, tile boundary를 외부 수치 검사했다.
- 외부 mask mapping은 설치된 UE 5.7 `UPCGTextureData::SampleInternal` texel-center 공식과 일치시켰다.
- Map Check를 실행했다.

## 3. 전역 배포 결과

### Grid와 저장 상태

| Test | Result | 측정값 |
|---|---|---:|
| Forest tile | Pass | `64/64` |
| Riparian tile | Pass | `64/64` |
| Tile size | Pass | `7.56 km × 7.56 km` |
| Landscape coverage | Pass | 정확한 `8 × 8` grid |
| Forest trigger | Pass | `64/64 GenerateOnDemand` |
| Riparian trigger | Pass | `64/64 GenerateOnDemand` |
| Fresh-load persistence | Pass | 수량/Graph/trigger 유지 |
| Manual exclusion | Pass | `PCG_EXCL_Manual_Central_01` 유지 |

### Instance 수량

| Layer | A | B | C | D | Total | Nonempty tiles |
|---|---:|---:|---:|---:|---:|---:|
| Forest / Aleppo Pine | `175,367` | `174,881` | `34,892` | `34,825` | `419,965` | `64` |
| Riparian / Black Alder | `11,812` | `3,990` | `7,834` | `11,887` | `35,523` | `60` |
| Combined |  |  |  |  | `455,488` |  |

V33 Forest tile당 count는 최소 `1,852`, 평균 `6,561.95`, p95 `9,867`, 최대 `11,933`다. 현재 저장된 Riparian tree는 빈 타일 포함 최소 `0`, 평균 `555.05`, p95 `1,380`, 최대 `2,408`, nonempty `60/64`다.

### 생성 시간

- Forest V33 PCG generation callback 합계: 약 `14.22 s`
- Forest V33 commandlet 전체 elapsed: 약 `23.11 s`
- Near-Bank Grass V33 단일 셀 generation: 약 `18.76 s`
- 생산 Riparian tree는 V33 대상 Graph가 아니므로 별도의 V33 generation callback 시간은 기록하지 않았다.

이 값은 `-RenderOffscreen` Editor commandlet의 해당 세션 결과다. packaged runtime frame-time 또는 사용자 PC의 live Editor FPS와 동일한 지표가 아니다.

## 4. 공간 정확성

### Water/Riparian mask

| Test | Forest | Riparian | Result |
|---|---:|---:|---|
| Mask image 밖 origin | `0` | `0` | Pass |
| Hard-water bilinear density `>=0.9` | `0` | `0` | Pass |
| Riparian influence 밖(1.5 pixel 초과) | N/A | `0` | Pass |

Nearest-pixel zone 분류에서 경계값처럼 보이는 소수 point가 있었으나, UE와 같은 bilinear/texel-center sampling으로 재검사했을 때 hard-water overlap은 `0`이다. Riparian 경계의 차이는 모두 source raster 1.5 pixel, 약 45m 이내이며 mask 원해상도 약 30m/pixel 범위다.

### Spacing과 tile seam

| Test | Forest | Riparian | Result |
|---|---:|---:|---|
| Minimum XY spacing | `15.02 m` | `18.04 m` | 기록 |
| Exact duplicate below `1 cm` | `0` | `0` | Pass |
| Severe overlap below `9 m` | `0` | `0` | Pass |
| Cross-tile severe overlap below `9 m` | `0` | `0` | Pass |
| Pair below target `18 m` | `6` | `0` | Forest tile seam 제한 기록 |

Forest의 6개 pair는 독립 tile seed 경계에서 발생하며 최단 거리는 약 15.02m다. 심각 겹침 기준 9m는 모두 통과했다. 전체 전역 pruning을 추가하면 이 6개를 제거할 수 있지만, 현재는 시각·생성 비용 tradeoff 때문에 보류한다.

## 5. Graph와 Asset 검증

- Forest Graph: `/Game/Environment/PCG/Graphs/PCG_ENV_ForestRegion`
- Riparian Graph: `/Game/Environment/PCG/Graphs/PCG_ENV_RiparianTrees`
- Forest own-bounds Sampling: `/Game/Environment/PCG/Subgraphs/PCG_ENV_ForestRegionSampling`
- Hard water Subgraph: `/Game/Environment/PCG/Subgraphs/PCG_ENV_WaterChannelMask`
- Riparian Subgraph: `/Game/Environment/PCG/Subgraphs/PCG_ENV_RiparianInfluence`
- 활성 Water Texture: `T_ENV_WaterChannel_Daejeon_Extended_PrimaryCalibrated`
- 활성 Riparian Texture: `T_ENV_RiparianZones_Daejeon_Extended_VisualMatch`
- 두 생산 Graph의 WaterExclusion: `Binary`
- Forest/Riparian A/B/C/D Static Mesh reference: 모두 확인
- 생산 출력 방식: ISM, Actor spawn 아님
- Tree simple collision primitive: 없음

Tree Prototype 단계에서 확인했던 Landscape grounding, yaw/scale variation, duplicate `0` 조건은 이후 생산 Graph가 동일 ground trace/variation 경로를 재사용하고 해당 구조가 변경되지 않아 반복 전체 trace 검사를 수행하지 않았다.

## 6. Corrected River Alignment Prototype v4

### 외부 정합 검사

- Source: VisualMatch GeoJSON의 `금강`, `osm_id=34834591`
- Selected source point index: `1050..1057`
- Corrected path length: 약 `1.73 km`
- Resampled cross-sections: `59`
- Lateral columns: `3`
- OSM centerline role: 위성 탐색을 제한하는 coarse prior
- Satellite local snap: 각 section을 법선 방향 `±3 pixels`에서 평가
- Selected local offsets: `-3..+2 pixels`, mean `+0.2119 pixels`
- Preview: `work/corrected_river_prototype_v4_preview.png`

v1은 식생 제외용 buffered hard-water mask를 메시 폭으로 사용했고, v2의 평탄 단면은 Landscape 하상 carve가 없어 떠 보였다. v3는 실제 Water Area polygon과 모든 vertex trace를 사용해 구조 검증은 통과했지만, polygon 전체에 동일한 `(-2, 0) pixel` shift를 선택했다. 왼쪽의 잘 맞는 넓은 수면이 점수를 지배해 오른쪽의 육지/도로 위 오정합이 숨겨졌고 사용자의 육안 검수에서 실패했다. v4는 실제 Landscape Material 좌표식을 확인한 뒤 각 longitudinal section을 위성 영상에서 독립적으로 보정하고 경로 연속성을 제한했다.

### Unreal 검사

| Test | Result | 측정값 |
|---|---|---:|
| Actor count | Pass | `1` |
| Actor | Pass | `ENV_RiverSurface_CorrectedPrototype_04` |
| Mesh/Material reference | Pass | fresh-load 일치 |
| Landscape traces | Pass | `59 × 3 = 177` points |
| Geometry | Pass | `177 vertices / 232 triangles` |
| OBJ topology | Pass | index/triangle topology valid |
| Horizontal diagonal | Pass | 약 `1.729 km` |
| Landscape-following Z range | 기록 | 약 `32.24 m` |
| Validation ribbon width | Pass | 약 `27 m` |
| Surface clearance | Pass | 모든 vertex 정확히 `12 cm` |
| Collision/Overlap | Pass | disabled |
| Shadow/Distance Field/Decal | Pass | disabled |
| Nanite/Ray tracing | Pass | disabled |
| Map save/reload | Pass | reference/settings 유지 |
| Rejected v3 Actor removal | Pass | `1` removed; v3 mesh는 당시 보존 후 v20 정리에서 제거 |

이 Prototype은 실제 수면 시뮬레이션이 아니라 중심 좌표·접지 검수용 draped ribbon이다. 약 32m Z range는 Landscape를 따라간 결과이며 실제 하천 수위 자료가 아니다. 약 27m 폭도 최종 강 폭이 아니다.

추가 범위 검사에서 OSM `금강` LineString의 전체 `1,058`점 중 `1050..1057`, 단 `8`개 원본 점만 v1-v4에 사용된 것이 확인됐다. 단일 LineString ribbon은 사용자가 표시한 넓은 수역 경계와 합류/분기를 표현할 수 없다. 같은 위치의 VisualMatch Water Area `osm_id=34546126`은 Polygon 경계점 `183`개이며 위성 수역 윤곽과 상당 부분 일치한다. 따라서 v4는 technical asset test는 통과했지만 최종 강 형태 visual test는 실패다.

잘못된 v1/v2/v3 Actor는 Level에서 제거했다. v3 Mesh Asset은 당시 실패 원인 비교용으로 보존했으나 v20 정리에서 제거했다. 더 이전의 `SM_ENV_RiverPrototype`과 `M_ENV_RiverPrototype`도 referencer `0`을 확인한 뒤 제거했다.

## 7. Water Area River Prototype v5

### 외부 형상 계획

- Source: `Daejeon_SatelliteExtent_VisualMatch_Water_Area.geojson`
- Feature: OSM way `osm_id=34546126`, `natural=water`, `water=river`
- Source geometry: `Polygon`, ring `183`점(중복 폐합점 포함), 고유 `182`점
- Approximate source area: `2.50523 km²`
- Grid step: `0.5 pixel`, 약 `15 m`
- Preview: `work/water_area_river_prototype_v5_preview.png`
- Preview legend: cyan/yellow가 v5 Water Area polygon, red가 폐기한 v4 8-point ribbon

### Unreal 검사

| Test | Result | 측정값 |
|---|---|---:|
| Actor count | Pass | `1` |
| Actor | Pass | `ENV_RiverSurface_WaterAreaPrototype_05` |
| Mesh | Pass | `SM_ENV_RiverSurface_WaterAreaPrototype_05` |
| Mesh/Material reference | Pass | fresh-load 일치 |
| Landscape traces | Pass | 모든 `12,090` vertices |
| Geometry | Pass | `12,090 vertices / 22,278 triangles` |
| Raster cells | Pass | `11,139` |
| Horizontal extent | Pass | 약 `6.507 × 3.314 km` |
| Horizontal diagonal | Pass | 약 `7.302 km` |
| Landscape-following Z range | 기록 | 약 `93.30 m` |
| Surface clearance | Pass | 모든 vertex `12 cm` |
| Collision/Overlap | Pass | disabled |
| Shadow/Distance Field/Decal | Pass | disabled |
| Nanite/Ray tracing | Pass | disabled |
| Map save/reload | Pass | reference/settings 유지 |
| Rejected v4 Actor removal | Pass | `1` removed; v4 mesh는 당시 보존 후 v20 정리에서 제거 |
| Map Check | Pass | `0 error, 0 warning` |

이 v5는 실제 수평 수면이나 Water Body가 아니다. 긴 삼각형이 지면에서 뜨는 문제를 피하려고 polygon 내부를 약 15m 격자로 나누고 각 vertex를 Landscape에 trace한 draped 형상·좌표 등록 Prototype이다. `93.30 m` Z range는 수위가 아니라 Landscape 기복을 뜻한다.

### 저장 수목 교차 검사

| Layer | 저장 Total | Bounding-box candidates | Water Area 내부 | Result |
|---|---:|---:|---:|---|
| Forest | `310,501` | `463` | `0` | Pass |
| Riparian | `34,153` | `102` | `0` | Pass |

Forest/Riparian 생산 Actor와 instance 수량은 변경되지 않았다. 이 검사는 선택한 v5 polygon 하나에 대한 exact point-in-polygon 검사이며 아직 전역 수계 검사가 아니다.

사용자 육안 검사에서 v5 Polygon은 실제 연속 수로가 아니라 육지를 크게 따라가는 것으로 확인됐다. 기술 검증과 tree overlap `0`은 Asset 구조와 선택 Polygon 내부만 검증한 것이므로 시각 정합 성공을 의미하지 않는다. v5 최종 판정은 Fail이며 Actor는 v6 교체 후 제거했다. v5 Mesh도 v20 정리에서 제거했다.

## 8. Direct Vector Centerline Region Prototype v6

### 외부 지역 하천망 검사

- Source: `Daejeon_SatelliteExtent_VisualMatch_Centerline.geojson`
- Projection: WGS84 direct base, 기준 원점 `(36.3504, 127.3845)`, `30 m/pixel`
- Image-fitted affine: 사용하지 않음
- Region: 약 `21 × 15 km`
- Selected source: `15` features, `455` segments
- Named rivers: `갑천`, `금강`, `대교천`, `미호천`, `안산천`, `용수천`, `용호천`
- Total centerline length: 약 `55.02 km`
- Preview: `work/vector_river_region_prototype_v6_preview.png`
- Preview left/yellow: v6 direct projection
- Preview right/cyan: 이전 affine 결과, 비교용이며 Unreal v6에는 미사용

### Unreal 검사

| Test | Result | 측정값 |
|---|---|---:|
| Actor | Pass | `ENV_RiverVectorRegionPrototype_06` 1개 |
| Mesh | Pass | `SM_ENV_RiverVectorRegionPrototype_06` |
| Mesh/Material reference | Pass | fresh-load 일치 |
| Projection policy | Pass | direct base, no fitted affine |
| Landscape traces | Pass | 모든 `3,198` vertices |
| Geometry | Pass | `3,198 vertices / 2,288 triangles` |
| Horizontal extent | Pass | 약 `21.004 × 13.783 km` |
| Horizontal diagonal | Pass | 약 `25.123 km` |
| Landscape-following Z range | 기록 | 약 `90.79 m` |
| Diagnostic ribbon width | 기록 | `30 m` |
| Surface clearance | Pass | 모든 vertex `18 cm` |
| Collision/Overlap | Pass | disabled |
| Shadow/Distance Field/Decal/Nanite/RT | Pass | disabled |
| Map save/reload | Pass | reference/settings 유지 |
| Rejected v5 Actor removal | Pass | v5 Actor `0`, v5 Mesh는 v20 정리에서 제거 |
| Forest/Riparian preservation | Pass | `310,501 / 34,153` |
| Map Check | Pass | `0 error, 0 warning` |

v6는 한 feature의 면을 수면으로 확정하지 않는다. 여러 Centerline의 형태와 offset을 한 화면에서 비교하기 위한 얇은 진단 ribbon이며 실제 강 폭이나 최종 visible water가 아니다.

사용자 화면에서는 v6의 폭 `30 m`/clearance `18 cm`가 광역 시점과 구름·지형 가림에 비해 너무 작았고, `455`개 source edge를 각각 ribbon으로 만든 탓에 주요 강줄기보다 작은 feature와 선분 경계가 두드러졌다. 좌표 판정이 불가능한 표시 방식으로 최종 판정해 v6 Actor를 제거했으며 Mesh도 v20 정리에서 제거했다.

## 9. Primary River Alignment Overlay Prototype v7

### 외부 위성 정합 검사

- Source: `Daejeon_SatelliteExtent_MajorWater_Centerline.geojson`
- Projection: WGS84 direct base, 기준 원점 `(36.3504, 127.3845)`, `30 m/pixel`
- Selected: `7` features / 연결 상태를 유지한 `7` polylines
- Named rivers: `갑천`, `금강`, `미호강`, `미호천`
- Total centerline length: 약 `47.33 km`
- Preview: `work/primary_river_overlay_prototype_v7_preview.png`
- 외부 판정: cyan 중심선이 `daejeon_satellite_z16`의 주요 어두운 하도 중앙을 연속적으로 따름
- 사용자 Editor 판정: `Mirror Y`와 X scale `-1.12` 적용 후 주요 하도와 일치

### Unreal 검사

| Test | Result | 측정값 |
|---|---|---:|
| Actor | Pass | `ENV_RiverPrimaryAlignmentOverlayPrototype_07` 1개 |
| Mesh | Pass | `SM_ENV_RiverPrimaryAlignmentOverlayPrototype_07` |
| Mesh/Material reference | Pass | fresh-load 일치 |
| Projection policy | Pass | direct WGS84, no fitted affine |
| Source continuity | Pass | `7` continuous polylines |
| Landscape traces | Pass | 모든 `1,810` vertices |
| Geometry | Pass | `1,810 vertices / 1,796 triangles` |
| Horizontal extent | Pass | 약 `20.993 × 15.047 km` |
| Landscape-following Z range | 기록 | 약 `88.04 m` |
| Diagnostic ribbon width | 기록 | `90 m`; 실제 강 폭 아님 |
| Surface clearance | Pass | 모든 vertex `8 m`; 실제 수위 아님 |
| Collision/Overlap/Shadow/Nanite/RT | Pass | disabled |
| Map save/reload | Pass | reference/settings 유지 |
| v6 Actor removal | Pass | v6 Actor `0`, v6 Mesh는 v20 정리에서 제거 |
| Forest/Riparian preservation | Pass | `310,501 / 34,153` |
| Map Check | Pass | `0 error, 0 warning` |

수동 보정이 저장된 v7 Actor Transform은 `Location=(-485149.0006,-1538182.6156,0) cm`, `Yaw=180°`, `Scale=(-1.12,1,1)`이다. 이를 world vertex 좌표식으로 환산한 regional calibration은 `x'=1.12x+45169.8801`, `y'=-y-3060805.2311` cm다.

## 9.1 Primary Water Area Region Prototype v9

승인된 regional calibration을 VisualMatch Water Area 중 Primary Centerline과 교차하는 `river` Polygon `8`개에 bake했다. Actor에는 음수 scale을 남기지 않았다.

| Test | Result | 측정값 |
|---|---|---:|
| Actor/Mesh | Pass | `ENV_RiverWaterAreaRegionPrototype_09` / `SM_ENV_RiverWaterAreaRegionPrototype_09` |
| Selected area | 기록 | `8 features`, 약 `10.3014 km²` |
| Grid/geometry | Pass | `30 m`, `11,446 cells`, `13,572 vertices`, `22,892 triangles` |
| Landscape traces | Pass | 모든 `13,572` vertices |
| Actor scale | Pass | `(1,1,1)` |
| Surface clearance | Pass | `25 cm` |
| Topology | Pass | flipped/degenerate XY triangle 없음 |
| Collision/Overlap/Shadow/Nanite/RT | Pass | disabled |
| Map save/reload | Pass | reference/settings 유지 |
| Forest/Riparian preservation | Pass | `310,501 / 34,153` |

기존 생산 water mask와 교정된 v9 Polygon을 비교했을 때 Forest `945`, Riparian `5` instances가 수역 cell 안에 있었다. 영향 범위는 Forest `6/64` tiles (`Y01_X01`, `Y01_X02`, `Y01_X04`, `Y02_X02`, `Y02_X03`, `Y02_X04`)과 Riparian `2/64` tiles (`Y01_X04`, `Y02_X02`)이었다. 사용자 육안 승인 뒤 기존 Extended VisualMatch와 교정 Primary mask를 maximum으로 병합하고 이 8개 타일만 재생성했다.

별도 source Texture `/Game/Environment/PCG/Data/Mask/T_ENV_WaterChannel_Daejeon_CalibratedPrimary`와 생산 병합 Texture `/Game/Environment/PCG/Data/Mask/T_ENV_WaterChannel_Daejeon_Extended_PrimaryCalibrated`를 확인했다. 병합 Texture는 `2017×2017`, Grayscale, sRGB off, NoMipmaps, Never Stream, Virtual Texture off, Clamp X/Y다. 기존 mask의 `207,096`개 양수 pixel을 유지하고 Primary에서 `19,127`개를 추가해 `226,223`개가 됐다.

## 9.2 Primary Water Production Mask 적용

| Test | Result | 측정값 |
|---|---|---:|
| Targeted regeneration | Pass | Forest `6`, Riparian `2` tiles |
| Forest total | Pass | `310,501 → 308,892` (`-1,609`) |
| Riparian total | Pass | `34,153 → 34,139` (`-14`) |
| Primary water overlap | Pass | Forest `945 → 0`, Riparian `5 → 0` |
| Non-target tile preservation | Pass | mismatch `0` |
| Target A/B/C/D variants | Pass | 8개 대상 타일 전부 유지 |
| Active graph Texture | Pass | `T_ENV_WaterChannel_Daejeon_Extended_PrimaryCalibrated` |
| Fresh-load persistence | Pass | Actor/수량/Graph reference 모두 일치 |
| Fresh-load Map Check | Pass | `0 error, 0 warning` |
| Fresh-load process exit | Pass | exit code `0` |

Mutation commandlet는 맵 저장과 Map Check 완료 뒤 Python callback/PCG 정리 단계에서 access violation으로 한 차례 종료됐다. 저장된 결과를 신뢰한다고 가정하지 않고 새 프로세스에서 읽기 전용으로 다시 검증했으며, 모든 저장 수량과 참조가 일치했고 정상 종료했다. 후속 자동화에서는 이 일회성 async callback 종료 방식을 재사용하지 않는다.

## 9.3 Representative Reservoir Boundary Prototype v13

대청호·탑정호에 Primary v9 regional affine을 적용한 사전 결과는 Landscape 범위를 벗어나므로 폐기했고 Unreal Asset으로 import하지 않았다. 대신 원본 WGS84를 위성 pixel/Landscape world로 직접 투영하고, Unreal OBJ importer의 Y축 handedness 변환만 생성 단계에서 상쇄했다.

| Test | Result | 측정값 |
|---|---|---:|
| Source feature selection | Pass | 대청호 `osm_id=413915`, 탑정호 `osm_id=18072679` |
| External satellite preview | Pass | 두 source 경계가 위성 수역을 추종 |
| Regional v9 affine propagation | Rejected | 대청호 북쪽 이동, 탑정호 Landscape 밖 |
| OBJ Y compensation | Pass | local Y pre-flip, saved Actor identity transform |
| Daecheong geometry | Pass | `4,428` traced vertices / `4,428` triangles |
| Tapjeong geometry | Pass | `234` traced vertices / `234` triangles |
| Bounds center error | Pass | 대청호 `10.35 m`, 탑정호 `17.50 m` (< 30m source pixel) |
| Collision/Overlap/Shadow/DF/Decal/Nanite/RT | Pass | disabled |
| Mesh/Material reference | Pass | independent fresh-load 일치 |
| Forest/Riparian preservation | Pass | `308,892 / 34,139` |
| Approved v9 preservation | Pass | Actor 1개 유지 |
| Unreal process exit | Pass | 생성/독립 검증 모두 exit code `0` |
| Editor visual alignment | Pending | 사용자가 두 shoreline을 위성영상과 비교해야 함 |

v13은 실제 수면이 아니라 `90 m` 폭, 지면 위 `8 m`의 진단 ribbon이다. 육안 검수 통과 후에도 그대로 생산 수면으로 사용하지 않고, 승인된 좌표식만 filled Water Area/tile 생성에 재사용한다.

## 9.4 Representative Reservoir Outer Boundary Prototype v14

저장된 v13 수동 Transform을 fresh-load로 읽은 결과 두 Actor 모두 X scale `1.12`, rotation `0`이었다. 대청호 이동량은 `(250120, -16020, 0) cm`, 탑정호 이동량은 `(-186010, 5460, 0) cm`였다. 이를 지역별 Actor 보정으로 유지하지 않고 satellite-registration pixel affine에 bake했다.

| Test | Result | 측정값 |
|---|---|---:|
| Saved v13 transform audit | Pass | 두 지역 X scale `1.12`, rotation `0` |
| Registered satellite preview | Pass | 대청호/탑정호 outer shoreline이 위성 수면 경계를 추종 |
| Ring classification | Pass | 대청호 outer 1 + inner 5, 탑정호 outer 1 + inner 1 |
| v14 ribbon selection | Pass | 각 Polygon outer ring `0`만 사용 |
| Daecheong geometry | Pass | `4,566` traced vertices / triangles |
| Tapjeong geometry | Pass | `238` traced vertices / triangles |
| Actor transform | Pass | scale `(1,1,1)`, rotation `0` |
| Collision/Overlap/Shadow/DF/Decal/Nanite | Pass | disabled |
| Mesh/Material reference | Pass | independent fresh-load 일치 |
| Forest/Riparian preservation | Pass | `308,883 / 34,137` |
| Approved v9 and v13 preservation | Pass | 각 Actor 1개 유지 |
| Map Check invocation | Pass | fresh-loaded World 사용 |
| Editor visual alignment | Pass | 사용자가 수평 위치와 굴곡 일치를 확인함 |

Landscape trace 높이 범위는 대청호 약 `266.74 m`, 탑정호 약 `63.56 m`로 측정됐다. 이는 평평한 저수지 수면으로 사용할 수 있는 값이 아니며, 현재 Landscape와 실제 수문 지형의 차이 또는 boundary가 통과하는 비수면 지형을 뜻한다. 따라서 v14는 좌표·형태 진단에만 합격했고 final filled water surface에는 아직 합격하지 않았다.

## 9.5 Global Registered Mask 및 Visible-water Feasibility v15

| Test | Result | 측정값 |
|---|---|---:|
| Project-local GeoJSON source integrity | Pass | 외부 원본과 SHA-256 일치 |
| Regenerated Extended Water PNG | Pass | 프로젝트 PNG와 다른 pixel `0` |
| Regenerated Riparian PNG | Pass | 프로젝트 PNG와 다른 pixel `0` |
| Regenerated active merged Water PNG | Pass | 프로젝트 PNG와 다른 pixel `0` |
| Water Graph reference/settings | Pass | merged Texture, `2017²`, grayscale, no mips, clamp, `Set` |
| Riparian Graph reference/settings | Pass | Extended Texture, `2017²`, grayscale, no mips, clamp, `Set` |
| Saved Forest/Riparian tiles | Pass | `64 / 64` |
| Saved Forest/Riparian instances | Pass | `308,883 / 34,137` |
| Active hard-water overlap | Pass | Forest `0`, Riparian `0` |
| Riparian influence membership | Pass | raster tolerance 밖 위반 `0` |
| Major Water Area Landscape trace | Pass | 18개 후보, 표본 miss `0` |
| Flat single-plane candidate | Rejected | `0 / 18` |
| Segmented/local visible-water candidate | Conditional | `3 / 18` |
| Mask-only until manual surface authoring | Required | `15 / 18` |

전역 Mask 재계산은 기존 프로젝트 결과와 완전히 같으므로 PCG tile 재생성 및 Level 저장을 수행하지 않았다. 이 no-op은 불필요한 generation cost와 수목 seed 변동을 방지하기 위한 의도된 결과다. 대청호 표본 Z range는 약 `233.3 m`, 탑정호는 약 `60.5 m`로 다시 확인되어 단일 평면 Water Mesh 자동 생성은 중단한다.

## 10. Level 및 Log

- v18 저장·재로드 후 전체 Level Actor 수: `207`
- River production Actor: `63`; 수역별 Actor가 아니라 최대 약 `7.68 km` 공간 tile
- PCGVolume: 생산 `128` + manual exclusion `1`
- Work map file size: 약 `152.02 MB`
- 최신 Map Check: 오류 `0`, 경고 `0`
- Unreal process exit code: `0`

로그의 Chromium usage-statistics registry 접근 실패, VisionOS/Editor icon 누락, startup `LogAutomationTest: Condition failed`는 PCG 생성/MapCheck 실패와 별개다. PCG Python 보고서와 Map Check는 성공했다.

## 11. Plugin 및 Config

- `.uproject`는 이번 전역 배포/강 Prototype 단계에서 변경하지 않았다.
- Stable `PCG` plugin은 활성 상태다.
- `ProceduralVegetationEditor`는 Editor conversion 용도다.
- `Water`, `PCGWaterInterop`를 핵심 production dependency로 추가하지 않았다.
- 모든 mask/GeoJSON 변환은 development-time local processing이며 runtime network dependency가 없다.

## 12. 완료/미완료 판정

| Test | Status |
|---|---|
| UE 5.7.4 project/map load | Pass |
| Landscape/slope/mask Graph | Pass |
| 전역 Forest 64 tile | Pass |
| 전역 Riparian 64 tile | Pass |
| A/B/C/D species variants | Pass |
| Water overlap | Pass (`0/0`) |
| Severe tree overlap | Pass (`0/0`) |
| Save/reload persistence | Pass |
| Map Check | Pass (`0 error, 0 warning`) |
| Corrected 1.73km river subset v3 visual registration | Fail; Actor 제거 |
| Satellite-snapped 1.73km river subset v4 technical test | Pass |
| v4 최종 강 형태 육안 검수 | Fail; source 범위/geometry type 부적합 |
| Water Area 기반 v5 외부 형상/Unreal technical test | Pass |
| v5 Forest/Riparian exact overlap | Pass (`0/0`) |
| v5 위성 수역 육안 정합 | Fail; Actor 제거, Mesh는 v20 정리에서 제거 |
| Direct Centerline 지역 하천망 v6 technical test | Pass |
| v6 Level/Forest/Riparian 보존 | Pass |
| v6 지역 하천망 표시 방식 | Fail; Actor 제거, Mesh는 v20 정리에서 제거 |
| Major 주요 하천 연속 polyline v7 technical test | Pass |
| v7 외부 위성 overlay 정합 | Pass; 주요 하도 중앙 추종 |
| v7 Editor 육안 정합 | Pass; 사용자 Mirror Y/X scale 보정 후 승인 |
| Primary Water Area v9 Unreal technical test | Pass |
| v9 Editor 육안 수역 폭/위치 검수 | Pass; 사용자 승인 |
| v9 교정 수역 내 Forest/Riparian overlap | Pass (`0/0`), 영향 타일만 재생성 |
| Extended + Calibrated Primary water mask | Pass; production Graph 적용 |
| 대청호/탑정호 v13 direct-WGS84 boundary technical test | Pass |
| 대청호/탑정호 v13 Editor 육안 정합 | Superseded by registered outer-ring v14 |
| 대청호/탑정호 v14 registered outer-boundary technical test | Pass |
| 대청호/탑정호 v14 Editor 육안 정합 | Pass; 사용자 수평 위치/굴곡 승인 |
| 전역 Water/Riparian registered Mask | Pass; 재생성 pixel diff `0` |
| 현재 전역 Water overlap | Pass; Forest/Riparian `0/0` |
| 전역 visible water v17 XY 정합 | Fail; regional/feature별 좌표 혼용 및 OBJ Y pre-flip 누락 |
| 전역 visible water v18 technical validation | Pass; 단일 전역 affine, 63 tile, save/reload, Map Check `0/0` |
| 전역 visible water v19 Conservative technical validation | Superseded; river/reservoir/lake/canal 혼합 입력으로 인해 고립 수면이 남음 |
| 전역 river-only surface v20 technical validation | Pass; river 93개, 24 tile, fresh-load, Map Check `0/0` |
| 전역 river-only surface v20 Editor 육안 검수 | Pending |
| 도로/건물 전체 exclusion | Pending; source data 없음 |
| Shrub/Grass/Rock layer | Pending; production asset/instance budget 미확정 |
| Live Editor/Game performance profile | Pending |
| Windows cook/package | Pending |
| Packaged Level 실행 | Pending |
| 네트워크 차단 offline 실행 | Pending |

## 13. 다음 검증

1. 좌표/Mask는 동결하고 Water/Riparian 전체 타일을 불필요하게 다시 생성하지 않는다.
2. visible water가 필요한 경우 `segmented_or_local_surface_candidate` 3개 중 하나에서만 짧은 구간 prototype을 만든다.
3. Black Alder 이외의 Riparian shrub/grass asset을 확인하고, 자산이 있으면 별도 저밀도 계층의 instance budget을 먼저 정한다.
4. live Editor 또는 Standalone에서 CPU/GPU/frame time/VRAM을 측정하고 병목 근거에 따라 density, shadow, material, culling을 조정한다.
5. Windows Development cook/package와 네트워크 차단 offline 실행을 수행한다.

## 14. Global River Surface v18/v19/v20 교정 및 CC0 환경 자산

### River Surface 생성 결과

| Test | Result | 측정값 |
|---|---|---:|
| UE version | Pass | `5.7.4-51494982` |
| VisualMatch Water Area source | Pass | 285 features |
| v17 외부 위성 정합 | Fail | regional v7 affine + 2개 feature별 v13 Transform 혼용 |
| v17 OBJ handedness 처리 | Fail | v14에서 검증된 local Y pre-flip 누락 |
| v17 Level Actor 제거 | Pass | 잘못된 36개 Actor 제거; Mesh는 v20 정리에서 제거 |
| v18 single global registration | Pass | 7-control pixel affine을 Water Area 285개 전체에 동일 적용 |
| v18 representative bounds | Pass | 대청호/탑정호가 승인된 v14 bounds와 수치상 일치 |
| Surface tile 생성 | Pass | 63 Static Mesh / 63 Actors |
| Geometry budget | Pass | 43,913 vertices / 67,892 triangles |
| Landscape trace | Pass | 43,913 / 43,913 vertices |
| Actor transform | Pass | 모든 Actor scale `(1,1,1)` |
| Collision/Shadow/DF/Nanite/Ray tracing | Pass | 모두 비활성 |
| Material | Pass | 63/63 지정 Material 일치 |
| Forest 보존 | Pass | 64 actors / 308,883 instances |
| Riparian 보존 | Pass | 64 actors / 34,137 instances |
| 이전 비교 Actor 정리 | Pass | 진단 Actor 6개를 v20 정리에서 제거 |
| Fresh-load audit | Pass | 저장 Map 재로드 후 전체 항목 일치 |
| Map Check | Pass | 오류 0 / 경고 0 |
| v19 conservative source filter | Pass | 285개 중 13개 제외, 272개 포함 |
| v19 mask delta | Pass | 추가 `0` pixel / 제거 `454` pixel |
| v19 Surface tile 생성 | Pass | 62 Static Mesh / 62 Actors |
| v19 Geometry budget | Pass | 43,526 vertices / 67,498 triangles |
| v19 Landscape trace | Pass | 43,526 / 43,526 vertices |
| v19 fresh-load water-only audit | Pass | 62/62 v19 Mesh, identity Transform, Material/Collision/Shadow/Tag 일치 |
| v19 Map Check | Pass | 오류 0 / 경고 0 |
| v20 river-only source filter | Pass | 285개 중 river 93개 포함, 192개 제외 |
| v20 excluded by type | Pass | reservoir 186 / lake 2 / canal 1 / 약한 river 3 |
| v20 delta vs v19 | Pass | 추가 `0` pixel / 제거 `85,362` pixel |
| v20 Surface tile 생성 | Pass | 24 Static Mesh / 24 Actors |
| v20 Geometry budget | Pass | 13,524 vertices / 19,106 triangles |
| v20 Landscape trace | Pass | 13,524 / 13,524 vertices |
| v20 fresh-load audit | Pass | 24/24 v20 Mesh, identity Transform, Material/Collision/Shadow/Tag 일치 |
| Obsolete River cleanup | Pass | 구형 Static Mesh 173개와 진단 Actor 6개 제거 |
| 현재 River Asset | Pass | v20 Mesh 24개 + 공유 Material 1개 |
| v20 Map Check | Pass | 오류 0 / 경고 0 |

강표면은 지형 추종 visual proxy이며 `Water Body River`가 아니다. v20은 v18에서 승인된 전역 좌표 등록을 유지하면서 `water=river` Polygon만 사용한다. 저수지·호수·운하는 보이는 표면에서 전부 제외했고, 약한 river feature 3개도 제외했다. Water/Riparian constraint mask와 수목·식생 PCG는 이번 단계에서 수정·재검사하지 않았다. v20 당시 높이는 Landscape trace + `18 cm`, Material은 `M_ENV_RiverSurface_CorrectedPrototype`이었다. 현재 생산 상태는 아래 v23 검증을 따른다.

### Kenney CC0 Mesh import 검증

공식 Kenney Nature Kit 2.1 ZIP을 프로젝트 `SourceData`에 보관하고 10개 FBX를 Static Mesh로 가져왔다.

| Test | Result |
|---|---|
| 원본 ZIP SHA-256 | `FA7974A0D342BFE63C38664BA9F8EC1A4AAB8EA25F099BDC56870E33588C4D9D` |
| License | Pass, CC0 1.0 Universal |
| Static Mesh asset count | Pass, 10 |
| Unreal centimeter scale | Pass, 10/10 |
| LOD0 geometry | Pass, 10/10 nonzero |
| Material assignment | Pass, 10/10 |
| Nanite | Off, 10/10 |
| Simple/convex collision | 0, 10/10 |
| Production PCG wiring | 변경하지 않음 |

Mesh import는 성공했지만 기존 사실적 Tree와의 시각 스타일 통합은 아직 검수되지 않았다. 이 때문에 기술적으로 유효한 자산 확보와 production 배치를 구분했다.

## 15. 다음 검증 순서

1. `Daejeon_PCG_Work`를 열고 `Environment/River/Production` 폴더 표시를 켠다.
2. 지상 시점에서 v23 강둑의 30 m raster 계단이 충분히 완화됐는지 확인한다.
3. 드론/PIE 시점에서 밝은 cyan 띠, 큰 교차 물결, 과도한 shimmer가 줄었는지 확인한다.
4. 움직임 방향이 굴곡에서 눈에 띄게 어긋날 때만 주요 강줄기용 spline/flow-map 단계를 추가한다.
5. 시각 검수가 통과하면 하천 식생을 수역 mask에서 분리된 근안 band에 저밀도로 시험한다.
6. River/하천 식생 instance count, frame time, GPU time을 측정하고 밀도·그림자·culling을 조정한다.
7. 마지막으로 Windows Development cook/package, packaged Level 실행, 네트워크 차단 offline 실행을 검증한다.

## 16. River Surface v22 WaterMaterials 적용 검증

2026-08-26에 UE `5.7.4-51494982`의 Unreal Python/Editor Asset API를 사용해 수역 범위만 변경했다. Forest/Riparian PCG Graph와 수목 instance는 수정하거나 재생성하지 않았다.

| Test | Result | 확인값 |
|---|---|---:|
| WaterMaterials asset registry | Pass | `/Game/WaterMaterials` 아래 `107` assets |
| Parent material | Pass | `/Game/WaterMaterials/Materials/M_River_Cheaper`, Translucent |
| Project Material Instance save | Pass | `/Game/Environment/River/Materials/MI_ENV_RiverSurface_WaterMaterials_Flow` |
| Approved tile exclusions | Pass | `X02_Y05`, `X04_Y05`, `X05_Y04`, `X05_Y05` 모두 부재 |
| Active production Actors | Pass | `20/20`, active v22 plan과 label 일치 |
| Rejected X02_Y05 Mesh cleanup | Pass | 참조 `0` 확인 후 Asset 제거 |
| Component material references | Pass | `20/20` shared Instance 사용 |
| Static Mesh material references | Pass | `20/20` shared Instance 사용 |
| Collision/Shadow policy | Pass | 기존 비활성 정책 유지 |
| Save/reload | Pass | `Daejeon_PCG_Work` 저장 후 재로드 검사 |
| Map Check | Pass | 오류 `0`, 경고 `0` |
| Material/shader error in apply log | Pass | 적용 로그에서 관련 Error 없음 |
| Visual flow/colour review | Pending | 실제 Editor/PIE 렌더링 육안 검수 필요 |
| Packaged/offline test | Pending | 이번 단계에서 cook/package 미실행 |

현재 Material은 normal/반사 애니메이션으로 흐르는 물의 인상을 제공하지만 하천 굴곡별 접선 방향을 계산하는 flow-map은 아니다. 다음 육안 검수에서는 중앙의 넓은 강과 좁은 지류를 각각 지상/드론 시점에서 확인하고, 투명도·색·움직임이 과하면 공유 Material Instance만 조정한다. 기존 terrain-following 메시와 낮춘 displacement 강도를 유지하므로 Actor별 Transform 또는 개별 Material 복제는 하지 않는다.

## 17. River Surface v23 강둑 평활화 및 하천 재질 조정 검증

2026-08-26에 사용자가 제공한 Editor 녹화 영상을 검토했다. 뾰족한 강둑의 직접 원인은 약 `30 m/pixel` 수역 raster의 cell 외곽을 그대로 면으로 만든 계단 경계였고, 부자연스러운 원거리 수면은 WaterMaterials의 WPO/normal/반사와 밝은 cyan 색이 하천 규모에 비해 강한 것이었다. 수역 위치, river-only 분류, 네 개 제외 타일, 식생/PCG는 변경하지 않았다.

| Test | Result | 확인값 |
|---|---|---:|
| Boundary-only smoothing plan | Pass | degree-2 경계 정점 `29,682`, 합류점/끝점 `83` 고정 |
| Maximum bank displacement | Pass | 실제 `0.3128 pixel`, 약 `9.38 m` (`0.35 pixel` 한도 이내) |
| Surface area preservation | Pass | 변화 `0.1117%` |
| Triangle topology | Pass | `135,893` vertices / `241,216` triangles, inverted `0`, degenerate `0` |
| Landscape edge protection | Pass | 31개 정점만 검증된 v22 좌표 유지 |
| Active production Actors | Pass | `20/20`, v23 Mesh와 plan label 일치 |
| Approved tile exclusions | Pass | `X02_Y05`, `X04_Y05`, `X05_Y04`, `X05_Y05` 모두 부재 |
| Shared Material Instance references | Pass | Actor/Mesh `20/20` 일치 |
| Collision/Shadow policy | Pass | 모두 비활성 |
| Save/reload and Map Check invocation | Pass | `Daejeon_PCG_Work` 저장 후 재로드 검사 |
| Terrain-contact samples | Pass | 60,304 samples, 아래로 파묻힘 `0` |
| Terrain gap | Pass | min `20.87 cm`, p01 `119.79 cm`, median `175 cm`, p99 `226.98 cm` |
| Material parameter readback | Pass | 29 scalar + 2 vector overrides 확인 |
| Static offscreen visual review after v23 | Pass | 대표 지류 `X03_Y01` 동일 카메라 비교에서 cyan slab와 넓은 하부 지형 투과 띠가 크게 감소 |
| Animated flow review | Pending | 실제 Editor/PIE에서 드론 이동 중 normal 애니메이션 속도 확인 필요 |
| Packaged/offline test | Pending | 이번 단계에서 cook/package 미실행 |

현재 결과는 raster 위치 정확도를 유지하면서 계단 모서리를 완화한 절충안이다. 30 m 원본보다 세밀한 실제 shoreline을 복원한 것은 아니며, 수면 움직임도 굴곡별 flow map이 아닌 낮은 강도의 world-space normal 애니메이션이다. 2026-08-27 오프스크린 정지 렌더에서 Material 강도를 낮추는 것만으로는 넓은 띠가 충분히 사라지지 않았고, 원본 Material의 `FakeSpec_Intensity1/2/3=512` 보조층을 끈 뒤에도 일부가 유지됐다. 최종적으로 `Opacity=0.94`, `OpacityDeep=0.985`로 하부 Landscape 투과를 억제하자 동일 카메라에서 거대한 띠가 크게 감소했다. 이 결과 때문에 별도의 v24 Z 재메시는 만들지 않았으며 v23 접지와 강둑 좌표를 보존했다. 실제 움직임이 빠르거나 반복적으로 보이면 공유 Material Instance만 다시 조정한다.

## 18. X03_Y01 Directional Flow Map Prototype 검증

2026-08-27에 `ENV_RiverSurface_Production_X03_Y01` 한 타일만 대상으로 bend-aware Flow Map과 프로젝트 소유 Material을 생성했다. 기존 `WaterMaterials` 원본과 v23 Mesh는 수정하지 않았다.

| Test | Result | 확인값 |
|---|---|---:|
| Flow source load | Pass | VisualMatch centerline `168` LineString, 대상 타일에 사용된 line segment `223` |
| Flow texture generation | Pass | `256×256`, water pixel `3,819`, sampling pixel `7,980` |
| Direction normalization | Pass | mean vector length `1.0`, min `0.99999994` |
| Unreal texture import/settings | Pass | sRGB Off, VectorDisplacementmap, NoMipmaps, NeverStream, Clamp |
| Prototype Material save | Pass | 3 texture samples, Opaque, WPO 사용 안 함 |
| Target-only assignment | Pass | X03_Y01은 prototype MI, 나머지 `19/19`는 기존 공유 MI |
| Production Mesh preservation | Pass | 생산 Actor `20/20` 모두 v23 Mesh 유지 |
| Collision/Shadow policy | Pass | prototype 대상도 비활성 유지 |
| Save/reload | Pass | Map 재로드 후 component material override 유지 |
| Map Check | Pass | 오류 `0`, 경고 `0` |
| Static offscreen render | Pass | Error Material/단색 fallback 없이 normal과 색 변화 렌더링 |
| Real-time bend direction review | Pending | Editor Realtime/PIE에서 굴곡별 움직임과 속도 확인 필요 |
| Hydrologic downstream sign | Unverified | GeoJSON vertex 순서가 하류 방향인지 확인되지 않음 |
| Packaged/offline test | Pending | 한 타일 Prototype 승인 후 생산 전환 단계에서 수행 |

이 검증은 방향성 Material/Flow Map 경로가 UE 5.7.4에서 저장·재로드되고 기존 생산 구조를 침범하지 않는다는 기술 검증이다. 정지 렌더는 애니메이션 방향을 증명하지 못하므로 전역 적용 승인으로 해석하지 않는다. 사용자 육안 검수에서 굴곡을 따라 흐르는 인상, 속도, 반복 무늬, 이음부가 통과하면 그 다음에만 전역 생성 방식을 결정한다.

## 19. River Surface V24 Global Directional Flow 검증

2026-08-27에 기존 v23 river-only Mesh `20`개를 유지한 채 전역 방향성 Flow Map과 프로젝트 소유 Opaque Material을 적용했다. Forest/Riparian PCG, Landscape, constraint mask, 수목 instance는 수정하거나 재생성하지 않았다.

| Test | Result | 확인값 |
|---|---|---:|
| Active tile discovery | Pass | v23 Mesh/Level에서 활성 ID `20`개를 재확인 |
| Global flow generation | Pass | centerline sign continuity + 전체 2048 domain masked smoothing |
| Flow Texture source | Pass | 타일별 `256×256` RGBA `20`개 |
| Direction normalization | Pass | 모든 타일 최소 vector length `≥0.99999988` |
| Texture import/settings | Pass | sRGB Off, VectorDisplacementmap, NoMipmaps, NeverStream, Bilinear, Clamp |
| Shared parent Material | Pass | `M_ENV_RiverSurface_DirectionalFlow_V24` 한 개 |
| Per-tile Material Instance | Pass | `20/20`, FlowMap/MapUVBias와 tile ID 일치 |
| Actor/material assignment | Pass | 활성 Actor `20/20`, fresh reload 후 일치 |
| Production Mesh preservation | Pass | `20/20` 모두 V23SmoothedBanks 유지 |
| Collision/Shadow policy | Pass | `20/20` 비활성 |
| Artifact fix | Pass (structural) | unbounded offset 제거, bounded two-phase cyclic sampling 사용 |
| Material graph cleanup | Pass | 재구축 전 `164` expression 누적 상태를 정리, 최종 `87`개 |
| SM6 shader compile | Pass | `Failed to compile Material`/shader error `0` |
| Save/reload | Pass | `/Game/Maps/Daejeon_PCG_Work` 재로드 후 참조 유지 |
| Map Check | Pass | 오류 `0`, 경고 `0` |
| Static offscreen render | Pass | `X03_Y01`, 기본 Material fallback 없음, 굵은 반복 띠 미검출 |
| Runtime dependency | Pass | 저장 Mesh/Texture/Material만 사용; runtime PCG/GIS/network 없음 |
| Texture-sample budget | Pass | parent당 `5` samples, Opaque, WPO 없음 |
| Real-time animation review | Pending | Editor Realtime/PIE에서 굴곡, 속도, 타일 seam 확인 필요 |
| Hydrologic downstream sign | Unverified | GeoJSON vertex 순서가 하류 방향이라는 보장 없음 |
| Packaged/offline test | Pending | 최종 환경 통합 뒤 Windows cook/package에서 검증 |

초기 전역 재구축 시 UE 5.7.4 `DeleteAllMaterialExpressions()`가 순회 중 collection을 수정해 기존 expression 일부를 남기는 현상을 발견했다. 이 때문에 제거된 `Normalize` 고아 노드가 shader compile을 실패시켰으며 Map Check만으로는 검출되지 않았다. 재구축 도구를 0개가 될 때까지 반복 삭제하도록 수정한 뒤 expression 수는 `87`로 안정화됐고 SM6 compile 오류는 0건이 됐다. 따라서 V24 완료 판정에는 구조 보고서뿐 아니라 shader compiler 로그 검사가 포함된다.

정지 렌더는 Material 적용과 큰 반복 띠의 부재를 확인하지만 실제 애니메이션 방향과 시간 변화 전체를 증명하지 못한다. 다음 Editor 검수에서는 `ENV_RiverSurface_Production_X03_Y01`을 Realtime으로 보고 (1) 물결이 굴곡을 대체로 따르는지, (2) 5~10초 후 굵은 띠가 뭉치지 않는지, (3) 이웃 타일 경계에서 방향이 갑자기 반전하지 않는지만 확인한다. 이 세 항목이 통과하면 수면 단계는 동결하고 Riparian 식생으로 이동한다.

## 20. Tree Visual Scale V25 검증

2026-08-27에 드론 시점에서 강폭 대비 수목이 작게 읽히는 문제를 기존 생산 PCG 구조 안에서 보정했다. 새 Graph나 Actor 계층을 만들지 않고 Forest/Riparian Graph의 `TreeVariation` uniform scale과 A/B/C/D 가중치만 수정했다. Density, Spacing, Seed, Ground Trace, Water Exclusion, River V24, Landscape와 Collision은 변경하지 않았다.

| Test | Result | 확인값 |
|---|---|---:|
| Engine | Pass | `5.7.4-51494982+++UE5+Release-5.7` |
| Forest Graph scale | Pass | `1.05..1.35`, uniform |
| Forest weights A/B/C/D | Pass | `5 / 5 / 1 / 1` |
| Forest stored result | Pass | `64 actors / 351,397 instances` |
| Forest instance scale readback | Pass | min `1.050001`, mean `1.199973`, max `1.349999` |
| Forest calculated height | Pass | mean `17.16 m`, p95 `20.89 m` |
| Riparian Graph scale | Pass | `1.00..1.25`, uniform |
| Riparian weights A/B/C/D | Pass | `3 / 1 / 2 / 3` |
| Riparian stored result | Pass | `64 actors / 34,137 instances` |
| Riparian instance scale readback | Pass | min `1.000002`, mean `1.124718`, max `1.249981` |
| Riparian calculated height | Pass | mean `17.81 m`, p95 `21.36 m` |
| Graph/Level consistency | Pass | Graph 설정과 저장된 ISM 범위·Mesh 비율 일치 |
| Instance budget preservation | Pass | 합계 `385,534`, 위치/총수 불변 |
| Map Check | Pass | 오류 `0`, 경고 `0` |
| Editor drone-view visual review | Pending | 사용자가 대표 강변/산림을 Realtime으로 확인 |
| Packaged/offline performance | Pending | 최종 통합 뒤 측정 |

첫 자동 저장 호출은 UE Editor API가 Map 저장을 수행하고도 `false`를 반환해 Graph 설정 복구가 실행되는 상태를 만들었다. Level에는 조정된 ISM이 저장됐으나 Graph가 이전 값이었던 불일치를 별도 Graph-only 보정으로 해소했고, 새 UE 프로세스에서 다시 로드해 일치를 확인했다. 이 보정 과정에서는 PCG를 다시 생성하지 않았다.

V25는 instance 수를 늘리지 않으므로 Actor/instance 관리 비용은 증가하지 않는다. 다만 큰 수관은 masked pixel overdraw와 shadow coverage를 소폭 늘릴 수 있으므로, 추가 density 확대는 드론 시점 육안 검수와 `stat unit`/`stat gpu` 측정 전에는 적용하지 않는다.

## 21. River Flow Speed / Riparian Lower Layer V26 검증

2026-08-28에 기존 V24 전역 방향성 Flow 구조를 유지하면서 20개 생산 Material Instance의 속도 scalar만 소폭 높였다. 이어서 기존 Riparian mask와 Water hard exclusion을 재사용하는 하층 식생 Graph를 별도로 만들고 `Y01_X03` 한 셀에만 생성했다. Forest와 생산용 Riparian tree Graph/Actor는 재생성하거나 변경하지 않았다.

| Test | Result | 확인값 |
|---|---|---:|
| Engine | Pass | `5.7.4-51494982+++UE5+Release-5.7` |
| Flow V26 PrimarySpeed | Pass | `20/20`, `-0.018 → -0.022` |
| Flow V26 DetailSpeed | Pass | `20/20`, `-0.040 → -0.048` |
| Flow geometry/material graph preservation | Pass | Mesh/Flow Map/parent graph/Actor Transform 불변 |
| Flow fresh reload | Pass | 20개 Material Instance override readback 일치 |
| Candidate asset load | Pass | Shrub 2, Grass 2, Riparian proxy 2, Rock 4 |
| Candidate technical profile | Pass with limitation | 모두 1 LOD, Nanite Off, collision primitive 0, low-poly |
| Prototype Graph | Pass | Riparian influence/Water exclusion 보존, 정확한 4 mesh/weight |
| Prototype spacing/scale | Pass | `6 m`, uniform `2.5..4.0` |
| Prototype stored result | Pass | `315` instances, 셀 밖 `0` |
| Prototype collision | Pass | 4 ISM component 모두 NoCollision |
| Production Riparian preservation | Pass | 기존 `64` actors 유지 |
| Save/reload | Pass | 새 UE 프로세스에서 Graph/Actor/315 instances 재확인 |
| Map Check | Pass | 오류 `0`, 경고 `0` |
| Visual style | Hold | 저폴리 후보가 기존 사실적 수목/강과 뚜렷하게 불일치 |
| Global deployment | Not performed | 시각 승인 또는 사실적 대체 자산 지정 필요 |
| Packaged/offline test | Pending | 생산 하층 계층 확정 뒤 수행 |

자동 근경 캡처에서는 배치·접지 자체보다 원본 자산의 단순한 실루엣과 어두운 재질이 문제로 확인됐다. 특히 `SM_ENV_CC0_RiparianTallPlant_A/B`는 갈대나 자연스러운 강변 초본으로 보이지 않는다. 따라서 기술적으로 정상이라는 이유만으로 64개 셀에 확대하지 않았고, 단일 Prototype을 비교용으로 남겼다.

추가로 `/Game/Environment/Vegetation`, `/Game/Environment/Rocks`, `/Game/Megaplant_Library`의 Static Mesh `52`개를 Asset Registry로 전수 분류했다. 크기상 하층 후보로 잡힌 `41`개 중 현재 CC0 저폴리 10개를 제외한 항목은 Megaplant의 내부 `Branch`, `Twig`, `Leaves` 조립 부품이어서 독립된 관목/갈대/초본으로 PCG 배치할 수 있는 완성형 자산이 아니었다. 현재 로컬 프로젝트에는 기존 Megaplant 품질과 맞는 완성형 하천변 하층 Static Mesh 대안이 확인되지 않았다.

## 22. 신규 환경 Asset / Riparian Dense Meadow V31 검증

2026-08-29에 V30의 성기고 균등한 인상을 보정하기 위해 PN Grass 8종을 `Dense Sward`와 `Tall Seedhead Accent`로 재구성했다. Forest/Riparian 생산 Graph, 두 생산 계층의 64개 Actor, River Mesh/Flow Map은 변경하거나 재생성하지 않았다.

| Test | Result | 확인값 |
|---|---|---:|
| Engine | Pass | `5.7.4-51494982+++UE5+Release-5.7` |
| 신규 Asset audit | Pass | `715` assets 분류 |
| PN Grass Static Mesh | Pass | `110`개, 선택 8개 모두 4 LOD |
| Greasewood direct PCG use | Rejected | 로드 가능한 작은 Mesh 13개는 완성 관목이 아닌 PVE 조립 부품 |
| Huckleberry Oak compatibility | Failed/unused | UE5 custom version이 5.7.4보다 새로워 package load 불가 |
| Wood Sorrel / Mossy props | Audited/not deployed | 기술 후보지만 이번 Grass 전용 분기에 혼합하지 않음 |
| Dedicated sampling split | Pass | `PCG_ENV_RiparianGroundCoverSampling` 생성 |
| Shared sampler preservation | Pass | `PCG_ENV_ForestRegionSampling=0.0016 points/m²` 유지 |
| Prototype sampler | Pass | `0.75 points/m²` |
| Riparian/Water constraints | Pass | Influence 보존, Water hard exclusion 보존 |
| Two-layer graph | Pass | Base/Accent 각각 Select, Bounds, Self Pruning, World Raycast, Variation, Spawner |
| Grass meshes/weights | Pass | Dense 4종 `4/4/3/3`, Accent 4종 `4/3/3/2` |
| Patch threshold | Pass | `0.68..1.0` |
| Dense spacing/scale | Pass | `0.5 m`, non-uniform `(1.25..1.75, 1.25..1.75, 1.15..1.55)` |
| Accent spacing/scale | Pass | `0.8 m`, uniform `1.10..1.55` |
| Stored result | Pass | Dense `49,200`, Accent `15,133`, 합계 `64,333`, 셀 밖 `0` |
| Patch concentration | Pass | 25 m cell 최대 `580`, 점유 셀 중앙값 `162`, 100개 이상 셀 `174` |
| ISM policy | Pass | 8 components, NoCollision, CastShadow Off, Dense cull `50..350 m`, Accent cull `80..500 m` |
| Prototype generation time | Pass | commandlet에서 약 `20.35 s` 이내 |
| Forest production preservation | Pass | `64` actors 유지 |
| Riparian tree production preservation | Pass | `64` actors 유지 |
| River production preservation | Pass | `20` actors 유지 |
| River V28 speed readback | Pass | `Primary=-0.024`, `Detail=-0.052`, 20/20 |
| Fresh-process save/reload | Pass | 두 branch/8 mesh/64,333 instances/cull/settings 재확인 |
| Map Check invocation | Pass | `MAP CHECKDEP NOCLEARLOG` 실행 |
| Offscreen visual | Pass with limitation | 좁고 조밀한 군락과 군락 사이 열린 지면 확인; offscreen 조명에서는 재질 색이 검거나 노랗게 왜곡되어 실제 색 판정 불가 |
| Global Ground Cover deployment | Not performed | 대표 경로 성능 측정 후 결정 |
| Packaged/offline test | Pending | 최종 계층 확정 뒤 수행 |

초기 315-instance 결과는 배치 파이프라인은 정상이나 하층 식생으로 지나치게 성겼다. V28/V29는 가시성을 높였지만 한 계층의 네 종이 비슷한 간격과 scale을 공유해 한 종류가 균등하게 흩어진 것처럼 보였다. V30은 두 층을 만들었지만 넓고 낮은 바닥층이 요구한 좁은 밀집 군락으로 충분히 읽히지 않았다. V31은 hard constraint와 접지 구조를 바꾸지 않고 patch threshold를 높여 허용 면적을 줄이는 동시에 sampler density와 층별 밀도를 높였다. 결과적으로 V30보다 총 instance는 `17,715`개 줄이면서 군락 내부의 25 m 최대 밀도는 `196`에서 `580`으로 높였다. 전역 확대는 단순 64배 적용하지 않고, 대표 드론 경로에서 GPU와 masked overdraw를 확인한 뒤 셀 선별과 production density를 결정한다.

`Shrub_Huckleberry_Oak`은 프로젝트에 존재하지만 현재 엔진에서 사용할 수 없는 Asset이다. 삭제나 변환은 이번 단계 범위에 포함하지 않았고 어떠한 생산 참조도 만들지 않았다. 사용하려면 UE 5.7.4 호환 버전으로 다시 받아야 한다. `Shrub_Greasewood`는 Procedural Vegetation Editor에서 완성 Static Mesh로 export한 뒤에만 관목 후보로 재검증한다.

## 23. Forest Canopy / Near-Bank Layers V32 검증

2026-08-29에 항공뷰에서 Forest가 지나치게 작고 성기게 읽히는 문제와, 하천변 Grass가 넓게 균등 분산되는 문제를 서로 독립적으로 조정했다. River Surface/Flow, 생산 Riparian tree 64 Actor, `.uproject`는 변경하지 않았다.

| Test | Result | 확인값 |
|---|---|---:|
| Engine | Pass | `5.7.4-51494982+++UE5+Release-5.7` |
| Forest Graph reuse | Pass | 신규 Graph/Plugin 없이 `PCG_ENV_ForestRegion` 수정 |
| Forest spacing | Pass | `16 m → 15 m`, bounds `±750 cm` |
| Forest scale | Pass | uniform `1.22..1.55` |
| Forest species policy | Pass | A/B/C/D `5/5/1/1` 유지 |
| Forest generation callbacks | Pass | `64/64`, 빈 tile `0`, ISM `4/tile` |
| Forest stored result | Pass | `367,702`, V31 기준 대비 `+16,305` (`+4.64%`) |
| Forest mesh counts | Pass | A `153,336`, B `153,226`, C `30,646`, D `30,494` |
| Forest fresh reload | Pass | bounds/scale/weight/64 Actor/instance 수 재확인 |
| Near-bank gate | Pass | WaterExclusion 후 density `0.38..1.0` |
| Water hard exclusion topology | Pass | `WaterExclusion → Filter By Type → NearBankVegetationOnly` |
| Grass topology | Pass | Dense/Accent 각각 Bounds/Self Pruning/World Raycast/Variation/Spawner |
| Young Alder topology | Pass | 별도 Bounds/Self Pruning/WorldRaycast/Variation/Spawner |
| Grass mesh variety | Pass | Dense 4종 + Accent 4종, 각 mesh instance `591` 이상 |
| Young Alder variety | Pass | A/B/C/D `9/20/32/6` |
| Prototype stored result | Pass | Dense `10,983`, Accent `3,375`, Alder `67`, 총 `14,425` |
| Patch concentration | Pass | 25 m cell 점유 `79`, 최대 `580`, 중앙값 `139`, 100개 이상 `42` |
| Cell containment | Pass | 대표 7.56 km 셀 밖 instance `0` |
| Grass performance policy | Pass | 8 ISM, NoCollision, CastShadow Off, 350/500 m cull |
| Alder shadow policy | Pass | 4 ISM, NoCollision, CastShadow On, 1,000 m cull |
| Prototype generation time | Pass | 약 `19.21 s` |
| Actor preservation | Pass | Forest `64`, Riparian tree `64`, Prototype `1`, River `20` |
| Fresh-process save/reload | Pass | Graph 33 nodes, 12 mesh counts, cull/collision/shadow 정책 재확인 |
| Map Check invocation | Pass | `MAP CHECKDEP NOCLEARLOG` 실행 |
| Editor/PIE visual approval | Pending | 사용자의 실제 카메라 거리에서 확인 필요 |
| Live performance | Pending | `stat unit`, `stat gpu`, masked overdraw 미측정 |
| Global lower-layer deployment | Not performed | 승인 후 하천 교차/대표 경로 셀만 선별 |
| Packaged/offline test | Pending | 최종 전역 계층 확정 뒤 수행 |

Grass 총수가 V31의 `64,333`에서 `14,425`로 감소했지만 25 m 최대 군락 밀도는 `580`을 유지했다. 즉, 군락 내부를 성기게 만든 것이 아니라 Outer riparian zone을 제거해 Grass를 물가에 가까운 Near zone으로 압축한 결과다. Young Alder는 기존 Black Alder 완성 Mesh를 작은 scale로 재사용해 별도의 호환 불명 Asset이나 C++ 의존성을 만들지 않았다.

Forest의 hard-water exclusion Graph는 보존됐지만 spacing 변경 뒤 367,702개 origin 전체를 Water mask와 다시 비교하는 전역 CSV 검사는 이번 빠른 시각 조정 단계에서 반복하지 않았다. 따라서 V25의 overlap `0/0`은 과거 회귀 기준이며 V32의 신규 수치 결과로 보고하지 않는다. 실제 전역 배포 전에는 이 검사와 대표 드론 경로 성능 검사를 한 번 수행해야 한다.

## 24. Forest Density / Grass Visibility V33 검증

2026-08-29에 사용자가 요청한 “나무 크기는 유지하고 한 구역의 나무 수만 증가”와 “Grass 간격은 유지하고 드론 시점 가시 크기를 소폭 증가”를 기존 Graph 안에서 분리 적용했다. River Surface/Flow, Landscape, Water mask, Forest spacing, tree scale, seed와 species weight는 변경하지 않았다.

| Test | Result | 확인값 |
|---|---|---:|
| Engine | Pass | `5.7.4-51494982+++UE5+Release-5.7` |
| Forest sampling density | Pass | `0.0016 → 0.0020 points/m²` |
| Forest spacing preservation | Pass | `15 m`, bounds `±750 cm` |
| Forest scale preservation | Pass | uniform `1.22..1.55` |
| Forest species preservation | Pass | A/B/C/D `5/5/1/1` |
| Forest generation callbacks | Pass | `64/64`, 빈 tile `0`, ISM `4/tile` |
| Forest stored result | Pass | `419,965`, V32 대비 `+52,263` (`+14.22%`) |
| Forest mesh counts | Pass | A `175,367`, B `174,881`, C `34,892`, D `34,825` |
| Forest visual budget guard | Pass | 자동화 허용 범위 `400,000..500,000` 안 |
| Dense Sward scale | Pass | `(1.55..2.10, 1.55..2.10, 1.50..2.05)` |
| Tall Seedhead scale | Pass | uniform `1.45..2.05` |
| Grass spacing preservation | Pass | Dense `0.4 m`, Accent `0.65 m` |
| Grass distribution preservation | Pass | Dense `10,983`, Accent `3,375`, Alder `67` |
| Grass cull | Pass | Dense `50..500 m`, Accent `80..700 m` |
| Component policy | Pass | Grass NoCollision/Shadow Off; Alder NoCollision/Shadow On |
| Fresh-process save/reload | Pass | Forest/Grass Graph 값, Mesh reference, 수량, cull 정책 재확인 |
| Production Riparian readback | Recorded | `64` Actor, `35,523` instances, nonempty `60/64` |
| Map Check invocation | Pass | Grass 저장 commandlet에서 실행 |
| Editor/PIE visual approval | Pending | 실제 드론 높이에서 Forest 밀도와 Grass 크기 확인 필요 |
| Live performance | Pending | Forest `+14.22%`와 Grass cull 확대 후 `stat unit`/`stat gpu` 미측정 |
| Packaged/offline test | Pending | 최종 전역 계층 확정 뒤 수행 |

Forest regeneration용 첫 실행은 unrelated Riparian 수량이 지연 생성으로 변한 것을 실패로 오판해 Graph를 복구하고 Map을 저장하지 않았다. 이 검사는 Forest-only 작업의 성공 조건으로 부적절하므로 두 번째 실행에서는 Forest의 64 callback, Graph 값, 수량 budget, 빈 tile, ISM 구성만을 저장 gate로 사용했다. 현재 저장된 Map을 새 UE 프로세스에서 다시 읽었을 때 생산 Black Alder 수량은 `35,523`이었다. Riparian Graph Asset 자체는 V33 스크립트가 수정하지 않았지만 이전 문서값 `34,137`과 수량이 다르므로, 향후 성능 기준은 현재 readback을 사용한다.

V33은 사용자가 요청한 시각 밀도 조정이며 최적화 완료 판정이 아니다. Forest 증가는 instance 관리, 수관 shadow coverage와 masked foliage cost를 올릴 수 있고, Grass cull 연장은 중거리 masked overdraw를 늘릴 수 있다. 실제 드론 경로에서 통과한 뒤에만 이 값을 고정하고, 전역 lower-layer 배포 전에는 보이는 하천 셀을 선별한다.

## 25. Selective Riparian Lower-Layer Production V34 검증

2026-08-29에 V33 단일 셀 하층 식생을 64개 전역 셀로 단순 복제하지 않고, 저장된 River Surface가 존재하는 활성 셀만 선별 배포했다. Forest, 생산 Black Alder tree, River Surface/Flow, Landscape, mask Texture와 `.uproject`는 변경하지 않았다.

| Test | Result | 확인값 |
|---|---|---:|
| Engine | Pass | `5.7.4-51494982+++UE5+Release-5.7` |
| Active river cell discovery | Pass | `20` cells |
| Normal production actors | Pass | `18` full-cell actors |
| X04_Y01 quadrant diagnosis | Pass | Q01 `7,826`, Q00/Q10/Q11 `0` |
| X04_Y01 bounded production | Pass | Q01 actor `1`, full-cell actor `0` |
| Empty X05_Y00 handling | Pass | generated `0`, saved actor `0` |
| Saved lower-layer actors | Pass | exact `19` |
| Saved instances | Pass | exact `642,794` |
| Dense / Accent / Young Alder | Pass | `490,853 / 149,289 / 2,652` |
| Approved mesh set | Pass | Dense 4 + Accent 4 + Alder 4 = `12` |
| Graph reference | Pass | `19/19 PCG_ENV_RiparianLowerLayerPrototype` |
| Generation policy | Pass | `19/19 Generate On Demand`, Generate On Drop Off |
| Actor alignment | Pass | 18 full cells source-aligned, Q01 bounded offset/scale 일치 |
| Collision policy | Pass | 12 mesh 계층 모두 NoCollision |
| Shadow policy | Pass | Grass Off, Young Alder On |
| Cull policy | Pass | Dense `50..500 m`, Accent `80..700 m`, Alder `100..1,000 m` |
| Fresh-process save/reload | Pass | cell generation report와 actor별 mesh count 정확히 일치 |
| Map Check | Pass | 오류 `0`, 경고 `0` |
| Runtime dependency | Pass | 저장 ISM만 사용; runtime PCG/GIS/network 없음 |
| Live drone-view performance | Pending | `stat unit`, `stat gpu`, ProfileGPU 미측정 |
| Packaged/offline test | Pending | 최종 환경 통합 뒤 수행 |

일반 다중 셀을 한 Unreal Editor 프로세스에서 연속 생성하면 첫 셀 이후 PCG completion이 불안정한 현상이 있어, 각 셀을 독립 Editor 프로세스로 생성하고 저장했다. 이 방식은 느리지만 각 셀이 원자적으로 저장되고 실패한 셀이 다음 셀을 오염시키지 않는다. `X04_Y01` 전체 셀은 5분 이상 완료되지 않아 Transform/Bounds/입력 Actor를 점검했으며 이상이 없었다. 2×2 quadrant 검사에서 유효 데이터가 Q01에만 존재함을 확인해 그 영역만 저장했다.

fresh reload 검증은 이미 해결된 접지·회전·Forest 배치를 반복하지 않고, 이번 변경 범위인 Actor label/folder/transform, Graph, trigger, exact mesh count, Collision/Shadow/Cull만 검사했다. 검증 보고서는 `work/riparian_lower_layer_production_validation_v34.json`에 있으며 성공 상태와 오류 목록 `[]`를 기록한다.

프로젝트 Asset Registry에는 사용하지 않는 `Shrub_Huckleberry_Oak` 패키지가 UE 5.7.4보다 새로운 custom version으로 저장됐다는 기존 오류가 남아 있다. V34 Graph와 Actor는 이 Asset을 참조하지 않으며 이번 배포 오류가 아니다. 최종 cook 전에 호환 버전으로 교체하거나 미사용 패키지를 별도 정리해야 한다.
## 26. Forest High-Suitability Prototype / Drone Optimization Audit V35

### 구현 범위

- 생산 Forest Graph와 생산 Sampling Subgraph는 변경하지 않았다.
- 시험 Graph `/Game/Environment/PCG/Graphs/PCG_ENV_ForestHighSuitabilityPrototype`을 추가했다.
- 시험 Sampling `/Game/Environment/PCG/Subgraphs/PCG_ENV_ForestHighSuitabilitySamplingPrototype`을 추가했다.
- `PCG_ENV_Forest_Y00_X00` 한 셀만 시험 Graph를 사용한다.
- Suitability Density `0.65` 이상인 영역에만 12m 간격을 허용하고, 나머지는 기존 15m를 유지했다.

### 자동 검증 결과

| 검사 | 결과 |
|---|---:|
| Baseline Count | `7,258` |
| V35 Count | `8,272` |
| Increase | `1,014 / 13.9708%` |
| A/B/C/D | `3,437 / 3,414 / 716 / 705` |
| Minimum XY Center Spacing | `1,215.388cm` |
| Pairs Below 10m | `0` |
| 다른 63개 Forest Tile 변경 | `없음` |
| Tree Collision | `Off` |
| Tree Shadow | `On` |
| Map Check | `Error 0 / Warning 0` |

저장 후 두 차례 재생성의 인스턴스 수는 모두 `8,272`였고 Transform Hash도 다음 값으로 동일했다.

`cf8c003e84a713232b7752adf9f6d7a81dc8b6886b0369783dc49fe040c21600`

검증 원본은 다음 로컬 보고서에 있다.

- `work/forest_high_suitability_prototype_v35_report.json`
- `work/forest_high_suitability_determinism_v35.json`
- `work/forest_suitability_optimization_audit_v35.json`
- `work/drone_distance_optimization_audit_v35.json`

### 드론 시점 최적화 감사

- 실제 Pawn Camera FOV `90°`
- Spring Arm Target Length `10cm`
- Forest 생산 인스턴스 총계 `420,979`
- Forest Actor `64`, ISM Component `256`
- Forest ISM Cull Distance `0/0`
- Forest Shadow `On`
- World Partition, HLOD, Data Layer 미사용
- Tree A/B/C/D는 Nanite 사용, 전통적 LOD는 각 1개

이 상태는 “거리 최적화가 완료됨”을 의미하지 않는다. 현재 확인된 것은 최적화 입력과 비용 후보이다. 실제 드론 비행 중 Frame Time 측정, 시험 Cull Distance 적용, Standalone/Cook/Package/Offline 검증은 아직 수행하지 않았다.

### 판정

- V35 단일 셀 생성 및 결정성: 통과
- 기존 63개 Forest Tile 보존: 통과
- 전역 V35 배포: 보류, 드론 시점 육안 승인 필요
- 거리 최적화: 설계 및 기준 감사 완료, 실제 Parameter 시험 전
- 패키징/오프라인 실행: 최종 단계에서 별도 검증 필요

## 27. Riparian Lower-Layer Cost Reduction V37 Prototype

### 감사 결과

`Daejeon_PCG_Work`에 저장된 하천변 하층 식생은 Prototype을 포함해 Actor `20`, instance `657,219`개였다. 생산 Actor `19`개만 합산하면 `642,794`개이며, 이 중 Grass가 `640,142`개다. Grass는 이미 clump Static Mesh와 ISM을 사용하고 Collision/Shadow가 꺼져 있으므로, 새 대형 합본 Mesh보다 point 수와 masked overdraw를 먼저 줄이는 편이 더 작은 변경이다.

정적 비용 감사에서 기존 Prototype은 `14,425`개였고 `instance × LOD0 triangle` 추정치가 `6,078,272`였다. 이 수치는 draw call, 화면 점유율, material overdraw, GPU culling을 포함한 실제 Frame Time이 아니라 동일 자산 구성의 상대 비교용 proxy이다.

### 적용 결과

| 검사 | 결과 |
|---|---:|
| Prototype total | `14,425 → 9,558` |
| Instance reduction | `4,867 / 33.74%` |
| Dense | `10,983 → 7,094` |
| Accent | `3,375 → 2,397` |
| Young Alder | `67 → 67` |
| `instance × LOD0 triangle` | `6,078,272 → 3,208,107` |
| Triangle-work proxy reduction | `47.22%` |
| Mesh variants | `12/12` present |
| Grass LOD / Collision / Shadow | `4 LOD / Off / Off` |
| Other lower-layer production actors | `19/19 unchanged` |
| Map Check | `Error 0 / Warning 0` |

지상과 드론 시점 자동 캡처에서 Grass 군락의 연속성과 빈 공간이 함께 유지되는 것을 확인했다. 단일 셀 생성 시간은 약 `18.36s`, fresh validation 두 번은 각각 약 `22.40s`, `21.14s`였다. 따라서 이번 변경은 렌더링 비용 proxy를 줄였지만 PCG Editor 생성 시간이 유의하게 줄었다고 판정하지 않는다.

### 결정성 및 UE 5.7.4 제한

서로 다른 fresh UnrealEditor-Cmd process에서 생성한 두 결과는 모두 total `9,558`, 계층/mesh별 수량 동일, Transform hash 동일이었다.

`5fa6f32251068e7f35cedecc9f227d5090c3c6971679f91164953435d3c213ee`

한 process에서 같은 대형 PCGVolume을 즉시 두 번 `Cleanup → Generate`한 시험에서는 두 번째 Surface Sampler가 `42,869,756`개 후보 safeguard로 중단됐다. 실패한 빈 결과는 Level에 저장하지 않았다. 이는 fresh process 간 난수 불일치가 아니라 반복 재생성 경로의 Editor 제한이므로, 생산 배치는 셀별 단일 generation pass와 fresh reload readback을 사용한다.

검증 원본은 다음 로컬 보고서와 캡처에 있다.

- `work/riparian_lower_layer_performance_audit_v37.json`
- `work/riparian_lower_layer_optimized_prototype_v37.json`
- `work/riparian_lower_layer_optimized_fresh_run_v37_A.json`
- `work/riparian_lower_layer_optimized_fresh_run_v37_B.json`
- `work/riparian_lower_layer_v37/render/riparian_lower_layer_v37_ground.png`
- `work/riparian_lower_layer_v37/render/riparian_lower_layer_v37_drone.png`

### 판정

- 단일 셀 시각 밀도와 군락 구조: 자동 캡처 기준 통과, 최종 사용자 육안 확인 권장
- Instance/정적 triangle-work proxy: 감소 확인
- 독립 process 결정성: 통과
- Prototype 단계의 다른 하층 생산 Actor와 원본 Graph 보존: 통과
- 실제 드론 비행 Frame Time/GPU overdraw: 미측정
- 선택 생산 승격: 고부하 4셀만 후속 V38 통합 단계에서 적용
- Standalone/Cook/Package/Offline 실행: 최종 단계에서 별도 검증 필요

## 28. Forest Open-Water Exclusion V36

### 원인

`PCG_ENV_Forest_Y00_X00`의 기존 `WaterExclusion` 노드와 연결은 정상이며 현재 활성 Water mask 내부 수목 중심도 `0`개였다. 그러나 활성 입력 Texture가 OSM Full Water Area의 `정안저수지`(`water=reservoir`, way `119122441`)를 포함하지 않았다. 즉 PCG Difference 오류가 아니라 VisualMatch 기반 입력 마스크의 feature 누락이었다.

OSM 폴리곤 `374 pixel` 중 독립 위성 수면 후보와 `156 pixel`이 교차했고, 기존 활성 Water mask와의 교차는 `0 pixel`이었다. 수역 본체 안에는 V35 수목 중심 `65`개, 2-pixel 안전 여유까지 포함하면 `154`개가 있었다. 실제 PCG Bilinear 경계까지 적용한 V36 재생성에서는 `174`개가 제거됐다.

두 번째 어두운 표시 영역은 OSM reservoir/lake 폴리곤과 위성 수면 후보의 동시 근거가 없어 제외 대상에서 보류했다. 이는 산 그림자를 수면으로 오판해 정상 산림을 지우지 않기 위한 보수적 판정이다.

### 적용 및 검증

| 검사 | 결과 |
|---|---:|
| V35 → V36 Count | `8,272 → 8,098` |
| Removed | `174 / 2.1035%` |
| A/B/C/D | `3,343 / 3,359 / 700 / 696` |
| V36 Mask 내부 잔존 수목 중심 | `0` |
| Minimum XY Center Spacing | `1,215.388cm` |
| Pairs Below 10m | `0` |
| 다른 63개 Forest Tile 변경 | `없음` |
| Collision / Shadow / Cull | `Off / On / 0..0` 유지 |
| 재생성 2회 Count | `8,098 / 8,098` |
| 재생성 2회 Transform Hash | 동일 |
| Map Check | `Error 0 / Warning 0` |

결정성 해시는 다음과 같다.

`8e8b2631a740f20da07fe852e3545379cc12649faf178ab72c3ef009e160e589`

검증 원본은 다음 로컬 보고서에 있다.

- `work/forest_open_water_feature_analysis_v36.json`
- `work/forest_open_water_prototype_v36_report.json`
- `work/forest_open_water_determinism_v36_report.json`
- `work/forest_open_water_saved_overlap_v36.json`

### 판정

- 정안저수지 수목 오배치: 수정 및 자동 검증 통과
- V35 산림 덩어리/간격 보존: 통과
- 다른 산림 타일과 생산 Graph 보존: 통과
- 사용자 표시 지점 육안 확인: 최종 Editor 확인 권장
- 패키징/오프라인 실행: 최종 단계에서 별도 검증 필요

## 29. River Short-Gap V38 / Riparian V37 Production

### River 원인과 보정

`X00_Y00` 단절은 좌표계, actor transform, 타일 경계 또는 Water Material 오류가 아니었다. 원본 River Area raster에는 짧게 분리된 component가 있었고, VisualMatch River Centerline은 그 공백을 연속 통과했다. 전역 검사에서 같은 조건을 만족한 `<97.5m` 공백은 네 곳뿐이었다.

| 검사 | 결과 |
|---|---:|
| 전역 검출 공백 | `4` |
| 대상 타일 | `X00_Y00`, `X04_Y04` |
| 추가 mask cell | `23` |
| Connected components | `24 → 20` |
| Inverted / degenerate triangles | `0 / 0` |
| V38 mesh actor | `2/2` |
| 나머지 V23 actor | `18/18` |
| V24 tile Material Instance | `20/20 exact` |
| V24 Flow Map texture | `20/20 exact` |
| Collision / Shadow | `20/20 Off / Off` |
| Map Check | `Error 0 / Warning 0` |

`X00_Y00`의 두 연결부는 별도 Unreal high-resolution capture로 확인했다. 연결부는 연속이며, 약 `30m/pixel` 원본 해상도 때문에 국소 bank가 각져 보일 수 있다. 중심선 근거가 없거나 긴 공백은 오탐 방지를 위해 연결하지 않았다.

### V37 고부하 생산 셀

| Actor | Before | After | Instance 감소 | LOD0 triangle-work proxy 감소 |
|---|---:|---:|---:|---:|
| `Y02_X05` | `162,895` | `108,746` | `33.24%` | `46.95%` |
| `Y04_X07` | `94,566` | `63,100` | `33.27%` | `47.07%` |
| `Y05_X03` | `66,350` | `44,563` | `32.84%` | `46.58%` |
| `Y05_X07` | `50,523` | `33,745` | `33.21%` | `47.09%` |
| 합계 | `374,334` | `250,154` | `124,180 / 33.17%` | 셀별 약 `46.6–47.1%` |

첫 생산 셀 `Y02_X05`는 저장 후 fresh process에서 다시 생성했다. total `108,746`, 계층/mesh별 수량, 다른 Actor 수량과 모든 instance transform digest가 정확히 일치했다.

`a5f9a16a8cefecd95756f08d1dc7a7c4f5b75337297dfdf2d1f2a09e7d4b054e`

첫 네 셀 batch가 통과한 뒤 나머지 `15`개 생산 영역을 각각 fresh process의 단일 generation pass로 처리했다. 모든 셀이 `25%` 이상 instance 감소, `45%` 이상 시각 밀도 보존, `35%` 이상 LOD0 triangle-work proxy 감소, 12개 승인 mesh variant, Young Alder 수량 보존, 다른 actor 불변, seed와 category 정책 보존 기준을 통과한 경우에만 저장됐다. 특수 분할 actor `PCG_ENV_RiparianLowerLayer_Y01_X04_Q01`도 정확한 label을 지정해 같은 검사를 통과했다.

최종 fresh-process 통합 감사 결과 생산 하층 Actor는 `19`, 저장 instance는 `429,498`이고 V37 `19`셀/원본 Graph `0`셀 구성이 정확했다. Prototype `9,558`개를 포함한 총계는 `439,056`이며, 변경 전 `657,219`개보다 `218,163 / 33.19%` 적다. 전체 LOD0 triangle-work proxy는 `277,007,269 → 147,103,423`, 즉 `46.90%` 감소했다. 모든 셀은 non-empty이며 승인 mesh만 사용하고, Grass는 NoCollision/Shadow Off, Young Alder는 NoCollision/Shadow On, category cull 정책을 유지한다. `MapCheck`는 Error `0`, Warning `0`이다.

사용자가 찾지 못한 `Y02_X05`, `Y04_X07`, `Y05_X03`, `Y05_X07`은 삭제되거나 숨겨진 것이 아니었다. 네 actor 모두 `Environment/PCG/Production/Riparian/LowerLayer`에 존재했고 Editor Hidden은 False, 저장 instance와 visible ISM component가 확인됐다. 최종 전역 감사에서도 생산 Actor `19/19`가 모두 같은 Production 폴더에 있고 Editor Hidden `0`, ISM visible/hidden-in-game 정책 정상임을 확인했다. Prototype은 별도 `Environment/PCG/Riparian/Prototype` 폴더에 있으므로 Prototype 폴더만 펼치면 하나만 보인다.

검증 원본:

- `work/river_surface_short_gap_bridges_v38_plan.json`
- `work/river_surface_short_gap_bridges_v38_flow_repair.json`
- `work/river_surface_v38_render/river_surface_v38_X00_Y00_A.png`
- `work/river_surface_v38_render/river_surface_v38_X00_Y00_B.png`
- `work/riparian_lower_layer_deploy_v37/*.json`
- `work/riparian_lower_layer_validate_v37/X05_Y02.json`
- `work/riparian_actor_visibility_audit_v39.json`
- `work/river_riparian_postfix_audit_v38.json`

### 판정

- 표시된 X00 단절과 같은 입증 가능한 짧은 공백: 수정 완료
- 기존 타일별 흐름 방향·속도 재질: 보존 확인
- 강변 하층 식생 Production 19셀 + Prototype: V37 최적화 및 저장 완료
- Actor 존재·Outliner 폴더·Editor/ISM 가시성: 검증 통과
- 실제 드론 비행 Frame Time / GPU overdraw: 미측정
- Standalone/Cook/Package/Offline 실행: 최종 환경 통합 뒤 별도 검증 필요

## 30. V46 Riparian Coverage / Global Forest Suitability Validation

### 강변 풀

V45의 저장 결과를 기준으로 `DenseSwardVariation`과 `TallSeedheadVariation`의 XY scale만 `1.42배` 확대했다. 새 UE 5.7.4 process에서 맵을 다시 열어 다음을 확인했다.

| 검사 | 결과 |
|---|---:|
| Target Graph | `PCG_ENV_RiparianUltraLushPrototypeV46` |
| Total instances | `176,583` |
| Grass / Young Alder | `176,170 / 413` |
| XY coverage proxy | `2.0164×` |
| Grass Collision / Shadow | `Off / Off` |
| Alder Collision / Shadow | `Off / On` |
| Ground / Drone capture | `2/2 생성 성공` |

Point 위치, bank mask, water exclusion과 sampling은 V45에서 변경하지 않았다. 따라서 수면 안전 구역과 접지 경로는 보존된다. 지상 캡처에서는 서로 겹치는 연속적인 초본층이 확인됐으며 드론 캡처에서도 동일 bank patch가 표시됐다.

### 산림

`Y00_X01`에서 생산 V46 Graph를 먼저 검증했다. 수목은 `7,744 → 8,724`개로 `12.65%` 증가했고 최소 XY 중심 간격은 `1,208.47cm`, `10m` 미만 pair는 `0`이었다. 이후 남은 `62`개 공용 생산 셀을 순차 재생성했다. `Y00_X00`의 V36 수면 제외 Graph와 수목은 별도 경로로 보존했다.

| 검사 | 결과 |
|---|---:|
| Forest Actor | `64/64` |
| Shared V46 / Special V36 Graph | `63 / 1` |
| Promotion callbacks | `62/62` |
| Forest total before / after | `421,785 / 475,313` |
| Increase | `53,528 / 12.69%` |
| Empty tiles | `0` |
| Min / max instances per tile | `2,131 / 13,809` |
| A/B/C/D variants | 모두 존재 |
| Collision / Shadow | `Off / On` 유지 |
| Water mask / WaterExclusion topology | 보존 확인 |
| Riparian instance change during Forest deploy | `없음` |
| Clean-reopen validation | 통과 |

검증 원본:

- `work/riparian_ultralush_v46.json`
- `work/riparian_ultralush_v46/render/capture_report.json`
- `work/forest_high_suitability_production_v46_build.json`
- `work/forest_high_suitability_global_v46.json`
- `work/environment_density_v46_validation.json`

이번 검증은 정적 Graph/Asset 검사, Editor PCG 재생성, 저장, fresh-process 재열기와 오프스크린 렌더까지 포함한다. 실제 드론 비행 Frame Time, Cook/Package, Standalone 및 네트워크 차단 상태 실행은 아직 수행하지 않았다.

## 31. River Grounding / Riparian Scale V47

### 강 접지

`ENV_RiverSurface_Production_X03_Y01`은 V42 Grounded Mesh와 V24 방향성 Flow Material을 유지한 채 Actor Z만 `0 → -35cm`로 조정했다. V42 접촉 감사값에 offset을 적용한 예측 gap은 최소 `-10.0cm`, p01 `19.79cm`, 중앙 `34.32cm`, p99 `47.84cm`다. 국소적으로 최대 10cm 지형에 숨기는 대신 사용자 표시 경계의 검은 틈을 줄이는 선택이다.

| 검사 | 결과 |
|---|---:|
| Target mesh/material 보존 | 통과 |
| Target Z 저장/재로드 | `-35cm`, 통과 |
| 다른 River Actor 변경 | `0 / 19` |
| Collision / Shadow | `Off / Off` 유지 |
| Map 저장 및 fresh reload | 통과 |

### 강변 풀 크기

V47은 V46 대비 Grass XY `1.15배`, Z `1.20배`만 적용했다. 새 Actor나 instance는 추가하지 않았다.

| 검사 | 결과 |
|---|---:|
| Target Graph | `PCG_ENV_RiparianUltraLushTallPrototypeV47` |
| Total instances | `176,583` |
| Grass / Young Alder | `176,170 / 413` |
| Coverage-area proxy vs V46 | `1.3225×` |
| Height vs V46 | `1.20×` |
| Grass Collision / Shadow | `Off / Off` |
| Ground / Drone capture | `2/2 생성 성공` |
| Fresh-process V47 assignment/count | 통과 |
| Forest Actor/instance regression | `64 / 475,313`, 변경 없음 |

지상 렌더에서는 서로 겹치는 연속 초본층, 드론 렌더에서는 V46보다 커진 bank silhouette를 확인했다. 정적 instance 수는 동일하지만 XY/Z 확대로 masked overdraw가 증가할 수 있으므로 실제 드론 Frame Time과 GPU overdraw는 아직 미측정으로 남긴다.

검증 원본:

- `work/river_grounding_x03_y01_v47.json`
- `work/riparian_ultralush_v47.json`
- `work/riparian_ultralush_v47/render/capture_report.json`
- `work/riparian_ultralush_v47/render/riparian_ultralush_v47_ground.png`
- `work/riparian_ultralush_v47/render/riparian_ultralush_v47_drone.png`
- `work/environment_density_v47_validation.json`

Cook/Package, Standalone 실행, 실제 드론 경로 Frame Time 및 네트워크 차단 실행은 최종 통합 단계에서 별도 검증한다.

## 32. Global Riparian Production V49

생산 River `20`셀과 LowerLayer `20`셀을 1:1로 구성했다. `Y01_X03` V47 reference를 제외한 `19`셀은 공용 V49 전역 bank mask를 사용하며, fresh-process 재열기 결과 Grass `1,692,399`, Young Alder `3,961`, 합계 `1,696,360` instances가 저장됐다. 19개 V49 셀의 Grass center `1,516,229`개를 전수 검사해 water intrusion `0`, inactive-mask placement `0`을 확인했다. 승인 mesh와 Collision/Shadow/Cull/Generate On Demand 정책도 20셀 모두 통과했다.

Forest는 `64 actors / 475,313 instances`로 변하지 않았고 MapCheck는 errors `0`, warnings `0`이다. `X04_Y04` 대표 offscreen capture도 성공했다. 실제 드론 비행 Frame Time, GPU masked overdraw, Cook/Package, Standalone과 네트워크 차단 실행은 아직 미검증이다. 전체 수치와 셀별 결과는 `Docs/PCG_RIPARIAN_GLOBAL_V49.md`를 참조한다.

## 33. Conditional Far-Bank V50

### 원인

`X02_Y01`은 저장 Grass `59,476`개가 모두 유효한 bank mask에 있었지만 River bounds 중심에서 가장 가까운 Grass가 약 `1.36km` 떨어져 기존 `500/700m` Cull 밖이었다. 같은 거리 조건은 `X04_Y04`, `X07_Y05`에도 있었고, `X03_Y01`은 V47 local mask 때문에 near-bank 30m 연속성 누락 component가 4개 있었다.

### 결과

전체 LowerLayer의 Cull을 늘리지 않고 네 셀에 저밀도 FarBank Actor를 추가했다.

| Cell | V50 instances | Generation seconds |
|---|---:|---:|
| X02_Y01 | `7,296` | `4.196` |
| X03_Y01 | `20,868` | `4.374` |
| X04_Y04 | `26,509` | `4.502` |
| X07_Y05 | `19,091` | `4.476` |
| Total | `73,764` | - |

Fresh-load 결과 Actor `182`, FarBank `4`, LowerLayer `20 / 1,696,360`, Forest `64 / 475,313`으로 기존 계층은 변하지 않았다. 모든 FarBank component는 승인 Grass 8종, Collision Off, Shadow Off, Cull `1,800..2,200m`, Generate On Demand를 통과했다.

좌표 `73,764`개 전수 검사에서 mask `160`은 `37,404`, mask `255`는 `36,359`였다. 실제 water/outer-band 침범은 `0`이다. 한 점은 10m mask/float32 픽셀 경계에 양자화됐으며 인접 픽셀 density `255`와 Landscape 접지를 확인했다. V49+V50 결합 후 20셀 모든 near-bank texel은 30m 안에서 Grass center를 가져 누락 셀 `0`이다. X02_Y01 접지 `512/512`, river-centre offscreen capture, Map Check errors `0` warnings `0`을 통과했다.

실제 Drone Pawn 비행 FPS/CPU/GPU/overdraw, Cook/Package/Standalone/Offline은 아직 미검증이다. 상세 설계는 `Docs/PCG_RIPARIAN_FAR_BANK_V50.md`를 참조한다. 이번 변경과 무관한 `Megaplant_Library/Shrub_Huckleberry_Oak`의 newer custom-version Asset Registry 오류는 최종 Cook 전 별도 처리해야 한다.

## 34. X02 Far-Bank Dense V51

사용자 표시 구역의 공용 V50 FarBank `7,296`개를 `X02_Y01` 전용 V51 Graph로 재생성했다. 결과는 `29,563`개이며 배포 generation은 `5.693s`였다. 다른 세 FarBank Actor는 공용 V50 Graph와 기존 수량을 유지한다.

| 검사 | 결과 |
|---|---:|
| X02 V51 saved instances | `29,563` |
| Dense / Accent | `22,233 / 7,330` |
| Per-cell cap | `29,563 / 40,000`, 통과 |
| All FarBank actors / instances | `4 / 96,031` |
| LowerLayer actors / instances | `20 / 1,696,360`, 변경 없음 |
| Forest actors / instances | `64 / 475,313`, 변경 없음 |
| Fresh map actor count | `182`, 변경 없음 |
| Approved Grass / unknown | `8종 / 0` |
| X02 Ground Trace | `512/512`, 약 `0.0015cm` |
| Water/outer-band true intrusion | `0` |
| 20-cell near-bank coverage within 30m | `20/20`, 모두 `100%` |
| Collision / Shadow / Cull | `Off / Off / 1800..2200m` |
| Map Check | Error `0`, Warning `0` |

새 river-centre capture에서는 양쪽 bank가 끊긴 점무늬 대신 연속 초지 띠로 표시됐다. Capture 결과와 JSON audit는 성공했으나, 해당 offscreen commandlet는 `QUIT_EDITOR` 이후 engine shutdown에서 null access violation로 exit code `1`을 반환했다. 이 실행은 read-only였으며 맵 저장에는 영향을 주지 않았다. 별도의 fresh NullRHI 감사와 위치 전수 검사는 exit code `0`으로 통과했다.

실제 드론 비행 Frame Time, GPU masked overdraw, Cook/Package/Standalone/Offline 실행은 아직 미검증이다. 상세 수치와 구조는 `Docs/PCG_RIPARIAN_FAR_BANK_DENSE_X02_V51.md`를 참조한다.

## 35. Unified Riparian Bank V52

V49 LowerLayer와 V51 FarBank 위치를 10m grid에서 대조했다. `X02_Y01`, `X04_Y04`, `X07_Y05`의 FarBank centre는 20m 기준 100% 기존 LowerLayer와 중복이었다. `X03_Y01`은 85.70% 중복이지만 나머지 14.30%가 기존 V47 local mask의 실제 빈 구간을 보수하고 있었으므로 단순 삭제 대신 전역 V49 bank mask로 통합 재생성했다.

| Cell | Before Lower + Far | V52 Unified | Generation |
|---|---:|---:|---:|
| X02_Y01 | `89,166` | `58,533` | `6.941s` |
| X03_Y01 | `197,451` | `82,772` | `5.903s` |
| X04_Y04 | `197,465` | `106,569` | `5.807s` |
| X07_Y05 | `175,355` | `75,685` | `5.465s` |
| 합계 | `659,437` | `323,559` | - |

Fresh-load 결과는 Actor `178`, LowerLayer `20 / 1,456,513`, FarBank `0`, Forest `64 / 475,313`이다. 다른 16개 LowerLayer의 Graph와 Actor별 인스턴스 수, Forest 64개 Actor별 인스턴스 수는 모두 exact unchanged다. 대상 component는 승인 Grass 8종만 사용하며 Collision/Shadow Off와 Cull `180000..220000cm`를 통과했다.

Grass centre `1,453,896`개를 전수 검사했다. water intrusion과 inactive bank placement는 모두 0이다. 네 통합 셀은 모든 near-bank texel에서 10m 안에 Grass centre가 있고 전체 20셀은 30m 안 coverage가 100%다. `X03_Y01`의 10m coverage는 `91.39% → 100%`다. X04에서 323,559개 대상 centre 중 1개가 10m density 경계의 인접 outer active texel로 floor됐지만 water가 아니며 비율은 `0.00031%`다.

X02/X03 Ground Trace는 각각 `256/256`, terrain delta 약 `0.0015cm`로 통과했다. Offscreen 근접/항공 캡처도 생성됐다. 최신 저장 맵 Map Check는 Error `0`, Warning `0`이다.

맵 전체 기존 `LowerLayer 1,696,360 + FarBank 96,031 = 1,792,391`개는 V52 단일 LowerLayer `1,456,513`개가 되어 `335,878 / 18.74%` 감소했다. 이는 저장 인스턴스 감소이며 실제 FPS/GPU 개선률은 아직 측정하지 않았다. Cook/Package/Standalone/Offline도 최종 통합 단계의 미검증 항목이다. 상세 설계는 `Docs/PCG_RIPARIAN_UNIFIED_BANK_V52.md`를 참조한다.

## 36. Environment Production Audit / Rendering Optimization V55

### 확인된 이상과 수정

1. Forest `475,313`개가 Cull distance `0/0`으로 거리 제한 없이 렌더링되고 있었다. 배치와 그림자를 유지한 채 `3.5–5.0km` fade를 저장했다.
2. Riparian Grass `1,453,896`개는 자체 Cull을 갖지만 WPO disable distance가 `0`이었다. 근거리 바람은 유지하고 `250m` 밖 WPO를 중단했다.
3. Hardware Ray Tracing이 켜진 프로젝트에서 Riparian Grass 전체가 RT scene에 포함되어 있었다. Grass만 `VisibleInRayTracing=False`로 바꾸고 Forest/Alder는 유지했다.
4. Drone Pawn은 blocking sweep 이동을 사용하지만 Forest, Riparian, River 모두 `NoCollision`이며 overlap도 꺼져 있었다. 충돌 설정은 수정할 필요가 없었고 그대로 보존했다.
5. Tree material은 확인 가능한 wind parameter가 없고 현재 Static Mesh ISM 경로에 `DynamicWind` data가 없다. 검증되지 않은 전역 Tree wind는 적용하지 않았다.

### 저장 전·후 회귀 검사

| 검사 | 결과 |
|---|---:|
| Level actors | `178 → 178` |
| Forest actors / instances | `64 / 475,313`, exact unchanged |
| LowerLayer actors / instances | `20 / 1,456,513`, exact unchanged |
| River actors | `20`, exact unchanged |
| PCG graph assignments | exact unchanged |
| Component inventory / mesh / instance count | exact unchanged |
| Collision / overlap | `0` blocking instances, unchanged |
| Shadow / dynamic shadow / distance-field policy | exact unchanged |
| Forest unculled instances | `475,313 → 0` |
| Vegetation WPO unlimited instances | `1,931,826 → 0` |
| Grass visible in Hardware RT | `1,453,896 → 0` |
| Tree/Alder visible in Hardware RT | `477,930`, retained |
| Fresh-load Map Check | Error `0`, Warning `0` |

`X02_Y01` 대표 셀은 근접·항공 offscreen capture를 다시 생성했다. Grass `58,533`개가 유지됐고, Landscape trace 표본 `256/256`의 Z 차이는 약 `0.0015cm`, water/ground miss는 `0`이었다. Grass RT 제외 후 같은 항공 카메라에서도 식생 밀도와 bank silhouette의 눈에 띄는 손실이 없었다.

UE 5.7.4의 실제 실행 cvar도 fresh-load에서 확인했다. ISM RT culling `1`, cluster radius `10,000cm`, low-scale radius `1,000cm`, global RT culling `3`, per-instance `1`, global radius `30,000cm`, Nanite RT mode `0`이다. 엔진 자체의 거리/화면 크기 기반 RT culling이 이미 활성화되어 있으므로 Tree에 별도 RT cvar를 중복 적용하지 않았다.

이 검증은 정적 비용 원인을 제거했다는 증거이며 FPS 향상률을 뜻하지 않는다. 실제 드론 비행 경로의 Game/GPU Frame Time, masked overdraw, Cook/Package/Standalone/네트워크 차단 실행은 아직 측정하지 않았다.

### 남은 생산 위험

- 작업 맵은 약 `60.48km` Landscape이지만 World Partition/HLOD가 없다. 저장 ISM이 Level 로드 시 함께 존재하므로 메모리·초기 로드 비용은 거리 Cull만으로 해결되지 않는다.
- `DefaultEngine.ini`의 Game Default Map은 `/Game/Maps/Daejeon`이고 현재 작업 맵은 `/Game/Maps/Daejeon_PCG_Work`이다. 패키징 대상 맵을 확정해야 한다.
- `Megaplant_Library/Shrub_Huckleberry_Oak` 일부 자산의 newer custom-version 오류는 이번 환경 변경과 무관하지만 Cook blocker 후보로 남아 있다.
- Tree wind는 project-local leaf-only material prototype, 1km WPO 상한, GPU 측정 순으로 별도 검증해야 한다.

## 37. Y00_X02 Riparian Density Correction V56

`Y00_X02`는 `Y00_X03`과 같은 V49 Graph를 사용했지만 유효 bank 면적이 더 작아 `31,070` Grass로 보였다. 공용 Graph를 변경하지 않고 전용 Sampling Graph에서만 `0.10 → 0.20 point/m²`로 올렸다.

| 검사 | 결과 |
|---|---:|
| Y00_X02 Grass | `31,070 → 62,164` |
| Y00_X02 Alder | `87 → 133` |
| 다른 19개 LowerLayer | exact unchanged |
| Forest 64개 / 475,313 | exact unchanged |
| River 20개 | exact unchanged |
| Water intrusion | `0` |
| Instances outside cell | `0` |
| Active bank invalid centres | `0` |
| Near-bank coverage within 10m / 30m | `100% / 100%` |
| Landscape Ground Trace | `256/256`, 최대 차이 약 `0.0015cm` |
| Grass Collision / Shadow / RT | `Off / Off / Off` |
| Map Check | Error `0`, Warning `0` |

Fresh-load 근접·항공 캡처도 성공했다. 보정으로 추가된 LowerLayer는 `31,140`개로 기존 V55 LowerLayer의 약 `2.14%`이며, 공용 V49와 다른 셀의 비용은 증가시키지 않았다.

## 38. X02_Y01 River Flow Axis V57

### 원인

V24 생성기는 외부 centerline의 최근접 tangent를 Flow Map에 전달한다. `X02_Y01`에는 분리된 수로 두 개가 있고 큰 본류 쪽에서 다른 최근접 선분의 tangent가 선택되어 본류 장축과 거의 직각인 흐름이 만들어졌다. 공용 Material의 XY decode나 전체 좌표계 문제가 아니므로 전역 Material은 수정하지 않았다.

### 변경 및 검증

| 검사 | 결과 |
|---|---:|
| 대상 Actor | `ENV_RiverSurface_Production_X02_Y01` 한 개 |
| 큰 본류 water pixels | `1,380` |
| 교정 sampling pixels | `2,383` |
| 장축 정렬도 before / after | `0.141857 / 0.988330` |
| Water mask | pixel-exact 동일 |
| Sampling mask | pixel-exact 동일 |
| 비대상 Flow pixel 변경 | `0` |
| Primary / Detail Speed | `-0.024 / -0.052`, unchanged |
| Map UV Bias | unchanged |
| 다른 19개 River Material | exact unchanged |
| 모든 River Mesh / Transform | exact unchanged |
| Level / River Actor | `178 / 20`, unchanged |
| Collision / Shadow | 모든 River `Off / Off` |
| Fresh-load Map Check | Error `0`, Warning `0` |

정적 입력 검증은 Flow offset이 본류 장축을 따르는 것을 직접 확인한다. 시간에 따른 최종 움직임은 동일 V24 Parent Material의 `Time × Speed × FlowDirection` 경로를 그대로 사용한다. 따라서 속도와 표면 질감은 유지되고 X02_Y01 큰 본류의 방향만 교정된다. 실제 Editor realtime viewport에서의 최종 육안 확인은 정적 commandlet 검증과 별도로 수행한다.

이번 실행과 무관한 `Megaplant_Library/Shrub_Huckleberry_Oak` newer custom-version Asset Registry 오류는 계속 존재하며 최종 Cook 전 별도 처리해야 한다.

## 39. X02_Y00 River Flow Direction V58

### 재진단

직전 V57은 화면 형태만으로 `X02_Y01`을 추정해 적용했으나, 사용자가 다시 표시한 실제 Editor viewport를 저장 설정에서 읽어 Camera ray를 수면 Z와 교차시킨 결과 화면 중심은 `X02_Y00`이었다.

- Camera location: `(-815203.513458, -2276970.380344, 8976.236517)cm`
- Camera rotation: Pitch `-46.399900°`, Yaw `282.600027°`
- 수면 교차점: `(-813354.404718, -2285242.799000, 75.0)cm`
- 해당 Actor: `ENV_RiverSurface_Production_X02_Y00`

기존 Primary Normal `T_River_Waves01_Normals`은 `961×63`의 강한 띠 방향을 가진다. 이를 `T_Water_Normal_Subtle`과 `T_Water_Normal`로 임시 교체한 동일 카메라 캡처도 수행했지만, 흐름 질감이 과도하게 흐려져 생산 후보에서 제외했다. 임시 MI 변경은 저장 전에 원복했고 맵에는 남지 않았다.

### 최종 변경

`X02_Y00` 단일 water component의 PCA 장축으로 sampling RG vector를 정렬했다. 기존 부호를 유지하고 Water/Sampling mask와 비대상 pixel은 변경하지 않았다. 기존 Normal Texture와 유속은 보존했다.

| 검사 | 결과 |
|---|---:|
| 대상 Actor | `ENV_RiverSurface_Production_X02_Y00` 한 개 |
| Water / Sampling pixels | `729 / 1,795` |
| 장축 정렬도 before / after | `0.9366075 / 0.9999984` |
| Water mask | pixel-exact 동일 |
| Sampling mask | pixel-exact 동일 |
| 비-sampling RG | pixel-exact 동일 |
| Primary / Detail Speed | `-0.024 / -0.052`, unchanged |
| Map UV Bias | `(1.939453125, 3.939453125)`, unchanged |
| 다른 River Material | V24 `18`, V57 `1`, exact retained |
| 모든 River Mesh / Transform | exact unchanged |
| Level / River Actor | `178 / 20`, unchanged |
| Temporary diagnostic Actor | `0` |
| Collision / Shadow | 모든 River `Off / Off` |
| Fresh-load Map Check | Error `0`, Warning `0` |

같은 저장 viewport에서 V24 baseline, transient 후보, V58 후보와 저장 후 V58을 각각 8-frame offscreen sequence로 캡처했다. Specular 변화와 two-phase cyclic blend 때문에 단순 optical-flow 수치의 설명력이 낮아 그 값은 acceptance에 사용하지 않았다. 최종 acceptance는 source RG 장축 정렬, mask/parameter 불변성, target-only Material 변경, fresh-load asset readback과 Map Check로 구성했다.

`X02_Y01` V57은 자체 장축 오정렬 `0.141857 → 0.988330`을 고치는 유효한 별도 예외이므로 유지했다. 이번 수정은 새 Shader sample/branch, Actor, tick 또는 Runtime PCG를 추가하지 않는다.

Cook/Package/Standalone/오프라인 실행과 실제 Editor 사용자의 실시간 최종 육안 확인은 이번 국소 수정 단계에서는 수행하지 않았다. `Megaplant_Library/Shrub_Huckleberry_Oak` newer custom-version 오류도 기존 미해결 Cook 위험으로 남아 있다.

## 40. X02 Fixed-Axis River Motion V59

### 원인 재확인

V57/V58 Flow Map 자체는 장축에 정렬되어 있었지만 최종 렌더의 방향 인상은 Flow Map보다 `T_River_Waves01_Normals`의 Primary/Detail panning이 지배했다. 이 Texture는 `961×63`, No Mips의 강한 비등방성 패턴이므로 Flow Map만 수정해도 사용자가 표시한 파란 횡방향 움직임이 계속 보일 수 있었다.

Primary Normal을 90° 회전한 비파괴 후보도 동일 카메라에서 비교했으나 횡방향으로 분류되어 폐기했다. 최종 V59은 두 대상 타일의 기존 normal panning을 정지하고, 명시한 world-space 장축을 따르는 저강도 Sine 신호 하나만 움직인다.

### 결정론적 방향 검증

Headless Editor의 Material `Time`이 짧은 offscreen sequence에서 안정적으로 전진하지 않아 실제 생산값을 저장하지 않은 채 `LongitudinalPhaseOffset`을 `0.00 → -0.84`, `-0.12` 간격으로 8단계 이동했다. 방향 식별성을 위한 진단 Strength는 `0.16`이었고 종료 시 생산 Strength `0.085`, Speed `-0.015`, Phase Offset `0.0`으로 복구했다. 맵과 MI는 이 진단 과정에서 저장하지 않았다.

각 water-only ROI의 8-frame luminance를 `constant + cos(phase) + sin(phase)`로 pixel별 적합한 뒤 공간 phase gradient를 측정했다.

| 타일 ROI | Temporal fit R² 중앙값 | 화면 수직축 오차 | 수직/수평 gradient 비 |
|---|---:|---:|---:|
| `X02_Y01` upper | `0.972626` | `1.172991°` | `48.8391×` |
| `X02_Y00` lower | `0.987646` | `0.590483°` | `97.0286×` |

두 ROI의 phase gradient 부호도 일치했다. 따라서 V59 신호는 사용자가 파란색으로 표시한 화면 수평축이 아니라 빨간색으로 표시한 화면 수직/강 장축을 따른다.

### 저장·재로딩 회귀 검사

| 검사 | 결과 |
|---|---:|
| River Actor | `20` |
| V59 assignments | `X02_Y00`, `X02_Y01` 정확히 2개 |
| 다른 River Parent | V24 `18`개, exact retained |
| Target MI parent/scalar/vector/Flow Map | 모두 exact readback 통과 |
| Mesh/Transform/Collision/Shadow 변경 | `0` |
| Target Collision / Shadow | `Off / Off` |
| Temporary Actor | `0` |
| Fresh-process Map Check | Error `0`, Warning `0` |
| Material compile error | 확인되지 않음 |

최종 Parent는 V24의 `87` expressions에서 `111` expressions로 증가했지만 새 Texture sample, Dynamic Branch, Actor Tick 또는 Runtime generation은 없다. 추가 ALU는 두 대상 수면에만 적용된다. 시험용 Prototype MI 2개, Prototype Parent 1개와 폐기된 회전 Normal 1개는 참조가 없음을 확인하고 삭제했다. Prototype binary는 `work/backups_before_river_motion_x02_v59_20260901`에 보존했다.

검증 원본은 `work/river_motion_x02_v59/fixed_axis_phase_capture/analysis_report.json`, `work/river_motion_x02_v59/apply_report.json`, `work/river_motion_x02_v59/final_reload_capture/audit_report.json`, `work/river_motion_x02_v59/final_audit_unreal.log`이다.

실제 사용자의 realtime Editor viewport에서 속도와 시각적 강도를 최종 확인하는 단계는 남아 있다. Cook/Package/Standalone/오프라인 실행도 이번 국소 Material 수정 범위에서는 수행하지 않았다. 기존 `Megaplant_Library/Shrub_Huckleberry_Oak` newer custom-version 오류는 이번 변경과 무관한 Cook 위험으로 계속 남아 있다.

## 41. Global Local-Tangent River Flow V60

### 원인과 전역 감사

V24는 외부 centerline의 최근접 tangent를 water sampling pixel에 전달했다. 이 방식은 굴곡을 이루는 실제 water mask보다 외부 선분 선택에 의존하므로, 분기·근접 수로·급한 변곡점에서 잘못된 선분을 선택하거나 이전 방향을 계속 유지할 수 있었다. V59의 X02 고정축도 한 방향만 표현하므로 곡선 전체를 해결할 수 없었다.

20개 Production River를 water mask 기반 bounded local PCA(`14/24/36px`)로 전수 분석했다. 4,593개 감사 지점 중 사용자 표시 X02 두 타일을 포함해 `X05_Y00`, `X02_Y02`, `X02_Y01`, `X02_Y00`, `X04_Y04`, `X03_Y00`, `X00_Y02` 일곱 타일을 주요 의심 구간으로 판정했다.

| 검사 | Before | V60 | 결과 |
|---|---:|---:|---|
| Mean local tangent angle error | `13.959°` | 양자화 전 기준 `≈0°` | Pass |
| P90 angle error | `45.707°` | 양자화 전 기준 `≈0°` | Pass |
| P95 neighbor direction change | `6.134°` | `2.284°` | Pass |
| Neighbor jumps `>25°` | `0.385%` | `0.098%` | Pass |
| Valid local tangent coverage | - | `73,013 / 75,239` (`97.041%`) | Pass |
| Water/Sampling masks | 기준 | `20/20 pixel-exact` | Pass |
| Deterministic regeneration | 2회 | SHA-256 `20/20` 동일 | Pass |

### 비파괴 후보와 생산 반영

생산 반영 전 20개 V60 Texture를 transient MID에만 연결해 주요 7개 타일 top-down과 X02 8-frame 시퀀스를 캡처했다. 맵과 Asset은 저장하지 않았고 20개 원본 Material을 전부 복구했다. 이후 생산 Asset을 저장하고 모든 20개 River Actor를 V60 MI로 전환했다.

첫 반영 안전검사는 `X00_Y00` V38, `X03_Y01` V48, `X04_Y04` V38 개선 Mesh가 legacy V23 폴더가 아니라는 이유로 거부됐다. 자동 롤백은 성공했다. 읽기 전용 재감사에서 맵 `20/20` 원복과 V60 Asset `20/20` 유효성을 확인한 뒤, 특정 과거 폴더를 요구하지 않고 반영 전후 Mesh/Transform/Collision/Shadow/Tags의 exact equality를 검사하도록 수정해 재반영했다.

| 저장·재로딩 검사 | 결과 |
|---|---:|
| River Actor | `20/20` |
| V60 MI / Flow Texture reference | `20/20 exact` |
| Shared Parent | V24, `87` expressions |
| Mesh/Transform/Collision/Shadow/Tags 변경 | `0` |
| 새 Parent/Actor/Tick/Runtime PCG | `0/0/0/0` |
| Fresh-process integration audit | Pass |
| Representative captures | `15/15` 생성 |
| Fresh-process Map Check | Error `0`, Warning `0` |

V60의 tangent Texture 자체는 감사 기준을 통과했지만, 이 단계의 렌더 판정은 잘못된 acceptance였다. V24 Parent에 남아 있던 비등방성 `T_River_Waves01_Normals`의 animated Primary/Detail panning이 최종 화면 인상을 계속 지배할 수 있었고, 당시 ROI 분석은 그 가시 경로를 분리해 검증하지 못했다. 사용자의 반복 관찰로 V60은 화면상 횡방향 흐름을 해결하지 못한 것으로 판정했으며, V60은 이후 V61 위상 생성의 방향 원본으로만 사용한다.

## 42. Global Longitudinal Phase River Flow V61

### 수정 원인과 방식

사용자가 표시한 문제는 FlowMap 입력 벡터만의 오류가 아니라 최종 Material에서 실제 움직이는 신호가 여러 경로에 남아 있던 것이 원인이었다. V61은 V24/V60의 animated normal panning을 생산 경로에서 제거하고, V60 tangent를 전역으로 적분한 단일 longitudinal phase만 시간에 따라 이동시킨다. 굴곡에서는 phase gradient가 국소 tangent로 바뀌고, 같은 연결 성분의 타일 경계에서는 위상이 이어진다.

### 오프라인 위상 검증

| 검사 | 결과 |
|---|---:|
| Target tiles / sampling pixels | `20 / 75,239` |
| Connected components | `16` |
| Conjugate-gradient relative residual | `0.001484` |
| Gradient mean / P90 tangent error | `1.866° / 3.616°` |
| Forward gradient fraction | `99.943%` |
| Axis within 30° | `99.572%` |
| Water/Sampling B/A masks | `20/20 pixel-exact` |

### 생산 반영과 fresh-process 회귀 검사

생산 반영 전 `Daejeon_PCG_Work.umap`, V24 Parent, V60 MI/Texture를 `work/backups_before_river_flow_v61_20260902`에 보존했다. 첫 적용 검사는 세 개선 Mesh가 legacy V23 폴더에 없다는 이유로 안전하게 실패했고, 맵을 자동 복구했다. 검사 규칙을 특정 폴더가 아닌 반영 전후 Mesh path/Transform exact equality와 mesh 존재성으로 바로잡은 뒤 다시 적용했다.

| 검사 | 결과 |
|---|---:|
| River Actor / V61 MI / Phase Texture | `20/20/20 exact` |
| Shared Parent / expressions | `1 / 56` |
| Texture parameters | `FlowMapPhase`, `StaticNormalA`, `StaticNormalB` 정확히 3개 |
| Mesh/Transform 변경 | `0` |
| 개선 Mesh 보존 | X00_Y00 V38, X03_Y01 V48, X04_Y04 V38 exact |
| Collision / Shadow | `20/20 Off / Off` |
| Texture samples | V24 `5` → V61 `3` |
| Animated normal samples | `0` |
| Runtime Actor/Tick/PCG | `0/0/0` 추가 |
| Fresh-process actor audit | Pass |
| Fresh-process Map Check | Error `0`, Warning `0` |
| Material compile error | 확인되지 않음 |

별도 UE 프로세스에서 저장된 X02 생산 MI 세 개를 다시 읽고 Phase Offset을 `0.000..0.875`로 8단계 이동해 생산 강도와 고강도 진단을 각각 캡처했다. 진단값은 저장하지 않았고 종료 시 저장된 생산값 `Speed=-0.16`, `Strength=0.18`, `NormalStrength=0.30`, `PhaseOffset=0.0`으로 메모리 복구했다.

중앙 수로 ROI에서 각 pixel의 시간 평균을 빼 정적 강둑·식생을 제거하고, screen Y row profile energy와 screen X column profile energy를 비교했다. 해당 감사 카메라에서 X02 수로 장축은 screen Y다.

| 캡처 | 장축/횡축 시간 신호 비 | Temporal RMS | 결과 |
|---|---:|---:|---|
| Production (`Strength=0.18`) | `3.615×` | `0.02899` | Pass |
| Diagnostic (`Strength=0.65`, Normal `0`) | `3.271×` | `0.20197` | Pass |

이는 저장된 실제 생산 재질에서 움직이는 신호가 사용자가 파란색으로 표시한 횡방향보다 강 장축을 지배적으로 따른다는 렌더 근거다. 전 20개 구역의 공간 방향 근거는 위상 gradient 전수 감사이며, X02 캡처는 실제 Material 실행 경로를 검증한다.

검증 원본은 `work/river_flow_phase_v61/generation_report.json`, `work/river_flow_phase_v61/apply_report.json`, `work/river_flow_phase_v61/final_production_capture/capture_audit_report.json`, `work/river_flow_phase_v61/final_production_capture/motion_analysis.json`이다.

Cook/Package/Standalone/오프라인 실행과 사용자의 realtime Editor 최종 육안 확인은 아직 수행하지 않았다. 기존 `Megaplant_Library/Shrub_Huckleberry_Oak` newer custom-version 오류는 V61과 무관한 Cook 위험으로 남아 있다.

## 43. Targeted River Surface/Contact/Speed V62

### 원인 확인

- `X03_Y00`: source mask 내부 단일 zero pixel `(880,86)`이 실제 mesh의 작은 octagonal hole로 남아 있었다.
- `X02_Y00`: V23 baked `100cm`와 Actor Z `75cm`가 합쳐져 유효 clearance가 `175cm`였다.
- `X02_Y01–X03_Y01`: 공유 XY 표본 `43`개가 존재했지만 각각 `175cm`와 `28.8358cm` clearance 정책을 사용해 약 `146.1642cm`의 수면 단차가 있었다.
- `X02_Y01` LowerLayer: `58,533` instances로 20개 구역 median `77,602.5`, maximum `168,903`보다 낮았다. instance count 과밀이 렉의 직접 원인이라는 근거가 없어 감축하지 않았다.

### 적용 결과

| 검사 | 결과 |
|---|---:|
| X03_Y00 mask delta | active cell `+1`, triangle `+8` |
| X02_Y00 final sampled gap min / median | `13.734cm / 28.836cm` |
| X02_Y01 final sampled gap min / median | `16.905cm / 28.836cm` |
| X03_Y01 final sampled gap min / median | `8.000cm / 28.836cm` |
| Y01 shared refined points / max Z delta | `178 / 0.0cm` |
| FlowV61 LongitudinalSpeed | `20/20`, `-0.16 → -0.14` |
| X02_Y01 grass | `58,533`, exact retained |
| Other PCG instance components | 전부 exact retained |
| Other River Actors | `16/16` mesh/transform/material exact retained |
| Target Collision / Shadow | `4/4 Off / Off` |
| Saved-map fresh-process audit | Pass |
| Fresh-process Map Check | Error `0`, Warning `0` |
| X03_Y00 corrected-location top-down render | Pass, 내부 hole 없음 |

적용 전 `Daejeon_PCG_Work.umap`, 대상 기존 mesh 네 개, `FlowV61` MI 20개를 `work/backups_before_river_targeted_v62`에 SHA-256 manifest와 함께 보존했다. 첫 실행은 `X03_Y00` Landscape 최외곽의 smoothing 좌표가 경계 밖으로 약 `3.4cm` 나가 trace에 실패해 저장 전에 중단됐다. accepted V23과 같은 unsmoothed-coordinate height fallback을 추가한 뒤 재실행했으며, fallback은 `X03_Y00` vertex 6개에만 사용됐다.

검증 원본은 `work/river_targeted_v62_plan.json`, `work/river_targeted_v62_apply_report.json`, `work/river_targeted_v62_fresh_validation.json`, `work/river_targeted_v62_capture/capture_report.json`이다. 이번 단계에서는 realtime viewport frame profiling, Cook/Package/Standalone/offline 실행을 수행하지 않았다. 기존 Huckleberry Oak newer custom-version 오류는 이번 V62 변경과 무관한 별도 Cook 위험이다.

## 44. X02_Y00–X03_Y00 Seam Root-Cause and V63/V64 Validation

### V62 회귀 원인

V62 보고서는 Y01 경계만 acceptance 대상으로 삼았고, 같은 수정에서 `X02_Y00`을 공통 `28.8358cm` clearance로 내리면서 `X03_Y00`은 기존 `175cm`에 남겼다. 두 타일은 base shared world XY `17`개와 UV를 정확히 공유했지만 world Z가 `146.1642cm` 달랐다. Map Check는 Actor/Asset 유효성 검사이므로 이 시각적 접합 오류를 검출하지 못했다.

| 읽기 전용 원인 감사 | 결과 |
|---|---:|
| Shared base XY / max UV delta | `17 / 0.0` |
| X02 / X03 effective clearance | `28.8358 / 175.0cm` |
| Predicted shared-edge height delta | `146.1642cm` |
| X03 direct neighbors | `X02_Y00` 한 개 |

V63은 `X03_Y00`만 V62와 같은 두 단계 subdivision/Landscape projection으로 재생성했다. 다른 River 19개와 PCG는 건드리지 않았다.

| V63 geometry 검사 | 결과 |
|---|---:|
| X03 refined vertices / triangles | `76,641 / 148,224` |
| Shared refined points | `65` |
| Maximum surface height delta | `0.0cm` |
| Sampled X03 final gap min / median / max | `15.3592 / 28.8358 / 40.7627cm` |
| Other River / PCG changes | `0 / 0` |

### 남은 material 경계 원인과 최종 쌍방 보정

V63 geometry 캡처에서는 지형이 비치는 틈은 없어졌지만 수평 명암선이 남았다. 원본 V61 phase texture의 X02 마지막 열과 X03 첫 열은 active water row에서 평균 `43.008°`, 최대 `49.248°` 위상 차이가 있었다. 두 Texture가 `Clamp + Bilinear`이므로 각 타일은 경계에서 서로 다른 끝 texel을 읽었다.

X03 한쪽에만 1px gutter를 둔 중간 시험은 X03 경계값만 `X02-last + X03-first` 평균으로 바꾸고 X02는 `X02-last`를 그대로 읽어 선이 남았다. 최종 V64는 X02에도 reciprocal right gutter를 추가해 두 타일 모두 같은 두 texel 평균을 읽게 했다. Parent Material, shader expression, Texture sample, 유속은 변경하지 않았다.

| Fresh-process V64 검사 | 결과 |
|---|---:|
| Engine | `5.7.4-51494982` |
| Level / River / PCG Actor | `178 / 20 / 148` |
| X02/X03 Texture size | `258×258 / 258×258` |
| Shared edge texel coordinate | `256.4999995 / 0.5000159` |
| Offline bilinear edge RGBA equality | exact pass |
| Non-UV scalar/vector parameters | exact retained |
| V61 parent texture parameters | exactly `3` retained |
| Flow speed | `20/20`, `-0.14` |
| Other 19 River state | exact retained |
| PCG instance components | exact retained |
| Material sample / draw-call delta | `0 / 0` |
| Fresh-process Map Check | Error `0`, Warning `0` |

동일 top-down 카메라의 center-water crop에서 이전 경계 행(448–449)의 평균 absolute RGB delta는 `2.01875 → 0.34458`, `82.93%` 감소했다. 최종 top-down/oblique 캡처에서는 이전 직선 명암선과 노출된 지형 틈이 확인되지 않았다.

검증 원본은 `work/river_y00_seam_v63_unreal_audit.json`, `work/river_y00_seam_v63_apply_report.json`, `work/river_phase_pair_v64_fresh_validation.json`, `work/river_y00_visual_seam_v64_measurement.json`, `work/river_y00_seam_v63_capture/capture_report.json`이다. 적용 전 맵은 `work/backups_before_river_phase_pair_v64/Daejeon_PCG_Work.umap`에 SHA-256 `28778473C9993707DA51DB964782F2F32A299424A92457210AB58979F754B3BD`로 보존했다.

이번 범위에서는 사용자의 interactive viewport 확인, realtime frame profiling, Cook/Package/Standalone/offline 실행을 수행하지 않았다. 기존 Huckleberry Oak newer custom-version 오류는 이번 변경과 무관한 별도 Cook 위험으로 남아 있다.

## 45. Global River Grounding Root Cause / V65 Validation

### 45.1 요청 증상

`X00_Y02`에서 River surface가 Landscape보다 눈에 띄게 떠 있고, 강변에 어두운 수직 간격이 보였다. 특정 tile의 transform만 수정하기 전에 전체 River production을 재검사했다.

### 45.2 확인된 원인

Fresh Unreal 5.7.4 audit에서 기존 V23 mesh의 약 `100 cm` baked clearance와 River Actor의 `Z = +75 cm`가 함께 적용되어, 20개 tile 모두 처음에는 명목상 약 `175 cm` 위에 배치된 사실을 확인했다.

V62/V63에서 네 tile만 약 `28.8358 cm` 접지 mesh와 `Actor Z = 0`으로 교체되었고, 나머지 16개는 기존 `175 cm` 기준에 남았다. 따라서 `X00_Y02`는 새로운 개별 transform 오류가 아니라, 전체 수정이 아닌 부분 교체로 인해 남은 legacy 접지 tile이었다.

수정 전 동일 위험군으로 확인된 16개 tile은 다음과 같다.

- `X00_Y00`
- `X00_Y01`
- `X00_Y02`
- `X01_Y02`
- `X02_Y02`
- `X03_Y03`
- `X03_Y04`
- `X03_Y05`
- `X04_Y01`
- `X04_Y02`
- `X04_Y03`
- `X04_Y04`
- `X05_Y00`
- `X05_Y02`
- `X07_Y04`
- `X07_Y05`

기존 fine-contact tile은 `X02_Y00`, `X02_Y01`, `X03_Y00`, `X03_Y01`이었다.

### 45.3 설계 비교

전체 20개 tile에 2회 subdivision을 적용하는 보수적 계획은 무관통 공통 clearance `53.3054904937744 cm`를 만들 수 있었지만 약 `3,862,528` triangle이 필요했다. 접지 문제를 해결하기 위해 geometry를 약 3.3배 늘리는 것은 성능 목표에 맞지 않아 폐기했다.

채택한 V65는 1회 subdivision, 공통 clearance `28.835796991983898 cm`, 그리고 triangle center gap이 `8 cm` 미만인 제한된 위치의 국소 vertex lift를 결합한다. tile boundary의 동일한 world XY vertex에 같은 lift를 적용해 seam 높이도 동기화한다.

### 45.4 적용 결과

- River Actor: `20 -> 20`
- 전체 Level Actor: `178 -> 178`
- PCG Actor: `148 -> 148`
- legacy 접지 tile 교체: `16`
- 기존 fine-contact tile 재생성: `4`
- 모든 River Actor Z: `0 cm`
- 모든 material assignment: 유지
- 모든 `LongitudinalSpeed`: `-0.14` 유지
- collision: 전체 비활성화 유지
- shadow: 전체 비활성화 유지
- LOD0 triangle: `1,163,608 -> 965,632`
- triangle 감소: `197,976` (`17.01%`)
- 최소 triangle-center gap: `8.0 cm`
- shared boundary 최대 높이 차이: `0 cm`

### 45.5 적용 안전성

첫 asset import 직후 Unreal Python이 즉시 triangle count를 읽는 과정에서 importer 갱신 타이밍 때문에 수량 검사가 실패했다. 이 첫 실행은 Level 저장 전에 중단되었고, map SHA-256이 backup과 동일함을 확인했다.

Fresh process에서 20개 V65 asset의 계획 triangle 합계와 실제 import triangle 합계가 모두 `965,632`로 일치해 face retention `100%`를 확인한 뒤 map reference를 교체했다.

수정 전 map backup:

`work/backups_before_river_global_contact_v65/Daejeon_PCG_Work.umap`

### 45.6 Fresh validation

별도 UnrealEditor-Cmd process에서 map을 다시 로드해 다음을 검증했다.

- Engine: `5.7.4`
- Level load: 성공
- Actor count: `178`
- River Actor count: `20`
- PCG Actor count: `148`
- 모든 River mesh path가 V65 folder를 참조함
- Actor XY/rotation/scale 유지, Z `0`
- material/flow parameter 유지
- PCG instance component 수가 V62 accepted baseline과 일치
- imported triangle count가 계획값과 tile별로 일치
- Map Check: `0 errors`, `0 warnings`

`X00_Y02` ground/drone-height oblique render에서는 수면 가장자리와 지형 사이의 검은 수직 간격이 제거되었고, aerial oblique render에서도 해당 bank outline이 연결된 상태를 확인했다.

검증 자료:

- `work/river_global_grounding_v65_audit.json`
- `work/river_global_contact_v65_plan.json`
- `work/river_global_local_contact_v65_plan.json`
- `work/river_global_local_contact_v65_apply_report.json`
- `work/river_v65_import_counts_audit.json`
- `work/river_global_local_contact_v65_fresh_validation.json`
- `work/river_global_contact_v65_capture/X00_Y02_ground_oblique.png`
- `work/river_global_contact_v65_capture/X00_Y02_aerial_oblique.png`

### 45.7 미검증 및 기존 문제

- 이번 단계에서는 interactive viewport frame-time/GPU profile을 다시 측정하지 않았다.
- Cook, Package, Standalone 실행, network-offline packaged 실행은 아직 검증하지 않았다.
- log에 기존 Huckleberry Oak asset의 newer custom-version 오류와 일부 Chromium/UI warning이 남아 있다. V65 River 수정에서 이 asset이나 UI 설정은 변경하지 않았다.
- 두 대표 render는 `X00_Y02` 접지 수정의 시각 검증이다. 20개 전 구역의 미관을 동일 카메라 조건으로 캡처한 것은 아니며, 전 구역 접지는 geometry/Landscape 수치 검사와 seam 검사로 검증했다.

## 46. River Topology V66 / Shared Phase Atlas V67

### 46.1 원인과 범위

사용자가 표시한 증상을 한 원인으로 묶지 않고 다음과 같이 분리했다.

- Y00의 가로선: X02/X03가 서로 다른 Clamp phase texture 끝 texel을 읽어 생긴 material seam
- 작은 점/홀: source water mask의 1–8 cell enclosed hole
- 사슬처럼 꼬이거나 점으로 만나는 강: 8-neighbor component 판정이 diagonal-only contact를 유효 연결로 허용한 topology 오류
- `X04_Y04` 비정상 지류: 위성 근거가 약한 OSM `968820568/569/570`와 V38 bridge cells가 결합된 결과
- `X00_Y02` 불필요 지류: OSM `530929919`의 북쪽 분리 component

V66은 위 topology만 정리하고 Forest/Riparian/flow/material 구조를 유지했다. V67은 Y00 두 타일의 phase를 하나의 atlas에 배치해 같은 경계 sample을 사용하도록 했다.

### 46.2 결과

| 검사 | 결과 |
|---|---:|
| Source water cells | `30,176 → 30,052` |
| Removed / added cells | `198 / 74` |
| Small holes filled | `8 holes, 19 cells` |
| Remaining hole | Geum-river island `47 cells` 한 곳 |
| 4-neighbor / 8-neighbor components | `19 / 19` |
| Diagonal-only contacts | `0` |
| River LOD0 triangles | `965,632 → 961,664` |
| Triangle-center minimum gap | `8.0cm` |
| Cross-tile shared height delta | `0.0cm` |
| V67 shared atlas | `514×258`, common U `0.5000000118` |
| Same-camera seam-row RGB delta | V64 `0.34458` → V67 `0.33625` |
| Texture sample / draw-call delta | `0 / 0` |
| Flow speed | `20/20`, `-0.14 cycle/s` |
| River collision / shadow | `20/20 Off / Off` |
| Actor / River / PCG count | `178 / 20 / 148` |
| PCG production instances | Forest `475,313`; Riparian Main `35,523`; LowerLayer `1,487,653` |
| Manual exclusion references | reachable production graphs `7` |
| Fresh-load Map Check | Error `0`, Warning `0` |

비영구 렌더 캡처에서 Y00 선, `X04_Y03` 내부 hole, `X00_Y02` 북쪽 불필요 지류, `X04_Y04` 약한 사슬 지류가 남지 않은 것을 확인했다. 적용 전 맵은 `work/backups_before_river_topology_v66/Daejeon_PCG_Work.umap`과 `work/backups_before_river_phase_atlas_v67/Daejeon_PCG_Work.umap`에 각각 보존했다.

### 46.3 `PCG_EXCL_Manual_Central_01`

현재 Actor는 tag `PCG_Exclude_Vegetation`을 가진 단일 `PCGVolume`이다. Fresh-load에서 생산 Actor가 사용하는 reachable graph 20개 중 7개가 이 tag를 직접 조회했다. 삭제 시 다음 PCG 재생성에서 중앙 400m × 400m exclusion이 사라질 수 있으므로 유지한다. 렌더 mesh/instance를 만들지 않아 viewport 렌더 부하의 원인은 아니다.

### 46.4 Viewport 지연에 대한 현재 판단

이번 검증만으로 memory leak은 확인되지 않았다. 현재 Level은 World Partition을 사용하지 않고 생산 식생 instance가 합계 `1,998,489`개이며, 그중 LowerLayer가 `1,487,653`개다. 이동 중 새로운 화면 영역의 masked grass overdraw, texture streaming, Editor cache가 동시에 증가하는 것이 우선 의심되지만, 시간 누적형 leak인지 특정 공간 hotspot인지는 고정 비행 경로의 `stat unit`, `stat gpu`, `stat streaming` 또는 Unreal Insights trace가 필요하다. River V66/V67은 Actor/Draw call/texture sample을 늘리지 않았고 geometry도 `3,968` triangles 감소했으므로 이번 river 변경이 누적 렉을 증가시킨 근거는 없다.

현재 project log에는 이번 수정과 무관한 Huckleberry Oak asset의 newer custom-version 오류가 남아 있다. Cook/Package/Standalone/network-offline 검증과 interactive fixed-route frame profile은 이번 단계에서 수행하지 않았다.

검증 원본:

- `work/river_topology_v66_audit.json`
- `work/river_topology_v66_plan.json`
- `work/river_topology_grounded_v66_plan.json`
- `work/river_topology_v66_apply_report.json`
- `work/river_phase_atlas_v67_apply_report.json`
- `work/river_y00_visual_seam_v67_measurement.json`
- `work/river_final_v67_validation.json`
- `work/river_final_v67_capture/capture_report.json`

## 47. River Connectivity V68 Root-Cause, Repair, and Validation

### 47.1 원인 감사

V66 최종 mask는 4-neighbor와 8-neighbor 모두 `19` component였고 diagonal-only contact는 `0`이었다. 즉 현재 증상은 점 접촉 회귀가 아니라, 서로 다른 polygon/component 사이가 비어 있는 별도 문제였다.

원본 water-area GeoJSON, centerline GeoJSON, 위성 reference와 V66 topology를 같은 registration으로 비교했다. 하나의 수계가 여러 OSM polygon으로 나뉘어 있거나 수로 폭이 `30m` raster 한 셀보다 좁아지는 곳에서 source raster가 끊겼고, V66은 별도 component 연결을 범위 밖으로 두었기 때문에 그 간격이 production mesh에 그대로 남은 것이 확인됐다.

거리만 기준으로 모든 component를 연결하는 방식은 폐기했다. 그 방식은 사용자가 제거를 요청했던 `X00_Y02` 지류를 복구하거나, 비활성 tile과 저수지를 횡단하는 가짜 수면을 만들 수 있다.

### 47.2 수정 설계와 적용

동일 centerline network의 minimum-spanning edge, 동일 area feature, source-area corridor 또는 매우 짧고 명확한 collinear chain으로 확인된 `15`개 gap만 연결했다.

- Jeongan 계통: `5` edges
- 중앙 Yudeung/Yuseong/Gapcheon 계통: `5` edges
- `X05_Y00` collinear chain: `3` edges
- Geum area/centerline 계통: `2` edges

최종 mask는 `30,052 → 32,210` cells, 4-neighbor/8-neighbor component `19/19 → 4/4`가 되었다. `2,158` cells를 추가했고 기존 V66 water cell은 제거하지 않았다. 독립된 세 수계는 근거 부족 또는 이전 사용자 결정 때문에 의도적으로 합치지 않았다.

20개 River mesh를 `/Game/Environment/River/Production/Meshes/V68Connectivity`로 재생성하고 기존 Actor reference만 교체했다. Forest/Riparian PCG, actor 수, material/flow 구조, transform XY/rotation/scale은 변경하지 않았다. Landscape 접지는 V66과 동일한 공통 clearance와 국소 lift를 사용했다.

첫 import 직후 triangle count를 즉시 읽은 실행은 Unreal StaticMesh async build가 끝나기 전이어서 저장 전에 중단됐다. Fresh process에서 20개 asset의 planned/imported triangle 합계가 모두 `1,030,720`, mismatch `0`임을 확인한 뒤 같은 asset을 재사용해 map을 저장했다.

적용 전 map backup:

`work/backups_before_river_connectivity_v68/Daejeon_PCG_Work.umap`

### 47.3 Fresh-process 검증

| 검사 | 결과 |
|---|---:|
| Engine | `5.7.4-51494982` |
| Level / River / PCG Actor | `178 / 20 / 148` |
| River mesh path | `20/20` V68 folder |
| Imported / planned LOD0 triangles | `1,030,720 / 1,030,720` |
| V66 대비 triangle delta | `+69,056` (`+7.180886%`) |
| 4-neighbor / 8-neighbor components | `4 / 4` |
| Diagonal-only contact | `0` |
| Triangle-center minimum gap | `8cm 이상`, 전 tile |
| Shared boundary maximum height delta | `0cm` |
| Actor Z | `0cm`, `20/20` |
| LongitudinalSpeed | `-0.14`, `20/20` |
| Collision / Shadow | `20/20 Off / Off` |
| V67 shared phase atlas | 유지, shared UV exact |
| Manual exclusion | Actor/tag/7 production references 유지 |
| Fresh Map Check | Error `0`, Warning `0` |

비영구 카메라로 다음 네 계통을 별도로 캡처했고, 승인된 gap에서 수면이 이어진 것을 확인했다.

- `work/river_connectivity_v68_capture/01_X05_Y00_collinear_chain.png`
- `work/river_connectivity_v68_capture/02_X00_Y00_Y02_centerline_chain.png`
- `work/river_connectivity_v68_capture/03_X03_X04_central_network.png`
- `work/river_connectivity_v68_capture/04_X07_Y04_Y05_geum_chain.png`

검증 원본:

- `work/river_connectivity_v68_audit.json`
- `work/river_connectivity_v68_plan.json`
- `work/river_connectivity_grounded_v68_plan.json`
- `work/river_v68_import_counts_audit.json`
- `work/river_connectivity_v68_apply_report.json`
- `work/river_final_v68_validation.json`
- `work/river_connectivity_v68_capture/capture_report.json`

### 47.4 미검증 및 기존 문제

- 이번 단계에서는 사용자의 interactive viewport에서 지상/드론 근접 비행을 직접 조작해 보지 않았다.
- Cook, Package, Standalone 실행과 network-offline packaged 실행은 수행하지 않았다.
- 기존 Huckleberry Oak asset의 newer custom-version 오류와 일부 ToolMenus/UI warning은 남아 있다. V68은 해당 asset과 설정을 변경하지 않았다.

## 48. River Cleanup / Dense Contact V69

### 48.1 원인 감사

사용자 표시 위치를 V68 mask와 actor tile에 대응했다.

- `X00_Y02`: V68 component `2`, `67` cells, bbox `[130,660,152,694]`, centroid `[137.84,678.33]`의 독립 수로와 정확히 일치했다. V68에서 연결 근거가 없어 보존됐던 대상이며, main 수계와 이어진 것처럼 보이게 만들려면 근거 없는 물 cell 추가가 필요했다.
- `X04_Y04`: active cells `1,302`, local 4-neighbor components `3`, enclosed holes `0`. V68 OBJ는 `23,811` vertices, `41,664` triangles, degenerate index triangle `0`, non-manifold edge `0`이었다. 따라서 화면의 선/점은 topology hole이 아니었다.
- V68 `X04_Y04` dense contact: triangle당 `12` barycentric samples, 총 `499,968` samples, unique Landscape traces `321,408`, trace failure `0`. 최소 gap `-13.11891898cm`, below `0cm` `396`, below `2cm` `518`, below `8cm` `1,085`였다. 기존 center-only contact 검사가 triangle 내부 Landscape 상승을 놓친 것이 원인으로 확정됐다.

추가 수로 후보를 위해 현재 water surface 밖 OSM centerline을 같은 registration으로 overlay했다. uncovered feature는 `146`개(`river 136`, `stream 10`)였으나 선 자료만으로는 폭과 bank 범위를 확정할 수 없고, 사용자 이미지의 넓은 공백에 해당하는 단일 feature도 확정할 수 없었다. 새 수로를 production에 바로 추가하면 사슬형 topology, 능선 횡단, ground contact 및 Riparian 불일치가 재발할 가능성이 있어 V69 범위에서 보류했다.

### 48.2 적용 결과

`X00_Y02`의 위 component `67` cells만 삭제했다. 다른 V68 cell은 변경하지 않았고 final cell count는 `32,143`, 4-neighbor/8-neighbor components는 `3/3`, diagonal-only contact `0`, 작은 enclosed hole `0`이다.

`X04_Y04`는 triangle 수를 늘리지 않고 각 dense sample에서 필요한 local vertex lift를 계산했다. 최종 mesh 20개는 `/Game/Environment/River/Production/Meshes/V69Cleanup`에 저장했고, 기존 actor reference만 교체했다. V68 asset은 rollback용으로 유지했다.

첫 V69 import 직후 Unreal의 asynchronous Static Mesh build가 완료되기 전에 triangle count를 읽어 첫 적용은 Level 저장 전에 중단됐다. 이때 production map SHA-256과 backup SHA-256이 모두 `AAED935971519BB45A694B64B7E22653A89F0E7B549090AD117D070710A165F3`로 동일했다. 두 번째 process에서 완성된 V69 asset의 tile별 triangle count가 계획과 일치한 뒤 map을 저장했다.

적용 전 backup:

`work/backups_before_river_cleanup_v69/Daejeon_PCG_Work.umap`

적용 후 production map SHA-256:

`9B9CF49B80ECE8DB14C387685CB50D2D92F5AB11791AE0F8CFB1092FB626B7E4`

### 48.3 Fresh-process 정적·접지 검증

| 검사 | 결과 |
|---|---:|
| Engine | `5.7.4-51494982` |
| Level / River / PCG Actor | `178 / 20 / 148` |
| River mesh path | `20/20` V69 folder |
| Imported / planned LOD0 triangles | `1,028,576 / 1,028,576` |
| V68 대비 triangle delta | `-2,144` (`-0.2080%`) |
| 4-neighbor / 8-neighbor components | `3 / 3` |
| Small hole / diagonal-only contact | `0 / 0` |
| Triangle-center minimum gap | `8cm` |
| X04_Y04 dense samples | `499,968` |
| X04_Y04 dense final minimum gap | `7.99999957cm` |
| Accepted minimum / tolerance | `7.99cm / 0.01cm` |
| Dense samples below accepted minimum | `0` |
| Landscape trace failures | `0` |
| Shared boundary maximum height delta | `0cm` |
| Actor Z | `0cm`, `20/20` |
| LongitudinalSpeed | `-0.14`, `20/20` |
| Collision / Shadow | `20/20 Off / Off` |
| V67 shared phase atlas | material/texture/shared UV 유지 |
| Forest / Riparian instances | `475,313 / 35,523 / 1,487,653`, 변경 없음 |
| Manual exclusion | actor/tag/7 reachable graph references 유지 |
| Fresh Map Check | Error `0`, Warning `0` |

`below exact 8cm`으로 단순 비교하면 OBJ 6-decimal 저장 반올림 때문에 `101`개가 최대 `0.00000043cm` 작다. 이는 accepted tolerance `0.01cm`보다 약 23,000배 작고, `7.99cm` 미만 표본과 `0cm` 미만 표본은 모두 `0`이다.

### 48.4 비영구 시각 검증

별도 UE process에서 임시 camera로 세 위치를 캡처했다. camera는 종료 전에 제거했고 Level을 저장하지 않았다.

- `work/river_cleanup_v69_capture/01_X00_Y02_removed_fragment.png`: 제거 대상 위치에 독립 수면이 남지 않음
- `work/river_cleanup_v69_capture/02_X04_Y04_overview.png`: target tile 주변 overview
- `work/river_cleanup_v69_capture/03_X04_Y04_dense_contact_close.png`: 기존 최저 gap 위치에서 Landscape 선/점 관통 없음

### 48.5 검증 원본

- `work/river_v69_preflight.json`
- `work/river_v69_uncovered_centerlines.png`
- `work/river_v69_x04_y04_dense_contact.json`
- `work/river_cleanup_v69_plan.json`
- `work/river_cleanup_grounded_v69_plan.json`
- `work/river_cleanup_v69_apply_report.json`
- `work/river_final_v69_validation.json`
- `work/river_final_v69_x04_y04_dense_contact.json`
- `work/river_cleanup_v69_capture/capture_report.json`

### 48.6 미검증 및 기존 문제

- 추가 수로와 그 주변 Riparian PCG는 V69에 적용하지 않았다. 정확한 centerline/폭/양안 범위를 먼저 지정해야 한다.
- interactive viewport fixed-route frame profile, Cook, Package, Standalone, network-offline packaged 실행은 이번 단계에서 수행하지 않았다.
- 기존 Huckleberry Oak asset은 UE 5.7.4보다 newer custom version으로 저장되어 Asset Registry error가 계속 발생한다. V69은 해당 asset을 참조하거나 변경하지 않았다.
- 일부 Chromium/ToolMenus/UI warning은 River 수정과 무관하게 남아 있다.

## 49. River Water Exclusion / PCG Mask Sync V70

### 49.1 원인 식별

`X04_Y04`의 수면 위 grass는 개별 Actor 위치나 random seed 문제가 아니었다. River Surface는 V69까지 갔지만 세 공유 PCG mask graph가 V40/V49/PrimaryCalibrated texture를 계속 참조해, 실제 수면과 vegetation hard exclusion이 서로 다른 버전을 사용했다.

수정 전 전역 audit 결과는 다음과 같다.

| Layer | Saved instances | V70 water overlap |
|---|---:|---:|
| Forest | `475,313` | `101` |
| Existing Riparian | `35,523` | `51` |
| Riparian Lower Layer | `1,487,653` | `17,374` |

`X04_Y04` Lower Layer만 `456`개가 V70 수면에 있었다. 따라서 국소 instance 삭제로 숨기지 않고 shared spatial authority를 V70으로 동기화했다.

`X00_Y02`의 중앙 hole은 mesh 결손이 아니라 V66에서 보존한 `47` source-cell island였다. 사용자의 최신 의도에 따라 이 정확한 component만 채웠다. 후보 topology는 `32,143 → 32,190` water cells, component `3/3`, remaining enclosed hole `0`, diagonal-only contact `0`을 preflight에서 통과했다.

### 49.2 적용 내용

- River Surface: 20개 actor가 `/Game/Environment/River/Production/Meshes/V70WaterExclusion` mesh를 사용
- River water mask: `T_ENV_RiverWater_Daejeon_ProductionV70`, `6048×6048`
- River bank mask: `T_ENV_RiverBankBand_Daejeon_ProductionV70`, `6048×6048`
- Global water union: `T_ENV_WaterChannel_Daejeon_Extended_RiverV70`, `2017×2017`
- Shared graphs: 기존 3개 graph의 texture input만 변경; sampler filter/transform 유지
- PCG generation: Lower Layer 20개 전부 + affected Forest 4개 + existing Riparian 2개 = `26`개만 재생성
- Runtime architecture: 변경 없음; Editor pre-generated ISM과 `GenerateOnDemand` 유지

적용 전 surface-only backup:

`work/backups_before_pcg_river_masks_v70/Daejeon_PCG_Work_V70_SurfaceOnly.umap`

최종 production map SHA-256:

`8892251A9B1DCF519DDB46ADC46C74F6EE4BF7A80B0C44A1D9DECD3786AC3B04`

### 49.3 중단 복구와 재발 방지

작업 재개 중 두 automation 문제가 확인됐다.

1. 기존 pre-PCG backup과 현재 post-PCG map의 파일 크기를 같아야 한다고 검사해 재실행을 차단했다. 기존 backup은 덮어쓰지 않고, 새로 복사할 때만 크기를 검사하도록 수정했다.
2. 실패 rollback이 실행 시작 시점 입력이 아니라 고정 V40/V49 경로를 복원했다. 시작 시점 graph texture를 캡처해 그 값으로 복원하도록 수정했다.

또한 `NullRHI`에서 6048² bank texture를 사용하는 Lower Layer를 재생성하면 20개 actor가 모두 0 instance가 되는 것을 실제로 확인했다. 해당 중간 맵은 즉시 `RenderOffscreen` 재생성으로 복구했다. 이후 저장 acceptance에 `20/20 non-empty`와 결정론적 총량 일치를 추가했으며, `NullRHI`는 읽기 전용 audit에만 사용한다.

최종 `RenderOffscreen` 복구 실행은 exit code `0`, report success, map save를 모두 통과했다.

### 49.4 Fresh-process PCG 전역 검증

재생성 프로세스와 별도의 UE 5.7.4 process에서 저장 맵을 새로 열어 144개 PCG instance actor를 전수 검사했다.

| Test | Result | 측정값 |
|---|---:|---:|
| Engine | Pass | `5.7.4-51494982` |
| Level actor count | Pass | `178` |
| PCG instance actor count | Pass | `144` |
| Shared V70 graph inputs | Pass | `3/3` loaded, expected texture/size |
| Forest instances / water overlap | Pass | `475,182 / 0` |
| Existing Riparian instances / water overlap | Pass | `35,467 / 0` |
| Lower Layer instances / exact 10m water overlap | Pass | `1,623,247 / 0` |
| X00_Y02 added hole area overlap | Pass | 세 계층 모두 `0` |
| Actors with effective overlap | Pass | `0` |

Lower Layer를 30m global cell로만 보면 경계 후보 `13`개가 잡히지만, 실제 graph가 사용하는 10m water mask에서는 `0`이다. 해당 13개는 30m cell 내부에서 육상과 수면이 함께 존재해서 생긴 coarse-grid false positive다.

### 49.5 Fresh-process River 검증

또 다른 UE 5.7.4 process에서 River geometry와 설정을 독립 검사했다.

| Test | Result | 측정값 |
|---|---:|---:|
| River actors | Pass | `20/20` |
| V70 mesh references | Pass | `20/20` |
| Imported / planned LOD0 triangles | Pass | `1,030,080 / 1,030,080` |
| Actor Z | Pass | `0cm`, `20/20` |
| Triangle-center minimum gap | Pass | `≥8cm` |
| Requested dense-sample minimum gap | Pass | `≥8cm` |
| Cross-tile shared vertex height | Pass | exact |
| LongitudinalSpeed | Pass | `-0.14`, `20/20` |
| Collision / Shadow | Pass | `20/20 Off / Off` |
| V67 shared phase atlas | Pass | material/texture/shared UV 유지 |
| Manual exclusion | Pass | actor/tag/7 reachable graph references 유지 |
| Fresh Map Check | Pass | Error `0`, Warning `0` |

`PCG_EXCL_Manual_Central_01`은 계속 필요하다. 현재 7개 reachable production graph가 `PCG_Exclude_Vegetation` tag를 직접 조회하며, 이 Actor는 geometry를 렌더하지 않는 `PCGVolume`이다.

### 49.6 비영구 시각 검증

임시 camera로 다음 위치를 캡처한 뒤 camera를 제거하고 Level을 저장하지 않았다.

- `work/river_water_exclusion_v70_capture/01_X00_Y02_filled_hole.png`: 중앙 hole이 연속 수면으로 덮임
- `work/river_water_exclusion_v70_capture/02_X04_Y04_water_exclusion_overview.png`: X04_Y04 전경
- `work/river_water_exclusion_v70_capture/03_X04_Y04_water_exclusion_close.png`: 가까운 시점에서 grass가 수면 밖에 유지됨

### 49.7 주황색 신규 수로 preflight

production mutation 없이 annotation과 로컬 GeoJSON을 교차 분석했다.

- 주 annotation component: `24,066` pixels, bbox `[1324,409,2017,1121]`
- 실질 후보 centerline: OSM `34834591`, `388811980`, `1524242671`
- annotation pixels 중 centerline 8-pixel 이내: `69.82%`
- source area polygon 직접 coverage: `0.34%`
- width tag가 있는 후보: `0`
- 필요한 신규 production tile: `10`

결론은 route 선택 자체는 충분히 제한할 수 있지만, 정확한 수면 폭과 양안은 source-defined 상태가 아니라는 것이다. V70에는 신규 수로를 추가하지 않았다. 다음 단계는 신규 10개 tile만 격리한 prototype에서 width rule, 기존 강 합류, longitudinal phase, dense contact, Riparian exclusion 및 fixed-route frame profile을 함께 검증하는 것이다.

### 49.8 검증 원본

- `work/river_water_exclusion_v70_prepare_report.json`
- `work/river_water_exclusion_grounded_v70_plan.json`
- `work/river_water_exclusion_v70_apply_report.json`
- `work/pcg_river_masks_v70_apply_report.json`
- `work/pcg_river_masks_v70_regeneration_report.json`
- `work/all_pcg_water_overlap_v70_audit_before_fix.json`
- `work/all_pcg_water_overlap_v70_audit.json`
- `work/river_final_v70_validation.json`
- `work/orange_river_route_v70_preflight.json`
- `work/river_water_exclusion_v70_capture/capture_report.json`

### 49.9 미검증 및 남은 위험

- interactive viewport fixed-route CPU/GPU/frame-time/VRAM profile은 아직 실행하지 않았다. Lower Layer가 V69 `1,487,653`에서 V70 `1,623,247`로 `135,594`개 증가했으므로, 시각 밀도를 임의로 낮추기 전에 target PC에서 병목을 측정해야 한다.
- 신규 주황색 수로와 그 주변 Riparian은 아직 production에 적용하지 않았다.
- Cook, Package, Standalone, network-offline packaged 실행은 이번 단계에서 수행하지 않았다.
- 기존 Huckleberry Oak newer-custom-version Asset Registry error와 일부 Chromium/ToolMenus warning은 이번 River 수정 범위 밖이며 그대로다.

## 50. Approved Orange River V71 — 2026-09-04

### 50.1 적용 및 저장 검증

- 사용자 주황색 경로에 신규 River 7개, 기존 합류부 2개를 생성/갱신했다. 기존 18개 River mesh/material/numeric transform은 변하지 않았다.
- 기존 수면 source-cell 삭제는 0이다. 최종 4/8-neighbor connected components는 2/2이며, 기존 본류와 동쪽 강이 새 경로로 연결된다. 무관한 북쪽 component는 유지한다.
- 9개 tile의 triangle-center와 요청된 dense edge samples는 Landscape 최소 8cm clearance를 통과했다. 보존 타일과 공유하는 vertex 214개는 기존 높이에 맞췄다. inverted/degenerate triangles는 0이다.
- PCG 27개 component를 cleanup→generate했다. 성공 실행은 전역 수면 침범 검사 및 저장 포함 104.875초였다. 중간 자동화 실패 결과는 저장하지 않았다.
- 저장 결과: Actor 192, River 27, Forest 475,067, legacy Riparian 35,442, LowerLayer 2,874,422, 총 식생 3,384,931.
- 전역 수면 침범은 Forest/legacy의 2017² input mask 및 LowerLayer의 6048² input mask 기준 모두 0이다. 대상 외 PCG instance count는 동일하다.
- 새 프로세스에서 저장 맵을 다시 열어 모든 instance count, River triangle count 2,647,328, collision/shadow/Nanite/distance-field 정책을 확인했다.
- 임시 camera 5개 시점으로 전경, 북동쪽 지류, 굴곡, 동쪽 구역, 강변 근거리를 캡처해 검토했다. 임시 camera와 숨김 상태는 저장하지 않았다. 전체 animated flow/모든 sub-pixel 접지를 정적 캡처만으로 증명한 것은 아니다.

### 50.2 최적화와 측정 한계

새 grass의 collision/shadow/ray tracing은 Off, WPO disable distance는 250m, dense/accent end cull은 500m/700m다. 기존 production descriptor를 재사용했으며 임의 전역 밀도 감소는 없다. 새 River는 collision/shadow/distance-field generation을 사용하지 않는다.

실제 bank instance 근처 80m 높이의 고정 editor camera에서 각 구간 90 warm-up + 180 measured ticks를 비교했다.

| 측정 | grass on | grass hidden | grass on repeat |
|---|---:|---:|---:|
| 기본 editor median | 16.928 ms | 16.702 ms | 16.661 ms |
| session VSync/FPS limit 해제 median | 16.662 ms | 16.674 ms | 16.677 ms |
| 해제 측정 p95 | 17.345 ms | 17.467 ms | 17.393 ms |
| 해제 측정 private memory | 13,760.8 MiB | 13,483.3 MiB | 13,539.7 MiB |

session-only 설정은 `r.VSync 0`, `r.VSyncEditor 0`, `t.MaxFPS 0`, `t.IdleWhenNotForeground 0`이며 config에 저장하지 않았다. 여전히 약 60Hz ceiling을 보이는 offscreen editor tick 측정이므로 GPU 여유나 packaged FPS를 확정할 수 없다. 짧은 비교에서 지속 증가를 확인하지 못했지만 메모리 누수가 없다는 증거도 아니다. UI 비행 경로의 실제 CPU/GPU/VRAM profile이 다음 성능 검증이다. 이번 결과만으로 잔디를 줄일 근거는 부족하다.

### 50.3 미사용 생성 자산 정리

Git untracked 여부와 Unreal package referencer를 함께 조사하고, 현재/복구용 V70·V71 또는 외부 자산에서 참조하는 항목을 보호했다. 남은 미참조 생성 자산 245개(102,593,870 bytes)를 프로젝트 밖에 SHA-256 검증 백업 후 Unreal Editor API로 제거했다.

- 과거 River mesh 107개, material/MI 54개, flow texture 49개.
- 미사용 PCG graph 12개, subgraph 14개, mask texture 9개.
- 모든 현재 Actor/PCGVolume은 유지한다. `PCG_EXCL_Manual_Central_01`도 현행 exclusion 용도로 보존한다.
- 원본 vegetation/Fab 라이브러리, SourceData, plugin, project config는 정리 대상이 아니다.
- 정리 후 맵 재열기와 전체 instance count 일치를 확인했다. 이 디스크 정리를 FPS 개선으로 주장하지 않는다.

복구 사본: 작업 폴더 `work/archive_unused_generated_v71/Content/...` 및 `manifest.json`. 정확한 삭제 목록/해시는 `work/orange_v71_cleanup.json`에 있다.

### 50.4 결과 파일 및 미검증

- `work/orange_river_v71_apply_report.json`
- `work/orange_pcg_v71_generation.json`
- `work/orange_v71_validation_capture.json`
- `work/orange_v71_timing_uncapped.json`
- `work/orange_v71_cleanup.json`
- `work/ORANGE_RIVER_V71.md`

V71 map SHA-256: `62FD80FCD829FD22E6BF4EB827E8124E0E1B7F21932C63B1AB6BD7FB73B873AB`.

Cook/Package/Standalone/offline packaged 검증은 수행하지 않았다. 기존 Huckleberry Oak 라이브러리의 newer custom-version 오류는 별도 문제로 남아 있다. 신규 경로의 폭은 satellite 기반 시각 재구성이므로 사용자 시점에서 추가 확인이 필요하다. Git commit은 수행하지 않았다.

## 51. River Contact / Two Inlets V72 — 2026-09-04

### 51.1 원인 재현

`X07_Y05` 수정 전 근거리 화면 2개에서 흰 polygon 조각을 재현했다. 기존 dense4 검사에서 실제 Landscape가 물보다 높은 표본 209개, 최소 gap -31.5715 cm를 확인했다. 새 critical-intersection 검사에서는 같은 타일 최소 -35.222 cm였다. 표본 개수는 화면에 보이는 hole 개수가 아니다.

원인은 기존 center/고정 barycentric sampling이 water triangle edge와 Landscape triangle grid의 교차 지점에서 발생하는 높이 극값을 놓치는 것이다. 실제 Landscape는 collision MIP 0, XY 간격 3000 cm였다. 기존 구조를 유지하고 교점/내부 grid vertex를 직접 trace하는 Editor-baked 접지 보정으로 변경했다. XY 보존 대상 24개 중 23개에서 관통 표본이 있었고 X06_Y01은 최소 1.3936 cm로 여유만 부족했다. 수역 형태를 새로 만든 3개 타일의 plan-before 수치는 새 미보정 제안에 대한 것으로, 기존 V71 결함 수치와 구분한다.

추가 수역 두 곳은 V71의 사용자 주황색 corridor 제한 밖이었다. 이번 두 영역의 satellite water 근거와 기존 수면 연결성을 사용했다. 기존 source-cell 삭제 0, 추가 3,089, 최종 85,818, 4/8-neighbor component 2/2, inverted/degenerate triangle 0이다.

### 51.2 적용 범위

- River 27개는 V72ContactInlets Mesh를 사용한다. XY 변경은 X05_Y02/X05_Y03/X06_Y02뿐이며, 나머지 24개는 최종 source OBJ의 XY 오차 <0.00001 cm 및 UV/face 불변을 확인했다.
- 기존 flow texel과 속도 -0.14는 유지한다. 새 inlet 3개 타일만 V71 MI를 상속하는 V72 MI/phase texture를 사용한다. V67 특수 shared-seam material은 변경하지 않는다.
- water/bank/global-water mask 3개를 V72로 동기화하고 PCG 공간 셀 X05_Y02/X06_Y02/X05_Y03/X06_Y03의 기존 12개 component를 재생성했다.
- 전체 Actor 192, River Actor 27로 불변. 대상 외 PCG instance count도 불변이다. Runtime PCG, Tick, 추가 물리 연산은 도입하지 않았다.

### 51.3 최종 저장본 검사

Unreal native `StaticMeshExporterOBJ`로 저장된 실제 LOD0를 추출했다. UE 5.7.4 exporter의 X/Z/Y 출력 순서를 소스에서 확인하고 world coordinates로 복원했다. 생성 원본 OBJ의 검사만으로 통과 처리하지 않았다.

1차 native 검사에서는 모든 sample이 지면 위였지만 5개 타일의 27 sample entries가 7.99 cm 안전 여유에 미달했다. 최소 7.873469 cm였다. 38개 source vertex를 최대 0.626531 cm만 추가 보정했고 공유 tile edge는 불변으로 제한했다. 원본 OBJ/asset/검사 보고서를 backup한 뒤 적용했으며, 기준을 낮추지 않았다.

| 최종 검사 | 결과 |
|---|---:|
| Native LOD0 River count | 27 |
| Native LOD0 triangles | 2,746,176 |
| Critical sample entries | 17,970,194 |
| Minimum Landscape gap | 7.9986778103 cm |
| Gap <7.99 cm entries | 0 |
| Shared tile-edge groups | 1,660 |
| Maximum shared height difference | 0 cm |
| 전역 water-overlap instances | 0 |
| Fresh reload PCG count | 3,475,293 / 전체 Actor별 일치 |
| Final Map Check | Error 0 / Warning 0 |

최종 numeric 검사는 490.269초였다. 이는 generation time이나 runtime frame time이 아니다. 약 8 cm의 최소 간격을 float import 허용 오차 0.01 cm 기준으로 통과했다. 맵 극외곽의 기존 smoothing 범위 3.414 cm 밖에는 Landscape가 없으므로, 해당 perimeter trace miss만 제한된 inward retry를 사용하고 report에 횟수를 기록했다.

같은 카메라로 수정 전후 5시점을 캡처해 직접 비교했다. X07_Y05 근거리 2시점에서 이전의 흰 조각들이 사라졌고, 전경의 연속 수면과 두 inlet의 추가/연결을 확인했다. 임시 camera는 제거했으며 검사용 상태는 Level에 저장하지 않았다.

### 51.4 성능 관련 수량 및 한계

| 항목 | V71 | V72 |
|---|---:|---:|
| River triangles | 2,647,328 | 2,746,176 |
| Forest | 475,067 | 475,062 |
| Legacy Riparian | 35,442 | 35,442 |
| LowerLayer | 2,874,422 | 2,964,789 |
| Total vegetation instances | 3,384,931 | 3,475,293 |

Triangle +98,848 (+3.73%)는 추가 source-cell ×32에 정확히 해당하며, 접지 보정으로 추가된 triangle은 0이다. 식생은 +90,362 (+2.67%)다. PCG cleanup/generate, 전역 exclusion 검사, 저장을 포함한 실행은 54.328초였다. 영향받는 LowerLayer grass는 NoCollision/noShadow, 500m/700m end-cull을 유지한다. River collision/shadow/Nanite/DF 추가 비용은 비활성화했다. 전역 밀도는 임의 감량하지 않았다.

V72 interactive 비행 경로의 CPU/GPU/FPS/VRAM 측정은 수행하지 않았으므로 성능 무저하/메모리 누수 부재를 보장하지 않는다. 검증은 현재 Landscape collision MIP 0와 수면 LOD0 및 5개의 정적 화면 기준이다. 모든 render LOD, 시점, animated flow 또는 packaged 동작의 증명은 아니다. Cook/Package/Standalone/offline packaged 검증은 이번에도 별도이며 기존 Huckleberry Oak custom-version 오류는 남아 있다.

### 51.5 기록과 복구

- 작업 설명: `work/RIVER_V72.md`.
- 원인/접지 계획: `river_v72_x07_y05_before.json`, `river_v72_contact_plan.json`.
- 형태/phase/mask: `river_v72_summary.json`, `river_v72_phase_report.json`, `orange_masks_v72_apply.json`.
- 저장/재생성: `river_v72_apply_report.json`, `orange_pcg_v72_generation.json`.
- 최종 검증: `river_v72_roundtrip_repair.json`, `river_v72_validation.json`, `river_v72_capture_after.json`.
- 전후 PNG: `work/river_v72_capture_before/`, `work/river_v72_capture_after/`.

V71 backup map SHA-256: `62FD80FCD829FD22E6BF4EB827E8124E0E1B7F21932C63B1AB6BD7FB73B873AB`.
V72 map SHA-256: `8BA544184464A3AABBB162776E8E3B518E374E5B6D85695A6F3914442CFDAD5F`.
map hash에는 별도 Mesh asset의 내용은 포함되지 않는다. V71 map과 세 PCG graph backup은 `work/backups_before_river_v72/`에 있으며 기존 V70/V71 자산은 복구용으로 유지했다. 이번 변경에서 Git commit이나 기존 자산 삭제는 수행하지 않았다.

</details>
