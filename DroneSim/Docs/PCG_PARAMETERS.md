# PCG Environment Parameters

## Current parameters — reversible performance trial (2026-09-05)

Accepted V72 geometry, flow and placement are retained. Canonical names/references and the corrected legacy riparian-tree policy are preserved. Only rendering distances and local Editor scalability are adjusted in this trial; see PERFORMANCE_TRIAL.md for the pre-trial backup and validation. The folded history below contains superseded parameter sets, not additional active versions.

| Parameter | Current baseline |
|---|---|
| Forest / riparian tree volumes | 64 / 64, 8 × 8 layout |
| Lower-layer volumes | 27 |
| PCG tile width | 756,000 cm (7.56 km) |
| Generation | Editor pre-generated, GenerateOnDemand, non-partitioned |
| Forest / riparian base seed | 137 / 241 plus row × 8 + column |
| Water / bank mask | 6048 × 6048; canonical `T_ENV_RiverWater_Daejeon` / `T_ENV_RiverBankBand_Daejeon` |
| Global water rejection | 2017 × 2017; canonical `T_ENV_WaterChannel_Daejeon` |
| River actors / LOD0 triangles | 27 / 2,746,176 |
| Existing longitudinal flow speed | -0.14 |
| River collision / shadow / Nanite / distance field scale | Off / Off / Off / 0 |
| Forest instances | 475,062 |
| Riparian-tree instances | 35,442 |
| Lower-layer instances (not exclusively grass) | 2,964,789 |
| Total vegetation instances | 3,475,293 |
| Forest and legacy riparian trees | Start-cull metadata 3500 m, end-cull 5000 m, WPO disable 300 m; NoCollision, existing shadows retained |
| Lower-layer grass end-cull | Existing 500 / 700 m retained; X02_Y01, X03_Y01, X04_Y04, X07_Y05 formerly 2200 m now 1000 m, with 800 m start-cull metadata; NoCollision/noShadow |
| Grass WPO disable distance | Existing 250 m |
| Young alder end-cull / WPO | Existing end-cull 1000 m; WPO disable 300 m, NoCollision, shadows retained |
| Local Editor scalability | Shadow / GlobalIllumination / Reflection / PostProcess = 2 (High); other groups including Foliage unchanged at 3 (Epic) |
| FPS cap / hardware ray tracing | Unchanged: t.MaxFPS=0; r.Lumen.HardwareRayTracing=1 |
| Contact audit acceptance | ≥7.99 cm on checked saved LOD0 / Landscape collision geometry |
| Last contact minimum / shared-edge height delta | 7.9986778103 cm / 0 cm |

Dense regional graphs and the seam-safe shared flow atlas remain distinct. Adjust only the intended functional variant, synchronize water/bank inputs, regenerate affected components with a rendering-capable Editor process, then validate exclusion and fresh-load counts. Do not use NullRHI for texture-sampling PCG generation.

These counts and culling values are not an FPS guarantee. No density reductions or runtime PCG generation were introduced. The 35,442 legacy riparian trees formerly had no finite end-cull/WPO cutoff; their current limits match the forest policy. The trial tradeoff is no tree rendering beyond the unchanged 5 km limit, no tree WPO movement beyond 300 m, and no special-region grass rendering beyond 1 km; nearby placement and materials are preserved. Start-cull metadata alone does not prove a material-driven fade. Editor scalability is a UE 5.7 shared user preference, not a project Config or packaged-runtime change. Representative drone-view CPU/GPU/VRAM profiling and packaged tests remain necessary; all saved instances remain resident, so a large base-RAM reduction is not promised.

<details>
<summary>Historical parameter sets — superseded versions retained for diagnosis</summary>

## 0. Riparian Lower Layer V40 역사 기준

다음 값은 초기 V40 생산 기준의 회귀 이력이다. 현재 V70 spatial authority와 저장 instance 수는 이 문서의 `River Water Exclusion / PCG Mask Sync V70` 절을 기준으로 한다.

| Parameter | 현재값 | 의미 / Tradeoff |
|---|---:|---|
| Spatial authority | `T_ENV_RiverSurface_Daejeon_ShortGapBridges_v38.png` | 현재 보이는 Production 강과 같은 raster를 사용 |
| Raster resolution | `2017 × 2017`, 약 `30 m/pixel` | 이보다 좁은 제방 세부 형상은 표현하지 않음 |
| Inner bank width | `1 pixel ≈ 30 m` | 수면 바깥 양안의 고밀도 grass/reed 구간 |
| Inner bank density | `255/255` | 양안 연속성을 우선 |
| Outer transition width | 추가 `1 pixel ≈ 30 m` | 강에서 육지로 완만히 전환 |
| Outer transition density | `190/255` | 외곽 밀도를 약화 |
| Water overlap | `0 pixel` | bank mask는 수면 바깥쪽에만 생성 |
| Production Actor | `19` | 실제 강이 있는 기존 Lower Layer 셀만 유지 |
| Prototype Actor | `0` | 검증 후 제거 |
| Dense grass culling | `5,000–50,000 cm` | 근·중거리 중심; collision/shadow off |
| Accent culling | `8,000–70,000 cm` | 키 큰 종의 원거리 가시성 보완; collision/shadow off |
| Young alder culling | `10,000–100,000 cm` | collision off, shadow on |
| Generation | Editor pre-generated, `GenerateOnDemand` | packaged runtime에서 PCG 재생성하지 않음 |
| Saved Production instances | `224,456` | V37 `429,498` 대비 `-205,042` (`-47.74%`) |

V40은 V37의 mesh set, scale/rotation variation, Landscape grounding과 ISM 구성을 그대로 재사용한다. 새 대형 시스템이나 C++ 확장은 추가하지 않았다. 시각 밀도를 더 올릴 때는 전역 density를 올리기보다 inner-bank density/폭을 먼저 조정하고, 성능이 부족하면 dense grass의 end-cull을 먼저 낮춘다.

## 1. 문서 상태

- Project: `DroneSim`
- Level: `/Game/Maps/Daejeon_PCG_Work`
- Engine: `5.7.4`
- Last Updated: `2026-09-04`

이 문서는 현재 저장된 생산 Graph, Mask, Actor, River Surface Longitudinal Phase V61, Forest Canopy 및 Riparian Lower Layer의 실제 값을 기록한다. `cm`는 Unreal unit, `m`는 meter, `km`는 kilometer다.

## 2. Global Tile

| Parameter | Forest | Riparian | 설명 |
|---|---:|---:|---|
| GridDimension | `8 × 8` | `8 × 8` | Landscape 계층별 분할 |
| TileSize | `756,000 cm` | `756,000 cm` | `7.56 km` 정사각형 |
| TileCount | `64` | `64` | 계층별 Actor 수 |
| TileZCenter | `25,000 cm` | `25,000 cm` | PCGVolume 중심 |
| TileHeight | `50,000 cm` | `50,000 cm` | 수직 bounds |
| BaseSeed | `137` | `241` | tile index를 더함 |
| SeedRange | `137..200` | `241..304` | 결정론적 seed |
| GenerationTrigger | `GenerateOnDemand` | `GenerateOnDemand` | 저장 결과의 runtime 자동 재생성 방지 |
| Partitioned | `false` | `false` | World Partition 미사용 |

Tile index는 `row*8 + column`이다. 관련 없는 값 변경으로 전체 분포가 바뀌지 않게 BaseSeed와 tile index 규칙을 유지한다.

## 3. Terrain Sampling

`PCG_ENV_ForestRegionSampling`의 현재 주요 값이다.

| Parameter | Value | 의미 |
|---|---:|---|
| SurfaceSampler Density | 약 `0.0020 points/m²` | V33 Forest Landscape 후보 밀도 |
| PointExtents | `50 cm` | 초기 point bounds |
| Looseness | `0.5` | sampler 분포 완화 |
| NormalToDensity reference | `Z Up (0,0,1)` | 경사 판정 기준 |
| Slope DensityFilter | `0.866..1.0` | 약 30도 이하 허용 |

PCGVolume은 수평 후보 범위만 정한다. 실제 높이는 후속 `World Raycast`가 Landscape에서 결정한다.

## 4. Forest Distribution

| Parameter | Value | 의미 |
|---|---:|---|
| Spatial Noise node | `ForestPatchNoise` | broad-scale 군집 |
| Noise transform scale | `(0.5, 0.5, 1.0)` | 저장 Graph 값 |
| Patch threshold | `0.5..1.0` | forest patch 통과 범위 |
| Tree spacing target | 약 `15 m` | `TreeSpacingBounds ±750 cm`와 Self Pruning 기준 |
| GroundTraceStart Z offset | `+500 cm` | trace 시작 여유 |
| WorldRaycast length | `2,000 cm` | 하향 Landscape trace |
| Species meshes | Aleppo Pine `A/B/C/D` | 형태 variation 4개 |
| TreeVariation uniform scale | `1.22..1.55` | V33에서도 유지한 드론/항공 시점 수관 크기 |
| Species weights A/B/C/D | `5 / 5 / 1 / 1` | 큰 A/B 형태를 우선하되 C/D variation 유지 |

V33은 15 m spacing과 `1.22..1.55` scale을 유지한 채 Forest sampling density만 `0.0016 → 0.0020 points/m²`로 높였다. fresh reload에서 저장 Graph와 `419,965` instances를 확인했다. V25에서 수행했던 전역 nearest-neighbor CSV 검사는 이번 density 조정 뒤 반복하지 않았다. Tile 경계는 독립 seed이므로 경계까지 동일한 15 m를 엄격히 보장하려면 tile 간 전역 pruning이 필요하며, 현재는 생성 비용과 시각적 이득의 tradeoff 때문에 추가하지 않는다.

## 5. Riparian Distribution

| Parameter | Value | 의미 |
|---|---:|---|
| Influence threshold | `0.2..1.0` | Riparian mask 양수 영역 유지 |
| Zone source | `0/85/170/255` | Outside/Outer/Near/Water |
| Water core | `Binary Difference` | terrestrial tree hard reject |
| Species meshes | Black Alder `A/B/C/D` | 하천변 교목 variation 4개 |
| TreeVariation uniform scale | `1.00..1.25` | V25 하천변 교목 크기 보정 |
| Species weights A/B/C/D | `3 / 1 / 2 / 3` | 큰 A/D와 중간 C를 우선하고 B 유지 |
| Tree spacing target | 약 `18 m` | `TreeSpacingBounds ±900 cm`와 Self Pruning 기준 |
| BaseSeed | `241` | tile index를 더함 |

전역 Riparian 결과의 실제 최소 간격은 약 `18.04 m`이며 `9m` 미만 pair와 exact duplicate는 없다.

## 6. Texture Mask

### 공통 Texture 설정

- Resolution: `2017 × 2017`
- sRGB: off
- Compression: Grayscale
- Mips: NoMipmaps
- Never Stream: on
- Virtual Texture Streaming: off
- Filter: Bilinear
- X/Y Tiling: Clamp
- Absolute transform scale: `(3024000,3024000,1)`
- Texture density merge: `Set`

### 활성 Texture

| 역할 | Asset | Threshold |
|---|---|---:|
| Forest suitability | `T_ENV_ForestSuitability_Daejeon` | Graph 내부 기존 값 |
| Water exclusion | `T_ENV_WaterChannel_Daejeon_Extended_PrimaryCalibrated` | 기존 Extended VisualMatch와 승인된 Primary 수역의 maximum; 양수 data를 `Binary Difference`에 전달 |
| Riparian influence | `T_ENV_RiparianZones_Daejeon_Extended_VisualMatch` | `0.2..1.0` |
| Calibrated Primary water source | `T_ENV_WaterChannel_Daejeon_CalibratedPrimary` | 단독 미적용; 병합 Asset을 만드는 승인 구역 source |

병합 Water exclusion은 기존 mask pixel을 제거하지 않고 `19,127`개 pixel만 추가했다. V33 현재 저장 수량은 Forest `419,965`, Riparian tree `35,523`이다. Water hard-exclusion Graph는 보존됐지만 V33 Forest origin 전체와 mask의 수치 overlap 검사는 다시 실행하지 않았으므로, 과거 `0/0` 결과를 V33의 신규 측정값으로 간주하지 않는다.

Water mask는 density를 약하게 만드는 soft filter가 아니다. `Binary Difference`를 사용해 물 data와 겹치는 terrestrial point를 제거한다.

### World/Pixel mapping

```text
pixel_x = ((world_x_cm + 3024000) / 6048000) * 2017 - 0.5
pixel_y = ((world_y_cm + 3024000) / 6048000) * 2017 - 0.5
```

이 공식은 UE 5.7 `UPCGTextureData`의 texel-center sampling과 외부 검증을 일치시키기 위해 사용한다. 임의로 `2016`으로 나누는 과거 방식과 혼용하지 않는다.

Visible-water mesh는 Landscape Material의 실제 sampling 경로도 별도로 사용한다.

```text
u = ((world_x_cm - landscape_origin_x_cm) / 3000) * 0.0004959999932907522
v = ((world_y_cm - landscape_origin_y_cm) / 3000) * 0.0004959999932907522
pixel_x = u * 2017 - 0.5
pixel_y = v * 2017 - 0.5
```

OSM과 위성영상 사이에는 위치별 잔차가 있으므로 검증 없이 한 feature 전체에 고정 pixel shift를 적용하지 않는다. Primary 구역에서는 사용자 수동 정합 결과를 다음 regional calibration으로 확정했다.

```text
world_x_corrected = 1.12 * world_x_source + 45169.8801 cm
world_y_corrected = -1.0 * world_y_source - 3060805.2311 cm
```

이 calibration은 vertex 좌표와 시험 mask에 bake하며 생산 Actor에 음수 scale을 사용하지 않는다. 다른 권역에는 대표 subset 검증 전까지 적용하지 않는다.

## 7. Manual Exclusion

| Parameter | Value |
|---|---|
| Required tag | `PCG_Exclude_Vegetation` |
| Current actor | `PCG_EXCL_Manual_Central_01` |
| Difference use | Forest/Riparian 공용 |

추가 제외 영역은 같은 tag의 Volume으로 만든다. Graph를 복제하거나 각 tile마다 exclusion Volume을 따로 만들 필요는 없다.

## 8. Visible River Prototype

| Parameter | Value | 설명 |
|---|---:|---|
| Actor | `ENV_RiverPrimaryAlignmentOverlayPrototype_07` | 주요 하천 형상·offset 검수 Actor |
| Source | Major Centerline | GeoJSON 전체 `421` features 중 주요 하천 선택 |
| Projection | direct WGS84 base | image-fitted affine 미사용 |
| Region | 약 `21 × 15 km` | 전역이 아닌 진단 구역 |
| Selected features/polylines | `7 / 7` | 원본 연결 상태를 유지해 bbox clip |
| Named rivers | `4` | 갑천/금강/미호강/미호천 |
| Centerline length | 약 `47.33 km` | 선택 구역 합계 |
| Ribbon width | `90 m` | 광역 가독성용 고정 폭, 실제 강 폭 아님 |
| Maximum segment | `60 m` | Landscape drape 분할 간격 |
| Vertices/Triangles | `1,810 / 1,796` | 현재 Static Mesh geometry |
| Surface clearance | `8 m` | 정합 검수 가독성용, 실제 수위 아님 |
| Landscape Z range | 약 `88.04 m` | 실제 수위가 아니라 지형 기복 |
| Collision/Overlap | off | 불필요한 physics 제거 |
| Shadow/DF/Decal/Nanite/RT | off | 시험 표면 비용 최소화 |

이 값은 주요 벡터 하천의 수평 형상과 체계적 좌표 offset을 검수하기 위한 과거 draped regional diagnostic 설정이다. 수평 수면, 실제 강 폭·수위, 흐름, 수심 또는 Landscape carve parameter가 아니다. v5/v6 Actor와 Mesh는 v20 정리에서 제거했으며 이 표는 실패 원인과 좌표 검증 이력만 기록한다.

### Primary Water Area Prototype v9

| Parameter | Value | 설명 |
|---|---:|---|
| Actor | `ENV_RiverWaterAreaRegionPrototype_09` | 교정된 Water Area 형상 검수 Actor |
| Selected features | `8` | VisualMatch `river` Polygon |
| Approximate area | `10.3014 km²` | 선택 Polygon raster 면적 |
| Grid | `30 m` | 약 30m/pixel source와 일치 |
| Active cells | `11,446` | surface cell |
| Vertices/Triangles | `13,572 / 22,892` | Static Mesh geometry |
| Surface clearance | `25 cm` | Landscape z-fighting 방지 |
| Actor scale | `(1,1,1)` | 보정은 vertex에 bake |
| Water mask safety buffer | `1 pixel ≈ 30 m` | shoreline trunk 간섭 방지 |
| Production Graph 적용 | `false` | 육안 승인 전 기존 결과 보존 |

### Representative Reservoir Boundary Prototype v13

| Parameter | Value | 설명 |
|---|---:|---|
| Actors | `ENV_RiverBoundary_DaecheongPrototype_13`, `ENV_RiverBoundary_TapjeongPrototype_13` | 대청호/탑정호 shoreline 정합 진단 |
| Projection | direct WGS84 | Primary v9 regional affine 미적용 |
| OBJ import compensation | local Y pre-flip | Unreal OBJ handedness 변환 상쇄 |
| Ribbon width | `90 m` | 광역 시점 가독성용, 실제 shoreline 폭 아님 |
| Simplification tolerance | `45 m` | 30m/pixel 입력에 맞춘 진단 비용 절감 |
| Maximum segment | `240 m` | Landscape drape 분할 상한 |
| Surface clearance | `8 m` | 육안 정합용; 실제 수위 아님 |
| Daecheong diagnostic points | `2,220` | source `16,828` points에서 단순화 |
| Tapjeong diagnostic points | `119` | source `547` points에서 단순화 |
| Actor transform | identity | scale `(1,1,1)`, rotation `0` |
| Collision/Overlap/Shadow/DF/Decal/Nanite/RT | off | 진단 Actor 비용 최소화 |

v13의 목적은 전역 좌표식을 승인하는 것이다. 육안 검수 전에는 ribbon 폭이나 clearance를 생산 수면/수위 값으로 사용하지 않는다.

### Representative Reservoir Outer Boundary Prototype v14

| Parameter | Value | 설명 |
|---|---:|---|
| Actors | `ENV_RiverOuterBoundary_DaecheongPrototype_14`, `ENV_RiverOuterBoundary_TapjeongPrototype_14` | 대청호/탑정호 outer shoreline 진단 |
| X pixel registration | `scale 1.1225866717114303`, `offset -107.90908937682859` | 저장된 v13 `Scale X=1.12` 보정을 vertex에 bake |
| Y pixel registration | `scale 1.0082081114061412`, `offset -7.532280681195135` | 위성 Texture 등록 보정 |
| Ring policy | outer ring `0` only | 대청호 inner ring 5개, 탑정호 inner ring 1개를 ribbon에서 제외 |
| Ribbon width / clearance | `90 m / 8 m` | 광역 진단 가독성용; 실제 shoreline 폭·수위 아님 |
| Simplification / max segment | `45 m / 240 m` | 30m/pixel source에 맞춘 비용 제한 |
| Daecheong traced geometry | `4,566 vertices / 4,566 triangles` | outer shoreline만 사용 |
| Tapjeong traced geometry | `238 vertices / 238 triangles` | outer shoreline만 사용 |
| Actor scale / rotation | `(1,1,1) / 0` | 수동 Transform 불필요 |
| Daecheong Landscape Z range | `1546.88–28220.50 cm` | 약 `266.74 m`; 실제 수위로 사용 금지 |
| Tapjeong Landscape Z range | `1568.69–7924.26 cm` | 약 `63.56 m`; 실제 수위로 사용 금지 |
| Collision/Overlap/Shadow/DF/Decal/Nanite/RT | off | 진단 Actor 비용 최소화 |

v14 pixel registration과 outer-ring 선택은 수평 좌표 검수용이다. Filled Water Area는 육안 정합 승인과 별도의 수면 고도 정책이 확정되기 전에는 만들지 않는다.

### Global Water/Riparian Mask v15

| Parameter | Value | 설명 |
|---|---:|---|
| VisualMatch line/area | `168 / 285` | 위성 정합 필터를 통과한 전역 입력 |
| Central detail line/area | `264 / 38` | 중앙 대전 상세 수계 보존 |
| Hard-water positive pixels | `226,223` | Extended `207,096` + Primary calibration 추가분 병합 |
| Hard-water source coverage | `5.56%` | `2017 × 2017` active merged mask 기준 |
| Riparian zones | `0 / 85 / 170 / 255` | outside / outer / near / water |
| Near-bank radius | `90 m` | `3 pixel`; Black Alder 근접 영향 |
| Outer transition radius | `300 m` | `10 pixel`; 완만한 외곽 영향 |
| Forest/Riparian hard-water overlap | V25 측정 `0 / 0` | V33 Forest 저장 수량 `419,965`; hard-exclusion Graph 보존, 신규 전역 origin 재검사는 미수행 |
| Runtime GeoJSON processing | `off` | PNG로 사전 계산, 네트워크·runtime GIS 비용 없음 |

Visible water flat-plane 후보 판정은 전체 Z range `≤3 m` 및 `P90-P10 ≤2 m`를 보수적 설계 기준으로 사용했다. `P90-P10 ≤15 m`는 구간별/국소 수면 후보로만 분류한다. 이 임계값은 수문학적 수위 정확도가 아니라 현재 과장된 Landscape에서 명백한 관통을 피하기 위한 제작 기준이다. 조사한 대형 후보 18개 중 flat 후보는 `0`, segmented/local 후보는 `3`, Mask 전용 유지 후보는 `15`였다.

## 9. 성능 조정 순서

성능 문제가 생길 경우 다음 순서로 조정한다.

1. `stat unit`, `stat gpu`, `stat rhi` 등으로 병목을 먼저 확인한다.
2. Forest/Riparian instance 수와 shadow/material 비용을 분리해 본다.
3. GPU 병목이면 shadow distance, foliage material, visibility/culling을 우선 검토한다.
4. CPU/메모리 병목이면 전체 density와 tile별 instance budget을 낮춘다.
5. 충돌이 필요하지 않다면 현재처럼 simple collision을 추가하지 않는다.
6. World Partition 전환은 invasive architecture 변경이므로 마지막 선택으로 둔다.

현재 전역 수량은 검증됐지만 live FPS/GPU/VRAM 근거가 없으므로 아직 최종 최적화 값이라고 부르지 않는다.

## 10. River Surface v20 River-only 고정값

다음 값은 현재 강표면 생성 결과를 재현하기 위한 implementation parameter다. 초보 사용자가 매번 조절하는 artist-facing 값은 아니다.

| Parameter | 현재값 | 의미/Tradeoff |
|---|---:|---|
| SourceFeatureSet | `VisualMatch Water Area 285개 중 water=river 93개` | river 96개 중 근거가 약한 3개를 제외. reservoir 186, lake 2, canal 1은 visible surface에 사용하지 않음 |
| GlobalPixelX | `scale 1.1225866717114303`, `offset -107.90908937682859` | 7개 기준점과 v14 대표 수역에서 검증된 전역 satellite registration |
| GlobalPixelY | `scale 1.0082081114061412`, `offset -7.532280681195135` | 포함된 모든 feature에 동일하게 적용 |
| OBJImportCompensation | local Y pre-flip + winding reversal | Unreal OBJ handedness 변환 상쇄 |
| CellSize | 약 `60 m` | 더 작으면 형상은 정밀하지만 triangle/생성 비용 증가 |
| TileSize | 약 `7.68 km` | 단일 거대 Mesh 대신 실제 데이터가 있는 24개 공간 tile만 생성 |
| SurfaceClearance | `18 cm` | Landscape z-fighting 방지 |
| Collision | Off | 드론 환경 표현용; 물리 수면이 아님 |
| CastShadow | Off | 전역 수면 shadow 비용 방지 |
| Nanite | Off | 현재 19,106 triangle 전체 규모에서는 불필요 |
| RuntimeGeneration | Off | 저장된 Static Mesh 결과 사용 |

v17의 regional v7 world affine과 대청호·탑정호 feature별 Transform은 폐기됐다. v20은 수역별 예외 없이 하나의 전역 pixel affine만 사용한다. Actor별 위치·회전·scale 수동 보정은 생산 절차가 아니다. `river-only` 필터는 보이는 강표면 전용 정책이며, 현재 Water/Riparian PCG constraint mask의 범위를 축소하지 않는다. Z는 아직 각 vertex의 Landscape trace + `18 cm`이므로 실제 수위가 아니며, 다음 단계에서 강 구간별 높이 정책을 검증해야 한다.

## 11. 보조 식생/암석 자산 규격

| Category | Asset | 대략적 Bounds | LOD0 triangles |
|---|---|---:|---:|
| Shrub | `SM_ENV_CC0_Shrub_A` | `60×60×36 cm` | 104 |
| Shrub | `SM_ENV_CC0_Shrub_B` | `40×40×24 cm` | 32 |
| Grass | `SM_ENV_CC0_GrassClump_A` | `38×39×25 cm` | 132 |
| Grass | `SM_ENV_CC0_GrassClump_B` | `41×41×25 cm` | 224 |
| Riparian proxy | `SM_ENV_CC0_RiparianTallPlant_A` | `27×27×28 cm` | 32 |
| Riparian proxy | `SM_ENV_CC0_RiparianTallPlant_B` | `28×28×24 cm` | 44 |
| Rock | `SM_ENV_CC0_Rock_Large_A/B` | 최대 약 `102 cm` | 80/85 |
| Rock | `SM_ENV_CC0_Rock_Small_A/B` | 약 `36 cm` | 16/24 |

전역 배치 전에는 시각 스타일 승인이 필요하다. 승인 후에도 권장 초기 예산은 Shrub/Riparian proxy/Rock부터 저밀도로 도입하고, 가장 수량이 많아질 Grass는 별도 근거리 culling 또는 작은 시험 구역 성능 측정 뒤 확장하는 것이다.

## 12. River Surface v23 WaterMaterials 조정값

다음 값은 `/Game/Environment/River/Materials/MI_ENV_RiverSurface_WaterMaterials_Flow`에 저장된 현재 시작값이다. 모든 강 Actor가 하나의 Material Instance를 공유하므로 개별 Actor마다 Material을 복제하지 않는다.

| Category | Parameter | Value | 목적 |
|---|---|---:|---|
| Surface | `Roughness` | `0.45` | 항공 시점의 넓은 거울 반사 억제 |
| Surface | `Specular` | `0.15` | 과한 흰 highlight 억제 |
| Surface | `Opacity / OpacityDeep` | `0.94 / 0.985` | 수면 아래 Landscape 명암이 거대한 파동 띠처럼 비치는 현상 억제 |
| Flow | `Master_Speed` | `-0.04` | 전체 움직임을 느린 하천 수준으로 제한 |
| Flow | `River_Speed` | `0.12` | river normal 흐름 속도 |
| Flow | `WaterNormal_Speed` | `0.08` | 기본 수면 normal 속도 |
| Flow | `SubtleNormal_Speed` | `0.03` | 잔물결 속도 |
| Flow | `EdgeNormal_Speed` | `0.02` | 가장자리 움직임 속도 |
| Contact | `Master_Intensity` | `0.005` | 바다 같은 큰 WPO와 접지 흔들림 억제 |
| Contact | `Intensity1 / 2 / 3` | `0.02 / 0.0 / 0.0` | 다중 파형 중 큰 교차 물결 제거 |
| Normal | `NormalIntensity` | `0.10` | 움직임은 남기되 원거리 shimmer 억제 |
| Normal | `WaterNormal_Intensity` | `0.01` | 큰 normal 기여 제한 |
| Normal | `SubtleNormal_Intensity` | `0.05` | 작은 잔물결만 보조 표현으로 사용 |
| Normal | `EdgeNormal_Intensity` | `0.02` | 가장자리 과장 억제 |
| Reflection | `CubeMap_Intensity` | `0.005` | 원거리 과반사 억제 |
| Reflection | `FakeSpec_Intensity` | `0.02` | 전역 저비용 highlight를 미세하게 유지 |
| Reflection | `FakeSpec_Intensity1 / 2 / 3` | `0.0 / 0.0 / 0.0` | 원본 Material의 기본값 `512` 보조 반사층을 꺼 넓은 바다형 띠 제거 |
| Colour | `Colour` | `(0.008, 0.035, 0.040)` | 밝은 cyan을 줄인 저채도 청록 수면 |
| Colour | `ColourDeep` | `(0.002, 0.010, 0.014)` | 깊은 수면 색 |

현재 생산 Actor 수는 `20`이고 제외 타일은 `X02_Y05`, `X04_Y05`, `X05_Y04`, `X05_Y05`다. 색과 움직임은 위 공유 Instance에서만 조정한다. parent Material이나 `Content/WaterMaterials` 원본을 직접 편집하지 않는다. 이 값은 저비용의 world-space 움직임이며, 굴곡별 실제 유향을 표현하는 flow map은 아니다. 정지 이미지 기준 검수에서는 하부 Landscape 투과로 생기던 넓은 밝은 띠가 크게 줄었지만, 실제 애니메이션 속도는 Editor/PIE에서 확인해야 한다.

## 13. X03_Y01 Directional Flow Prototype 조정값

다음 값은 한 타일 검증용 `/Game/Environment/River/Materials/Prototype/MI_ENV_RiverSurface_DirectionalFlow_Prototype_01`에만 저장되어 있다. 생산 전역 기본값이 아니며, 육안 검수가 끝날 때까지 다른 `19`개 타일에 복사하지 않는다.

| Parameter | Value | 목적 |
|---|---:|---|
| `MapUVScale` | `0.0000013027292080026456` | world cm를 X03_Y01 Flow Map UV로 변환 |
| `MapUVBias` | `(0.939453125, 2.939453125, 0)` | 현재 위성/Level 좌표 등록에 맞춘 타일 원점 |
| `PrimaryTiling` | `0.000035` | 원거리에서 큰 바다형 반복을 피하는 저주파 normal |
| `PrimarySpeed` | `-0.018` | Flow Map 방향을 따르는 느린 1차 흐름 |
| `DetailTiling` | `0.00012` | 작은 잔물결 normal |
| `DetailSpeed` | `-0.040` | 2차 움직임 속도 |
| `DetailBlend` | `0.70` | 두 normal layer 혼합 |
| `NormalStrength` | `0.40` | 원거리 shimmer와 과한 파고 억제 |
| `MovingHighlightStrength` | `0.12` | Opaque 표면에서도 움직임이 읽히는 미세 색 변화 |
| `EdgeStrength` | `0.14` | 가장자리 Fresnel 강도 |
| `Roughness` | `0.32` | 과도한 거울 반사 억제 |
| `Specular` | `0.42` | 잔물결 highlight 유지 |
| `DeepColor` | `(0.003, 0.020, 0.026)` | 깊은 저채도 청록 |
| `FlowColor` | `(0.012, 0.070, 0.082)` | 주 흐름 색 |
| `EdgeColor` | `(0.016, 0.105, 0.115)` | 가장자리 보조색 |

Flow가 육안상 역방향일 때는 `PrimarySpeed`와 `DetailSpeed`의 부호만 함께 반전한다. 크기·위치 정합 문제를 speed, tiling 또는 Actor Transform으로 보정하지 않는다.

## 14. River Surface V24 Global Directional Flow 조정값

활성 타일 `20`개는 `/Game/Environment/River/Materials/M_ENV_RiverSurface_DirectionalFlow_V24` parent를 공유한다. V26 속도 조정 이후 각 Material Instance는 타일별 `FlowMap`/`MapUVBias`와 아래 두 속도 override를 가진다. 나머지 값은 parent 기본값이다.

| Parameter | Value | 목적 |
|---|---:|---|
| `MapUVScale` | `0.0000013027292080026456` | world cm를 8×8 전역 Flow Map 타일 UV로 변환 |
| `PrimaryTiling` | `0.000035` | 넓은 1차 normal의 world-space 반복 크기 |
| `PrimarySpeed` | `-0.022` | V26에서 약 22% 높인 1차 흐름 속도/방향 |
| `PrimaryTravel` | `0.20` | 한 cycle의 최대 UV 이동량 제한 |
| `DetailTiling` | `0.00012` | 작은 잔물결 반복 크기 |
| `DetailSpeed` | `-0.048` | V26에서 20% 높인 2차 흐름 속도/방향 |
| `DetailTravel` | `0.12` | detail cycle 최대 UV 이동량 제한 |
| `DetailBlend` | `0.70` | 1차/2차 normal 혼합 |
| `NormalStrength` | `0.40` | 표면 normal 강도 |
| `MovingHighlightStrength` | `0.035` | detail normal의 색 변화 기여; 넓은 띠 억제 |
| `EdgeStrength` | `0.14` | Fresnel 가장자리 보조색 |
| `Roughness` | `0.32` | 과도한 거울 반사 억제 |
| `Specular` | `0.42` | 잔물결 highlight 유지 |
| `DeepColor` | `(0.003, 0.020, 0.026)` | 깊은 저채도 청록 |
| `FlowColor` | `(0.012, 0.070, 0.082)` | 주 흐름 색 |
| `EdgeColor` | `(0.016, 0.105, 0.115)` | 가장자리 색 |

Flow Texture는 `256×256`, sRGB Off, VectorDisplacementmap, NoMipmaps, Never Stream, Bilinear, Clamp X/Y다. 전체 `20`개 Texture의 water pixel은 `30,279`, dilated sampling pixel은 `75,239`이며 생성 직후 방향 벡터 최소 길이는 `0.99999988` 이상이었다. Hydrologic downstream 방향은 검증되지 않았으므로 전체 흐름이 반대로 보일 때만 두 speed 부호를 함께 바꾼다. Actor Transform, Mesh, Flow Map registration은 속도 조정에 사용하지 않는다.

V26에서는 속도 외의 Material/Flow Map/Mesh/Actor Transform을 바꾸지 않았다. 20개 Material Instance의 두 scalar override를 새 UE 프로세스에서 다시 읽어 동일 값을 확인했다.

## 15. Tree Visual Scale V25 고정값

강폭과 드론 시점에서 수목이 지나치게 작게 읽히는 문제를 해결하기 위해 기존 `TreeVariation`과 Static Mesh Spawner 가중치만 조정했다. 후보 point, Density, Spacing, Seed, Ground Trace, Water Exclusion, Collision과 River Surface는 변경하지 않았다.

| Layer | Scale Min/Max | A/B/C/D weights | Stored instances | Mean calculated height | P95 height |
|---|---:|---:|---:|---:|---:|
| Forest / Aleppo Pine | `1.05 / 1.35` | `5 / 5 / 1 / 1` | `351,397` | `17.16 m` | `20.89 m` |
| Riparian / Black Alder | `1.00 / 1.25` | `3 / 1 / 2 / 3` | `34,137` | `17.81 m` | `21.36 m` |

Forest Mesh 수량은 A `146,530`, B `146,431`, C `29,279`, D `29,157`이다. Riparian Mesh 수량은 A `11,316`, B `3,780`, C `7,716`, D `11,325`이다. 두 계층 합계는 `385,534` instances이며 V25 전후 총 instance 수는 바뀌지 않았다.

크기 확대는 Actor 수나 draw instance 수를 늘리지 않지만, 화면에서 차지하는 masked foliage pixel과 shadow 영역은 소폭 증가할 수 있다. 따라서 추가 밀도 증가는 live `stat unit`/`stat gpu` 측정 전까지 보류한다.

## 16. Riparian Lower Layer V26 Prototype

다음 값은 생산 전역 설정이 아니라 `Y01_X03` 단일 셀의 시각·성능 시험값이다. 생산용 `PCG_ENV_RiparianTrees`를 복제해 Water hard exclusion과 Riparian influence를 보존하고, Spawner와 간격/scale만 하층 식생용으로 바꿨다.

| Parameter | Value | 설명 |
|---|---:|---|
| Graph | `PCG_ENV_RiparianLowerLayerPrototype` | 생산 Graph와 분리된 시험 Graph |
| Actor | `PCG_ENV_RiparianLowerLayer_Prototype_Y01_X03` | 단일 셀 Prototype |
| Seed | `2601` | 결정적 재생성용 |
| Minimum spacing | `6 m` | 과밀·중첩 제한 |
| Uniform scale | `2.5..4.0` | 원본 24–60cm 저폴리 자산의 가시 크기 보정 |
| Shrub weights | `A/B = 2/2` | 관목 후보 |
| Riparian proxy weights | `A/B = 4/4` | 키 큰 수변 식생 시각 대체 후보 |
| Stored instances | `315` | Shrub `114`, Riparian proxy `201` |
| Collision | Off | 네 ISM component 모두 NoCollision |
| Cast shadow | On | 근경 비교용; 전역 확장 전 성능 판단 필요 |

기술 검증은 통과했지만 후보는 Kenney Nature Kit 기반의 매우 단순한 저폴리 자산이다. 기존 Megaplants와의 시각 스타일 차이가 크고 `RiparianTallPlant`는 식물학적으로 검증된 갈대가 아니다. 따라서 이 Graph는 전역 생산 배치 승인을 받지 않았으며, 사실적 shrub/reed/grass 자산을 지정하거나 저폴리 스타일을 명시적으로 승인하기 전까지 단일 셀 Prototype으로 유지한다.

## 17. Riparian Dense Meadow V31 고정값

V31은 PN Grass 8종을 조밀한 바닥층과 키 큰 이삭층으로 분리한다. 두 층은 같은 좁은 군락 안에서 독립적으로 생성되어 서로 겹칠 수 있다. 아래 값은 `Y01_X03` 단일 셀 Prototype에만 적용되며 64개 셀 전역 값이 아니다.

| Parameter | Value | 설명 |
|---|---:|---|
| Dedicated sampling Graph | `PCG_ENV_RiparianGroundCoverSampling` | 생산 수목 sampler와 밀도 분리 |
| Shared Forest sampler density | `0.0016 points/m²` | 변경하지 않은 생산 기준값 |
| Ground Cover sampler density | `0.75 points/m²` | 좁은 군락 내부의 후보 밀도 |
| Meadow patch threshold | `0.68..1.0` | 군락 면적을 줄이고 군락 사이 공백 확보 |
| Dense / Accent Select ratio | `0.92 / 0.28` | 바닥층 우세, 이삭층도 군락 안에 충분히 혼합 |
| Dense / Accent spacing | `0.5 m / 0.8 m` | 층 내부 최소 간격; 서로 다른 층끼리는 겹침 허용 |
| Dense scale | `(1.25..1.75, 1.25..1.75, 1.15..1.55)` | 폭과 높이를 함께 키운 비균일 clump |
| Accent uniform scale | `1.10..1.55` | 원거리에서도 읽히는 키 큰 이삭층 |
| Dense mesh weights | `4 / 4 / 3 / 3` | 잎층 4종 |
| Accent mesh weights | `4 / 3 / 3 / 2` | 키 큰 이삭층 4종 |
| Stored instances | `64,333` | Dense `49,200`, Accent `15,133` |
| Dense Cull Distance | `50 m / 350 m` | 고밀도 바닥층의 원거리 비용 제한 |
| Accent Cull Distance | `80 m / 500 m` | 드론 시점의 이삭 실루엣 유지 |
| Collision / Shadow | `Off / Off` | 수량이 많은 하층 계층 비용 제한 |
| Generation time | 약 `20.35 s` 이내 | 단일 셀 commandlet 완료 기준; 환경별 편차 있음 |
| Approx. final height | 약 `0.6..1.5 m` | 원본 bounds와 scale에서 계산한 시각 범위 |
| 25 m patch density | 최대 `580`, 중앙값 `162` | 점유된 25 m 셀만 집계 |

Dense Sward Mesh 순서는 다음과 같다.

1. `/Game/PN_GrassLibrary/Meshes/grassMesh/lowGrass_03_02_SM`
2. `/Game/PN_GrassLibrary/Meshes/grassMesh/lowGrass_05_02_SM`
3. `/Game/PN_GrassLibrary/Meshes/grassMesh/lowGrass_09_02_SM`
4. `/Game/PN_GrassLibrary/Meshes/grassMesh/grass_09_07_mesh`

Tall Seedhead Accent Mesh 순서는 다음과 같다.

1. `/Game/PN_GrassLibrary/Meshes/grassMesh/grass_05_03_mesh`
2. `/Game/PN_GrassLibrary/Meshes/grassMesh/grass_12_10_mesh`
3. `/Game/PN_GrassLibrary/Meshes/grassMesh/grass_12_11_mesh`
4. `/Game/PN_GrassLibrary/Meshes/grassMesh/lowGrass_04_03_SM`

선택 8종은 모두 4 LOD를 가진다. 현재 결과를 64개 셀로 단순 복제하면 약 412만 Grass instance가 될 수 있으므로 자동 전역 복제하지 않는다. 생산 전환 시에는 대표 드론 경로의 `stat unit`, `stat gpu`, masked overdraw를 측정하고, 보이는 하천변 셀만 선별하거나 production density/cull 값을 별도로 낮춘다.

River V28 속도는 활성 FlowV24 Material Instance `20/20`에 `PrimarySpeed=-0.024`, `DetailSpeed=-0.052`를 사용한다. V26 대비 각각 약 `9.1%`, `8.3%` 빠르며 Mesh, Flow Map, parent Material과 Actor Transform은 변경하지 않았다.

## 18. Forest Canopy / Near-Bank Layers V32 고정값

### Forest Canopy

| Parameter | Value | 설명 |
|---|---:|---|
| Graph | `PCG_ENV_ForestRegion` | 기존 전역 Forest Graph 재사용 |
| Sampling density | `0.0016 points/m²` | 변경 없음 |
| Minimum spacing | `15 m` | `TreeSpacingBounds ±750 cm` |
| Uniform scale | `1.22..1.55` | 수관의 항공뷰 가시성 확대 |
| Species weights A/B/C/D | `5 / 5 / 1 / 1` | 변경 없음 |
| Stored actors / instances | `64 / 367,702` | 빈 tile `0` |
| Mesh counts A/B/C/D | `153,336 / 153,226 / 30,646 / 30,494` | fresh reload readback |
| Tile count min/avg/p95/max | `1,579 / 5,745.34 / 8,603 / 10,577` | 저장된 64개 Actor 기준 |
| Collision / Shadow | `Off / On` | 기존 생산 정책 유지 |

### Near-Bank Lower Layer Prototype

| Parameter | Value | 설명 |
|---|---:|---|
| Actor | `PCG_ENV_RiparianLowerLayer_Prototype_Y01_X03` | 단일 셀 Prototype |
| Near-bank density threshold | `0.38..1.0` | Outer zone 제거, Near zone 유지; Water는 앞선 Difference로 제거 |
| Dense spacing / scale | `0.4 m` / `(1.35..1.85, 1.35..1.85, 1.30..1.75)` | 좁고 조밀한 바닥층 |
| Accent spacing / scale | `0.65 m` / `1.25..1.80` uniform | 키 큰 이삭층 |
| Young Alder ratio / spacing / scale | `0.025` / `12 m` / `0.35..0.60` | 하천변 보조 식생과 그림자 |
| Dense / Accent / Alder instances | `10,983 / 3,375 / 67` | 총 `14,425` |
| 25 m cells | 점유 `79`, 최대 `580`, 중앙값 `139` | 군락 집중도 |
| Dense cull / policy | `50..350 m`, NoCollision, Shadow Off | 고밀도 masked Grass 비용 제한 |
| Accent cull / policy | `80..500 m`, NoCollision, Shadow Off | 중거리 이삭 실루엣 |
| Alder cull / policy | `100..1,000 m`, NoCollision, Shadow On | 저수량 그림자 계층 |
| Generation time | 약 `19.21 s` | 단일 셀 commandlet 측정 |

V32 Prototype을 64셀에 단순 복제하지 않는다. Grass는 수 km 항공뷰에서 보이도록 유지하는 계층이 아니며, 근·중거리 하천변 디테일이다. 전역 전환은 하천과 실제로 교차하거나 대표 드론 경로에서 보이는 셀을 우선 선별하고 `stat unit`, `stat gpu`, masked overdraw를 확인한 뒤 진행한다.

## 19. Forest Density / Grass Visibility V33 고정값

### Forest Canopy

| Parameter | Value | 설명 |
|---|---:|---|
| Sampling density | `0.0020 points/m²` | V32 `0.0016`에서 증가 |
| Minimum spacing | `15 m` | `TreeSpacingBounds ±750 cm`, 변경 없음 |
| Uniform scale | `1.22..1.55` | 변경 없음 |
| Species weights A/B/C/D | `5 / 5 / 1 / 1` | 변경 없음 |
| Stored actors / instances | `64 / 419,965` | 빈 tile `0`, ISM `4/tile` |
| Mesh counts A/B/C/D | `175,367 / 174,881 / 34,892 / 34,825` | fresh reload readback |
| Tile count min/avg/p95/max | `1,852 / 6,561.95 / 9,867 / 11,933` | 저장된 64개 Actor 기준 |
| V32 대비 증가 | `+52,263 / +14.22%` | scale·spacing 불변 상태의 밀도 증가 |
| Collision / Shadow | `Off / On` | 기존 생산 정책 유지 |

### Near-Bank Lower Layer Prototype

| Parameter | Value | 설명 |
|---|---:|---|
| Dense spacing / scale | `0.4 m` / `(1.55..2.10, 1.55..2.10, 1.50..2.05)` | 분포 유지, clump 가시 크기 증가 |
| Accent spacing / scale | `0.65 m` / `1.45..2.05` uniform | 분포 유지, 이삭 높이 증가 |
| Dense / Accent / Alder instances | `10,983 / 3,375 / 67` | 총 `14,425`, V32와 동일 |
| Dense cull / policy | `50..500 m`, NoCollision, Shadow Off | 기존 350 m보다 중거리 가시성 증가 |
| Accent cull / policy | `80..700 m`, NoCollision, Shadow Off | 기존 500 m보다 중거리 가시성 증가 |
| Alder cull / policy | `100..1,000 m`, NoCollision, Shadow On | 변경 없음 |
| Prototype generation time | 약 `18.76 s` | commandlet 측정 |

Grass의 sampler density `0.75`, Near-bank threshold `0.38`, Dense/Accent selection ratio `0.92/0.28`, Young Alder ratio `0.025`는 변경하지 않았다. 따라서 V33은 풀 개수를 늘리는 단계가 아니라 동일한 군락을 조금 더 크게 읽히게 하는 단계다. 전역 배포와 cull 확대의 성능 승인은 별도다.

## 20. Selective Riparian Lower-Layer Production V34 고정값

| Parameter | Value | 설명 |
|---|---:|---|
| Production Graph | `PCG_ENV_RiparianLowerLayerPrototype` | V33에서 승인한 기존 Graph 재사용 |
| Actor folder | `Environment/PCG/Production/Riparian/LowerLayer` | Outliner 생산 경로 |
| Generation trigger | `Generate On Demand` | Editor 생성 결과를 Level에 저장 |
| River-active input cells | `20` | 저장 River Surface와 교차하는 셀 |
| Saved lower-layer actors | `19` | 빈 `X05_Y00`은 저장하지 않음 |
| Special bounded actor | `PCG_ENV_RiparianLowerLayer_Y01_X04_Q01` | 전체 X04_Y01 대신 유효한 1/4만 저장 |
| Dense instances | `490,853` | 4종 합계 |
| Accent instances | `149,289` | 4종 합계 |
| Young Alder instances | `2,652` | A/B/C/D 합계 |
| Total instances | `642,794` | fresh reload exact readback |
| Dense Cull | `50..500 m` | Collision Off, Shadow Off |
| Accent Cull | `80..700 m` | Collision Off, Shadow Off |
| Young Alder Cull | `100..1,000 m` | Collision Off, Shadow On |
| Per-cell automation guard | `175,000` | 확인된 최대 셀 `X05_Y02=162,895` 위의 좁은 안전 상한 |
| Runtime generation | `Off` | 저장 ISM만 사용; offline 실행 경로 |

셀별 instance 수의 차이는 동일한 Graph가 각 셀의 Riparian mask, Water exclusion, Landscape validity와 patch threshold를 통과한 결과다. 전역 균등 density로 정규화하지 않는다. `X05_Y02`의 `162,895`개는 높은 적합 면적을 가진 셀로 검증됐으나, 실제 GPU 비용은 아직 측정하지 않았으므로 이 값을 최종 최적화 승인으로 해석하지 않는다.

다음 Forest 보강의 원칙은 현재 전역 density `0.0020 points/m²`, spacing `15 m`, scale `1.22..1.55`를 기준선으로 유지하고 높은 suitability 구간에만 조건부 추가 밀도를 주는 것이다. 구체적인 threshold와 추가 density는 드론 경로의 시각 검수와 성능 측정 후 확정한다.
## 21. Forest High-Suitability Densification V35 고정값

| 항목 | 값 | 상태/의미 |
|---|---:|---|
| Production Sampling Density | `0.002 point/m²` | 기존 생산 값, 변경 없음 |
| Prototype Sampling Density | `0.004 point/m²` | 고적합 시험 후보 확보용 |
| High Suitability Threshold | `0.65` | 기존 Mask Density 기준 |
| Base Minimum Spacing | `15m` | Density `< 0.65` 영역 |
| High-Suitability Minimum Spacing | `12m` | Density `>= 0.65` 영역에만 적용 |
| Tree Scale | `1.22–1.55` | 기존 V33 값 유지 |
| Species Weight A/B/C/D | `5/5/1/1` | 기존 V33 값 유지 |
| Prototype Target | `PCG_ENV_Forest_Y00_X00` | 전역 배포 전 단일 셀 |
| Baseline Instance Count | `7,258` | V33 생산 Graph |
| V35 Instance Count | `8,272` | 선택적 증밀 후 |
| Increase | `1,014 / 13.9708%` | 전체 일괄 증밀이 아님 |
| Minimum XY Center Spacing | `1,215.388cm` | 자동 검사 결과 |
| Pairs Below 10m | `0` | 자동 검사 결과 |

현재 거리 최적화 기준값은 다음과 같다.

| 항목 | 현재값 | 처리 원칙 |
|---|---:|---|
| Forest ISM Start Cull Distance | `0` | 아직 시험값 미적용 |
| Forest ISM End Cull Distance | `0` | 실제 드론 시점 측정 후 결정 |
| Forest Shadow | `On` | 원거리 Shadow 시험 전 |
| Camera FOV | `90°` | 실제 Pawn Blueprint 확인값 |
| Spring Arm Length | `10cm` | 실제 Pawn Blueprint 확인값 |
| World Partition/HLOD | 미사용 | 이번 단계에 구조 변경하지 않음 |

Cull Distance 숫자는 실제 비행 고도와 Frame Time 측정 없이 임의로 고정하지 않는다. Tree, Riparian Tree, Grass는 화면 기여도와 비용이 다르므로 Category별로 별도 기준을 사용해야 한다.

## 22. 거리 기반 최적화 적용 순서

1. V35 시험 셀의 시각적 밀도 승인
2. 대표 드론 고도에서 `stat unit`, `stat gpu`, `stat rhi` 기록
3. 시험 셀 하나에서 Tree Cull/Shadow 조정
4. 작은 Riparian/Grass에 더 짧은 거리 적용
5. 전역 적용 후 총 Instance 수와 Frame Time 재측정
6. Standalone, Cook, Package, Offline 실행 검증

화면을 흐리게 만드는 Post Process는 최적화 수치가 확인된 뒤 선택적인 미관 효과로만 검토한다.

## 23. Forest Open-Water Exclusion V36 고정값

| 항목 | 값 | 상태/의미 |
|---|---:|---|
| Target Cell | `PCG_ENV_Forest_Y00_X00` | 단일 셀 시험 적용 |
| Confirmed Water Feature | `정안저수지` | OSM `way 119122441`, `water=reservoir` |
| Source Polygon Area | `374 pixel` | 2017×2017 등록 좌표계 |
| Satellite Candidate Intersection | `156 pixel` | 수면 후보와 교차검증 |
| Safety Buffer | `2 pixel / 약 60m` | 수목 중심·수관 안전 여유 |
| Final Mask Area | `657 pixel` | 다른 수역/그림자 제외 |
| V35 Count | `8,272` | 변경 전 기준선 |
| V36 Count | `8,098` | 정안저수지 제외 후 |
| Removed | `174 / 2.1035%` | 해당 셀만 감소 |
| A/B/C/D | `3,343 / 3,359 / 700 / 696` | 네 변형 유지 |
| Minimum XY Center Spacing | `1,215.388cm` | V35와 동일 |
| Pairs Below 10m | `0` | 심한 겹침 없음 |

Texture는 `2017×2017`, Grayscale, sRGB Off, MipMap 없음, Never Stream, Bilinear, Clamp로 고정한다. Mask transform과 Density Merge는 기존 공용 Water mask Subgraph에서 복제한 `±3,024,000cm`, `SET`을 유지한다. 자동 위성 색상 분류만으로 전역 호수/그림자를 제거하지 않는다.

## 24. Riparian Lower-Layer V37 고정값

| 항목 | 기존 V34/V33 | V37 Prototype | 상태/의미 |
|---|---:|---:|---|
| Target Actor | `Y01_X03` | `Y01_X03` | 단일 셀 A/B 비교 |
| Graph | `PCG_ENV_RiparianLowerLayerPrototype` | `PCG_ENV_RiparianLowerLayerOptimizedPrototypeV37` | 원본 Graph 보존, 생산 Actor 19개와 Prototype에서 V37 사용 |
| Dense ratio | `0.92` | `0.60` | 선택 point 감소 |
| Accent ratio | `0.28` | `0.20` | 선택 point 감소 |
| Young Alder ratio | `0.025` | `0.025` | 변경 없음 |
| Dense XY scale | `1.55..2.10` | `1.65..2.25` | footprint 보완 |
| Dense Z scale | `1.50..2.05` | `1.50..2.05` | 변경 없음 |
| Dense mesh replacement 1 | `lowGrass_03_02_SM` | `lowGrass_07_03_SM` | 더 넓은 footprint, 4 LOD |
| Dense mesh replacement 2 | `grass_09_07_mesh` | `lowGrass_07_02_SM` | 더 낮은 LOD0 triangle, 4 LOD |
| Dense Cull | `50..500 m` | `50..500 m` | 변경 없음 |
| Accent Cull | `80..700 m` | `80..700 m` | 변경 없음 |
| Young Alder Cull | `100..1,000 m` | `100..1,000 m` | 변경 없음 |
| Grass Collision / Shadow | `Off / Off` | `Off / Off` | 변경 없음 |
| Alder Collision / Shadow | `Off / On` | `Off / On` | 변경 없음 |
| Total instances | `14,425` | `9,558` | `-4,867 / -33.74%` |
| Dense / Accent / Alder | `10,983 / 3,375 / 67` | `7,094 / 2,397 / 67` | Alder 보존 |
| `instance × LOD0 triangle` | `6,078,272` | `3,208,107` | `-47.22%`, 정적 비용 proxy |

V37 Prototype 검증 후 비용이 가장 큰 `4`개 셀을 작은 batch로 먼저 승격했고, 사용자 시각 승인 뒤 나머지 `15`개 생산 영역도 같은 acceptance로 순차 승격했다. 생산 Actor `19`개는 모두 V37을 사용하고 원본 Graph를 참조하는 생산 Actor는 `0`개다. 생산 전체는 `642,794 → 429,498` instances이며 Prototype을 포함하면 `657,219 → 439,056`이다. 실제 드론 Frame Time과 GPU overdraw는 별도 측정 항목이다.

## 25. River V38 / V37 Production Batch 고정값

| 항목 | 값 | 상태/의미 |
|---|---:|---|
| River V38 target tiles | `X00_Y00`, `X04_Y04` | 나머지 18타일 V23 유지 |
| Validated short gaps | `4` | 동일 VisualMatch 중심선이 잇는 `<97.5m` 공백 |
| Bridge width | `3 pixel` | 약 30m/pixel 입력에서 보수적 연결 |
| Added mask cells | `23` | X00 `10`, X04 `13` |
| Connected components | `24 → 20` | 네 공백만 병합 |
| X00 triangles | `3,664 → 3,744` | inverted/degenerate `0/0` |
| X04 triangles | `10,936 → 11,040` | inverted/degenerate `0/0` |
| River Material / Flow | `V24 per-tile MI / FlowMap` | 20/20 exact match |
| River Collision / Shadow / Nanite | `Off / Off / Off` | V38 target Nanite 명시 비활성화 |
| V37 production actors | `19` | 활성 하층 식생 생산 영역 전체 |
| Source-Graph production actors | `0` | 원본 Graph Asset만 rollback용으로 보존 |
| V37 batch instances | `374,334 → 250,154` | `-124,180 / -33.17%` |
| Production lower total | `642,794 → 429,498` | Actor 19 유지, `-33.18%` |
| Prototype lower total | `14,425 → 9,558` | V37 Prototype 유지 |
| Current lower total | `439,056` | 생산 + Prototype, 기존 `657,219` 대비 `-33.19%` |
| Total triangle-work proxy | `277,007,269 → 147,103,423` | `-46.90%`, 실제 Frame Time 아님 |
| Runtime generation | `Off` | 저장 Static Mesh/ISM, GIS/network 접근 없음 |

## 26. V46 강변 피복 및 산림 적합도 전역 배포 고정값

| 항목 | 값 | 상태/의미 |
|---|---:|---|
| Riparian target | `PCG_ENV_RiparianLowerLayer_Y01_X03` | V46 시각 피복 검증 구역 |
| Grass XY multiplier | `1.42` | Z scale과 point 위치 불변 |
| Grass coverage-area proxy | `2.0164×` | 인스턴스 증가 없이 겹침 강화 |
| Grass / Young Alder instances | `176,170 / 413` | V45 수량 보존 |
| Forest actors | `64` | non-empty 64/64 |
| Shared V46 forest graph | `63 actors` | 일반 생산 셀 전체 |
| Special open-water forest graph | `1 actor` | `Y00_X00` 정안저수지 보정 보존 |
| Standard spacing | `15m` | suitability `<0.65` |
| High-suitability spacing | `12m` | suitability `≥0.65` |
| Candidate sampling | `0.004 points/m²` | 분기 전 후보 확보 |
| Forest instances | `475,313` | 전역 배포 후 저장값 |
| Forest increase | `53,528 / 12.69%` | V46 전역 배포 직전 대비 |
| Forest min/max per tile | `2,131 / 13,809` | 빈 Tile 없음 |
| Runtime generation | `Off` | Editor 생성 ISM 저장 |

Grass V46의 `2.0164×`는 실제 GPU 비용이 아니라 Mesh XY footprint의 면적 비율이다. Forest 증가도 전체 Density를 균등하게 올린 결과가 아니라 동일 Graph가 각 Tile의 고적합도 density에 반응한 결과다.

## 27. River Grounding / Riparian Scale V47 고정값

| 항목 | 값 | 상태/의미 |
|---|---:|---|
| River target | `ENV_RiverSurface_Production_X03_Y01` | 사용자 표시 타일만 보정 |
| River mesh | `SM_ENV_RiverSurface_Daejeon_v42_X03_Y01` | 기존 Landscape 추종 Mesh 유지 |
| Actor Z | `0 → -35cm` | Geometry/Material/XY transform 불변 |
| Predicted min / p01 / median / p99 gap | `-10.0 / 19.79 / 34.32 / 47.84cm` | 경계 틈을 숨기기 위한 국소 10cm overlap 허용 |
| Riparian target | `PCG_ENV_RiparianLowerLayer_Y01_X03` | 단일 시각 검증 구역 |
| Graph | `PCG_ENV_RiparianUltraLushTallPrototypeV47` | V46 point/mask/exclusion 보존 |
| XY scale multiplier vs V46 | `1.15` | 피복 면적 proxy `1.3225×` |
| Z scale multiplier vs V46 | `1.20` | 항공뷰 실루엣 강화 |
| Dense scale min/max | `6.2054,6.2054,1.92 / 7.8384,7.8384,2.58` | V47 저장값 |
| Accent scale min/max | `4.4091,4.4091,1.86 / 5.8788,5.8788,2.58` | V47 저장값 |
| Grass / Young Alder instances | `176,170 / 413` | V46 수량 완전 보존 |
| Grass Collision / Shadow | `Off / Off` | 기존 성능 정책 유지 |

River Z 보정은 다른 `19`개 River Actor에 적용하지 않는다. Grass V47은 크기만 바꾸므로 point 수나 PCG generation 비용을 늘리지 않지만, 화면 피복과 masked overdraw는 증가할 수 있다. 실제 비행 Frame Time은 최종 최적화 단계에서 별도 측정한다.

## 28. Global Riparian V49 고정값

| 항목 | 값 | 상태/의미 |
|---|---:|---|
| Mask resolution | `6048×6048, 10m/pixel` | 전역 River 20셀 공용 |
| Water/safety | `0..10m, density 0` | terrestrial Grass hard reject |
| Near bank | `10..30m, density 255` | 연속 고밀도 |
| Middle transition | `30..60m, density 160` | broad noise 70% |
| Outer transition | `60..90m, density 64` | broad noise 30% |
| Standard sampling | `0.10 point/m²` | 18개 V49 셀 |
| Budget sampling | `0.08 point/m²` | X04_Y04 한 셀 |
| Reference | `V47 / 176,583` | X03_Y01 보존 |
| Total instances | `1,696,360` | Grass 1,692,399 + Alder 3,961 |
| Runtime generation | `Off` | Editor 생성 ISM 저장 |

Mesh Scale과 category별 Collision/Shadow/Cull은 V47을 그대로 사용한다. 셀별 수량과 검증 근거는 `Docs/PCG_RIPARIAN_GLOBAL_V49.md`를 참조한다.

## 29. Conditional Far-Bank V50 고정값

| 항목 | 값 | 상태/의미 |
|---|---:|---|
| 대상 셀 | `X02_Y01`, `X03_Y01`, `X04_Y04`, `X07_Y05` | 거리/연속성 감사로 선택 |
| Sampling | `0.015 point/m²` | V49 standard의 15% |
| Mask threshold | `0.60` | near+middle, 약 10–60m |
| Dense scale | `8.69..10.97 XY`, `1.92..2.58 Z` | 원거리 피복용 큰 clump |
| Accent scale | `6.17..8.23 XY`, `1.86..2.58 Z` | 원거리 실루엣 보조 |
| Dense/Accent Cull | `1,800..2,200m` | 네 보조 Actor에만 적용 |
| Young Alder ratio | `0` | 원거리 나무 중복 없음 |
| Collision / Shadow | `Off / Off` | Grass 전용 정책 |
| Saved instances | `73,764` | V49 LowerLayer 대비 `4.3484%` |
| Runtime generation | `Off` | Editor 생성 ISM 저장 |

다른 16개 River 셀에는 이 계층을 추가하지 않는다. 기존 V49/V47 LowerLayer 수량과 Cull은 변경하지 않는다.

## 30. X02 Far-Bank Dense V51 고정값

| 항목 | 값 | 상태/의미 |
|---|---:|---|
| Target Actor | `PCG_ENV_RiparianFarBank_Y01_X02` | 한 셀 전용 예외 |
| Dedicated Graph | `PCG_ENV_RiparianFarBankDenseX02V51` | 공용 V50 보존 |
| Dedicated Sampling | `PCG_ENV_RiparianFarBankDenseSamplingV51` | `0.06 point/m²` |
| Density multiplier vs V50 | `4.0×` | `0.015 → 0.06` |
| Bank threshold / band | `0.60 / 약 10–60m` | V50 동일, 0–10m water safety 유지 |
| Dense scale | `8.69..10.97 XY`, `1.92..2.58 Z` | V50 동일 |
| Accent scale | `6.17..8.23 XY`, `1.86..2.58 Z` | V50 동일 |
| Cull / Collision / Shadow | `1,800..2,200m / Off / Off` | V50 동일 |
| X02 FarBank instances | `29,563` | 기존 `7,296`, 셀 상한 `40,000` 통과 |
| All FarBank instances | `96,031` | LowerLayer의 `5.6610%` |
| Runtime generation | `Off` | Editor 생성 ISM 저장 |

다른 세 FarBank Actor는 공용 V50 `0.015 point/m²`를 유지한다. 따라서 이 값은 전역 강변 density가 아니라 `X02_Y01`의 넓고 비어 보이던 bank를 위한 국소 품질 예외다.

## 31. Unified Riparian Bank V52 고정값

| 항목 | 값 | 상태/의미 |
|---|---:|---|
| Unified cells | `X02_Y01`, `X03_Y01`, `X04_Y04`, `X07_Y05` | 기존 LowerLayer/FarBank 중복 쌍 |
| Shared sampling | `0.06 point/m²` | X03, X04, X07 |
| X02 sampling | `0.12 point/m²` | 넓은 X02 bank 전용 |
| Bank threshold / band | `0.60 / 약 10–60m` | 0–10m water safety 유지 |
| Dense scale | `9.559..12.067 XY`, `1.92..2.58 Z` | V51 대비 XY `1.10×` |
| Accent scale | `6.787..9.053 XY`, `1.86..2.58 Z` | V51 대비 XY `1.10×` |
| Cull / Collision / Shadow | `1,800..2,200m / Off / Off` | 단일 Grass 계층 |
| Young Alder | `Disabled` | 네 셀의 나무 중복 없음 |
| FarBank Actor | `0` | 네 중복 Actor 제거 |
| Unified target instances | `323,559` | 기존 두 계층 `659,437` |
| Total LowerLayer instances | `1,456,513` | 20개 Actor |
| Total reduction | `335,878 / 18.74%` | 기존 Lower+Far 전체 대비 |
| Runtime generation | `Off` | Editor 생성 ISM 저장 |

다른 16개 LowerLayer의 Graph와 인스턴스 수는 exact unchanged다. V50/V51 Graph Asset은 rollback용으로 보존하지만 저장 Level에서 참조하는 FarBank Actor는 없다.

## 32. Vegetation Rendering V55 고정값

| 항목 | 값 | 상태/의미 |
|---|---:|---|
| Forest Start/End Cull | `350,000 / 500,000cm` | `3.5–5.0km` fade, Nanite 근·중거리 유지 |
| Forest WPO Disable | `100,000cm` | 향후 Tree wind도 1km 밖에서는 평가하지 않음 |
| Riparian Grass WPO Disable | `25,000cm` | 250m 안의 기존 바람만 유지 |
| Young Alder WPO Disable | `100,000cm` | Tree 계층 공통 상한 |
| Riparian Grass Ray Tracing | `Off` | Raster main pass는 유지 |
| Forest/Alder Ray Tracing | `On` | 큰 수목의 반사·간접광 유지 |
| Grass Collision / Shadow | `Off / Off` | 드론 통과 및 기존 성능 정책 |
| Tree Collision / Shadow | `Off / On` | 드론 통과, 수목 그림자 유지 |
| UE RT ISM Culling | `On` | 기존 UE 5.7.4 per-instance/cluster culling 유지 |
| UE RT ISM Cluster / Low-scale Radius | `10,000 / 1,000cm` | 엔진 실제 실행값, 별도 override 없음 |
| UE RT Global Radius | `30,000cm` | `r.RayTracing.Culling=3`, 별도 override 없음 |

V55는 instance count, 위치, scale, rotation, PCG seed와 bank/forest mask를 변경하지 않는다. `WorldPositionOffsetDisableDistance`와 `VisibleInRayTracing`은 Live ISM Component뿐 아니라 현재 생산 Graph의 Mesh Spawner descriptor에도 저장한다.

## 33. Y00_X02 국소 밀도 V56

| 항목 | 값 | 의미 |
|---|---:|---|
| 적용 Actor | `PCG_ENV_RiparianLowerLayer_Y00_X02` | 한 셀만 보정 |
| Sampling Density | `0.10 → 0.20 point/m²` | 공용 V49는 유지 |
| Grass | `31,070 → 62,164` | 약 `2.001×` |
| Young Alder | `87 → 133` | 기존 선택 규칙의 재생성 결과 |
| Bank/Water Mask | V49 exact retained | 강변 폭 확장·수면 침범 없음 |
| Grass WPO / RT / Collision / Shadow | `250m / Off / Off / Off` | V55 정책 유지 |
| Total LowerLayer | `1,487,653` | V55 대비 `+31,140 / +2.14%` |

## 34. X02_Y01 Flow Axis V57 고정값

| 항목 | 값 | 의미 |
|---|---:|---|
| Target Actor | `ENV_RiverSurface_Production_X02_Y01` | 큰 본류 횡방향 흐름 보정 |
| Flow Texture | `T_ENV_RiverFlow_Daejeon_V57_X02_Y01` | 256×256 RGBA vector map |
| Corrected component | largest disconnected water component | 작은 상단 수로는 V24 벡터 유지 |
| Vector transform | `(x, y) → (-y, x)` | 본류 벡터 90도 회전 |
| Long-axis alignment | `0.141857 → 0.988330` | absolute dot-product mean |
| Primary / Detail Speed | `-0.024 / -0.052` | V28 값 exact retained |
| Map UV Bias | `(1.939453125, 2.939453125)` | V24 registration exact retained |
| Water / Sampling mask | pixel-exact retained | 강 형상 및 경계 변화 없음 |
| Collision / Shadow | `Off / Off` | 기존 River 정책 유지 |
| Runtime generation | 없음 | Editor-authored Static Mesh + MI |

## 35. X02_Y00 Flow Axis V58 고정값

| 항목 | 값 | 의미 |
|---|---:|---|
| Target Actor | `ENV_RiverSurface_Production_X02_Y00` | 실제 사용자 표시 시야가 교차하는 수면 |
| Flow Texture | `T_ENV_RiverFlow_Daejeon_V58_X02_Y00` | 256×256 RGBA vector map |
| Water / Sampling components | `1 / 1` | 단일 연속 수로 |
| Water / Sampling pixels | `729 / 1,795` | mask는 V24와 동일 |
| Target long axis | `(-0.13526456, 0.99080952)` | 기존 평균 흐름 부호 유지 |
| Long-axis alignment | `0.936608 → 0.999998` | absolute dot-product mean |
| Primary / Detail Speed | `-0.024 / -0.052` | V28 값 exact retained |
| Map UV Bias | `(1.939453125, 3.939453125)` | V24 registration exact retained |
| Primary Normal | `T_River_Waves01_Normals` | 기존 질감 유지 |
| Collision / Shadow | `Off / Off` | 기존 River 정책 유지 |
| Runtime generation | 없음 | Editor-authored Static Mesh + MI |

V58은 전역 River 방향값이 아니다. `X02_Y00` 한 component에서만 적용되는 보수적 예외이며, 나머지는 V24 또는 검증된 `X02_Y01` V57을 사용한다.

## 36. X02 Fixed-Axis Motion V59 고정값

| 항목 | 값 | 의미 |
|---|---:|---|
| Target Actors | `X02_Y00`, `X02_Y01` | 사용자 표시 구간과 직접 인접 타일 |
| Parent | `M_ENV_RiverSurface_FixedAxisMotion_V59` | V24 기반의 대상 전용 Parent |
| Longitudinal Axis | `(-0.10452846, 0.99452190)` | 사용자 빨간 표시의 화면 수직/강 장축 |
| Longitudinal Tiling | `0.00010 cm⁻¹` | 약 `100m` 주기의 저주파 이동 띠 |
| Longitudinal Speed | `-0.015 cycle/s` | 생산 이동 속도 |
| Longitudinal Strength | `0.085` | BaseColor/Roughness 저강도 혼합 |
| Longitudinal Roughness | `0.22` | 이동 띠의 목표 Roughness |
| Highlight Color | `(0.020, 0.115, 0.125, 1.0)` | 기존 수색과 가까운 저채도 강조색 |
| Longitudinal Phase Offset | `0.0` | 생산에서는 고정; 진단 캡처에서만 임시 변경 |
| Primary / Detail Speed | `0.0 / 0.0` | 횡방향으로 보이던 비등방성 normal panning 정지 |
| Moving Highlight Strength | `0.0` | 기존 Flow Map 기반 횡방향 가시 신호 제거 |
| Normal Strength | `0.35` | 정적 표면 질감 보존 |
| Flow Maps | V58 `X02_Y00`, V57 `X02_Y01` | Water mask와 UV registration 보존 |
| Collision / Shadow | `Off / Off` | 기존 River 정책 유지 |
| 추가 Texture sample / Actor / Tick | `0 / 0 / 0` | 두 대상 Actor에만 소량 ALU 추가 |

V59은 전역 River 파라미터가 아니다. 나머지 18개 River Actor는 V24 Parent와 기존 속도를 유지한다. `LongitudinalStrength`나 `Speed`를 조정할 때는 두 대상 MI를 함께 변경해 타일 경계의 시간·명암 불일치를 피한다.

## 37. Global Local-Tangent Flow V60 고정값

| 항목 | 값 | 의미 |
|---|---:|---|
| Target Actors | River Production `20/20` | V57/V58/V59 활성 예외를 전역 일관 방식으로 대체 |
| Flow Texture | `20 × 256×256 RGBA` | 타일별 V60 Texture |
| Parent Material | V24 shared parent | `87` expressions, 새 Parent 없음 |
| Local tangent radii | `14 / 24 / 36px` | 굴곡 크기에 따라 bounded local PCA 선택 |
| Direction sign | 기존 Flow vector와의 dot product | 국소 tangent의 180° 부호 모호성 제거 |
| Valid tangent coverage | `73,013 / 75,239` (`97.041%`) | 나머지는 보간·기존 유효 방향 보존 |
| Water / Sampling mask | `20/20 pixel-exact retained` | 수면 형상·경계·UV 불변 |
| Primary / Detail Speed | V24/V28 MI 값 exact retained | 사용자가 승인한 유속 유지 |
| Collision / Shadow | 기존 Actor 값 exact retained | Flow 교정에서 동작 정책 변경 없음 |
| Mesh / Transform / Tags | `20/20 exact retained` | 수면 접지·gap bridge 개선 메시 보존 |
| Runtime Actor / Tick / PCG | `0 / 0 / 0` 추가 | Editor-authored Texture + 기존 MI 구조 |

V59의 `LongitudinalAxis`와 전용 Parent는 현재 Level에서 사용하지 않는다. V60 Texture도 현재 화면 재질이 아니라 V61 위상 생성의 방향 원본이다. V60의 RG 벡터만 바꿔서는 V24 Parent의 비등방성 animated normal panning 방향이 바뀌지 않으므로, V60 단독을 생산 화면 수정으로 취급하지 않는다.

## 38. Global Longitudinal Phase V61 고정값

| 항목 | 값 | 의미 |
|---|---:|---|
| Target Actors | River Production `20/20` | 모든 활성 River에 같은 방식 적용 |
| Phase Texture | `20 × 256×256 RGBA` | `R/G=cos/sin phase`, `B/A=V60 mask` |
| Shared Parent | `M_ENV_RiverSurface_LongitudinalPhase_V61` | Parent 한 개, `56` expressions |
| Material Instances | `20` | 타일 ID와 Phase Texture 1:1 |
| Phase integration source | V60 RG | 국소 tangent를 read-only 입력으로 사용 |
| Phase step | `0.12 cycle/pixel` | 오프라인 위상 생성값 |
| Longitudinal Speed | `-0.14 cycle/s` | V62에서 12.5% 감속; 진행 방향과 phase는 유지 |
| Longitudinal Strength | `0.18` | BaseColor/Roughness 이동 신호 강도 |
| Longitudinal Roughness | `0.20` | 이동 띠의 목표 Roughness |
| Normal Strength | `0.30` | 정적 표면 미세 질감 강도 |
| Static normals | `T_Water_Normal_Subtle` × 2 scales | animated normal panning 없음 |
| Texture samples | `3` | V24 `5` 대비 `-2` |
| Blend / WPO | `Opaque / Off` | 수면 형상과 접지 불변 |
| Collision / Shadow | `Off / Off` | 기존 River 정책 유지 |
| Runtime Actor / Tick / PCG | `0 / 0 / 0` 추가 | 저장 Static Mesh + MI만 사용 |

`LongitudinalPhaseOffset=0.0`이 생산값이다. 방향 검증 때만 `0.125` 간격으로 일시 변경하고 저장하지 않는다. 속도 부호를 바꾸면 진행 방향만 반전하며, 굴곡 추종 축은 Phase Texture의 공간 gradient가 결정한다. 강도 조정은 `LongitudinalStrength`를 우선 사용하고, 방향 문제를 강도나 Normal Texture 회전으로 우회하지 않는다.

## 39. Targeted River Surface Contact V62 고정값

| 항목 | 값 | 의미 |
|---|---:|---|
| Shape fix | `X03_Y00`, source pixel `(880, 86)` | 내부 단일 hole만 채움 |
| Shape active cells | `1157 → 1158` | 한 셀 이외 수면 mask 불변 |
| Grounded tiles | `X02_Y00`, `X02_Y01`, `X03_Y01` | 사용자 표시 접지·경계 대상 |
| Contact subdivision | V23 triangle `×16` | 약 `3.75m` 높이 추종 해상도 |
| Common baked clearance | `28.83579699cm` | 세 grounded tile 동일 적용 |
| Minimum sampled final gap | `8.00cm` | Landscape 관통 방지 guard |
| Y01 shared refined points | `178` | X02/X03 경계 비교 표본 |
| Y01 maximum height delta | `0.0cm` | 동일 XY에서 수면 높이 일치 |
| X03_Y00 effective clearance | `175cm` | 기존 V23 시각 정책 유지 |
| Longitudinal Speed | `-0.14 cycle/s` | 기존 `-0.16`에서 12.5% 감속 |
| X02_Y01 LowerLayer instances | `58,533` | 전체 20구역 median `77,602.5`보다 낮아 미감축 |
| Collision / Shadow | `Off / Off` | 기존 River 정책 유지 |
| Runtime Actor / Tick / PCG | `0 / 0 / 0` 추가 | 저장 Static Mesh + 기존 MI |

접지 수정과 유속 수정은 독립 파라미터다. 접지 문제를 Actor Z offset으로 다시 우회하지 않으며, 이후 속도 조정 시 20개 `FlowV61` MI를 같은 값으로 유지해 타일별 시간 불일치를 방지한다.

## 40. X02_Y00–X03_Y00 Seam V63/V64 고정값

| 항목 | 값 | 의미 |
|---|---:|---|
| V62 legacy Y00 clearance delta | `146.164203cm` | X02 `28.8358cm`, X03 `175cm`의 불일치 |
| V63 target mesh | `X03_Y00` 한 개 | X02는 accepted V62 Mesh 유지 |
| V63 common clearance | `28.83579699cm` | X02/X03 동일 terrain-contact 정책 |
| Shared refined edge | `65 points` | maximum surface Z delta `0.0cm` |
| X03 sampled gap min / median | `15.3592 / 28.8358cm` | Landscape 관통 없음 |
| V61 raw seam phase delta mean / max | `43.008° / 49.248°` | 서로 다른 Clamp 끝 texel 샘플이 명암선 생성 |
| Padded phase texture size | `258×258` | 원본 256×256 interior 유지 + 1px gutter |
| PhaseMapUVScale | `1.2926304985e-6` | 기존 값 × `256/258` |
| X02 PhaseMapUVBias XY | `(1.9282945736, 3.9127906977)` | interior texel center 재등록 |
| X03 PhaseMapUVBias XY | `(0.9360465116, 3.9127906977)` | interior texel center 재등록 |
| Shared-edge texel coordinate | X02 `256.5`, X03 `0.5` | 양쪽이 같은 두 RGBA texel을 50:50 보간 |
| LongitudinalSpeed | `-0.14 cycle/s` | `20/20` exact retained |
| Texture sample / draw-call delta | `0 / 0` | 기존 V61 Parent 유지 |
| Additional source pixels | `2,056` total | 각 texture `+1.5686%` |

Gutter는 `Clamp`와 `Bilinear`를 전제로 한다. 한쪽 Texture에만 gutter를 추가하면 경계 양쪽의 보간식이 달라지므로 허용하지 않는다. `FlowMapPhase` interior나 `LongitudinalSpeed`를 경계 숨김 용도로 변경하지 않는다.

## 41. Global River Contact V65 고정 파라미터

| Parameter | Value | Purpose |
|---|---:|---|
| `BaseClearanceCm` | `28.835796991983898` | 전체 20개 River tile의 공통 기본 Landscape 간격 |
| `MinTriangleCenterGapCm` | `8.0` | 모든 triangle center에서 보장하는 최소 비관통 간격 |
| `SubdivisionCount` | `1` | 접지 보정 해상도와 triangle 비용의 균형 |
| `ActorZCm` | `0.0` | mesh에 bake된 접지 높이와 Actor transform의 이중 offset 방지 |
| `LongitudinalSpeed` | `-0.14` | V64에서 확정된 완만한 종방향 유속 유지 |
| `CollisionEnabled` | `False` | 드론과의 불필요한 River surface 충돌 및 collision 비용 방지 |
| `CastShadow` | `False` | 수면의 불필요한 shadow 비용 방지 |

### 41.1 국소 보정 규칙

`BaseClearanceCm`만으로 triangle center 간격이 `8 cm` 미만이 되는 지점에서만 incident vertex를 위로 이동한다. 보정량은 world XY 기준으로 공유되며, tile seam 양쪽의 동일 좌표는 동일한 최종 높이를 사용한다.

현재 검증 결과는 다음과 같다.

- 최소 triangle-center gap: `8.0 cm`
- 1 percentile gap: `12.0159821510315 cm`
- median gap: `28.8357969919839 cm`
- 최대 국소 lift: `73.0156898498531 cm`
- 99 percentile 국소 lift: `44.3533658981327 cm`
- shared boundary 최대 높이 차이: `0 cm`

이 값은 artist-facing River 높이 조절값이 아니라 현재 Landscape와 production River mesh 사이의 접지 일관성을 위한 bake 기준이다. 임의로 Actor Z를 추가 조절하면 V23의 이중 clearance 문제가 다시 발생할 수 있다.

## 42. River Topology V66 / Shared Atlas V67 고정값

| Parameter | Value | Purpose |
|---|---:|---|
| Small enclosed-hole fill limit | `8 source cells` | 점·작은 홀만 메우고 넓은 섬은 보존 |
| Filled cells | `19` | 8-cell `X04_Y03` hole 포함 |
| Removed source/bridge cells | `198` | 약한 지류와 연결용 bridge 제거 |
| Added topology cells | `74` | hole fill 19 + orthogonal support 55 |
| Diagonal-only contacts | `54 → 0 groups` | 점으로만 만나는 사슬형 접촉 제거 |
| Remaining enclosed land | `47 cells`, bbox `[172,582,184,589]` | 의도된 Geum-river island 유지 |
| V66 LOD0 triangles | `961,664` | V65 대비 `-3,968` (`-0.4109%`) |
| Base clearance / min center gap | `28.83579699cm / 8cm` | V65 접지 정책 유지 |
| Shared boundary max height delta | `0cm` | 타일별 단차 방지 |
| V67 atlas size | `514×258` | X02/X03 Y00 phase를 한 texture에 배치 |
| V67 shared-edge normalized U | `0.5000000118` 양쪽 동일 | 경계 bilinear sample 완전 공유 |
| Texture sample / draw-call delta | `0 / 0` | shader/render 구조 유지 |
| Stored phase pixels | `133,128 → 132,612` | 두 기존 texture 대비 `-516` pixels |
| LongitudinalSpeed | `-0.14 cycle/s`, `20/20` | 기존 전역 유속 유지 |

`PCG_EXCL_Manual_Central_01`은 `PCG_Exclude_Vegetation` tag를 유지해야 한다. 현재 bounds extent는 `(20000,20000,50000)cm`이며 수평 전체 크기는 `400m × 400m`다. 제거 여부는 중앙 exclusion을 폐기하고 식생을 재생성할 의도가 명확할 때만 결정한다.

## 43. River Connectivity V68 고정값

| Parameter | Value | Purpose |
|---|---:|---|
| Source raster cell size | `30m` | topology 및 bridge 판단 단위 |
| Initial water cells | `30,052` | accepted V66 topology |
| Final water cells | `32,210` | 승인된 corridor 적용 후 |
| Added / removed cells | `2,158 / 0` | 기존 강을 삭제하지 않고 fracture만 연결 |
| Approved bridge edges | `15` | centerline/same-feature/source-corridor/collinear 근거 기반 |
| 4-neighbor components | `19 → 4` | 실제 연결 근거가 있는 수계 내부 통합 |
| 8-neighbor components | `19 → 4` | point-contact가 아닌 edge-connected 결과 |
| Diagonal-only contacts | `0` | V66 topology 정리 결과 유지 |
| Preserved independent systems | `3` | Jemin 제거 fragment, X05 독립 수로, X07 Geum 계통 |
| Base clearance | `28.835796991983898cm` | V65/V66 전역 접지 정책 유지 |
| Minimum triangle-center gap | `8cm` | Landscape 관통 방지 |
| Contact subdivision | `1` | 접지 해상도와 geometry 비용 절충 |
| Actor Z | `0cm`, `20/20` | baked contact와 transform 이중 offset 방지 |
| V68 LOD0 triangles | `1,030,720` | V66 대비 `+69,056` (`+7.180886%`) |
| LongitudinalSpeed | `-0.14 cycle/s`, `20/20` | V67 흐름 속도 유지 |
| Collision / Shadow | `Off / Off`, `20/20` | 드론 충돌 및 불필요 shadow 비용 방지 |
| Runtime Actor / Tick / PCG | `0 / 0 / 0` 추가 | Editor-baked Static Mesh 방식 유지 |

Component 수 `4`는 실패가 아니라 보수적 acceptance 결과다. 모든 물 조각을 한 component로 만드는 전역 dilation이나 긴 직선 bridge는 사용하지 않는다. 새로운 간격을 연결할 때는 같은 centerline network, 같은 source feature 또는 명확한 source-area corridor 중 하나를 먼저 확인한다.

## 44. River Cleanup V69 고정값

| Parameter | Value | Purpose |
|---|---:|---|
| Parent topology | V68 Connectivity | 기존 승인 수계·bridge 유지 |
| Removed component tile | `X00_Y02` | 독립된 짧은 수로만 제거 |
| Removed component signature | `67 cells`, bbox `[130,660,152,694]` | 다른 수면 cell의 우발적 삭제 방지 |
| Water cells | `32,210 → 32,143` | 정확히 `67` cells 감소 |
| 4-neighbor / 8-neighbor components | `3 / 3` | main, X05, X07 세 독립 수계 유지 |
| Added cells | `0` | 근거 없는 연결·확장 없음 |
| Small enclosed holes / diagonal-only contacts | `0 / 0` | topology 회귀 없음 |
| Base clearance | `28.835796991983898cm` | 기존 전역 접지 기준 유지 |
| Minimum contact gap | `8cm` | Landscape 관통 방지 목표 |
| Dense contact target | `X04_Y04` | 실제 바닥 노출이 확인된 tile에만 적용 |
| Barycentric denominator | `4` | triangle당 vertex 제외 표본 `12`개 |
| Dense sample / unique trace count | `499,968 / 321,408` | triangle 내부 Landscape 보간 검증 |
| Dense deficient samples before lift | `1,888` | center-only 검사가 놓친 국소 접지 후보 |
| Dense final minimum gap | `7.99999957cm` | `0.01cm` OBJ 정밀도 허용 오차 내 `8cm` |
| Dense samples below accepted `7.99cm` | `0` | 최종 접지 acceptance |
| Cross-tile height delta | `0cm` | 경계 단차 없음 |
| Actor Z | `0cm`, `20/20` | baked Z와 transform 이중 offset 방지 |
| V69 LOD0 triangles | `1,028,576` | V68 대비 `-2,144` (`-0.2080%`) |
| LongitudinalSpeed | `-0.14 cycle/s`, `20/20` | V67 흐름 유지 |
| Collision / Shadow / Nanite | `Off / Off / Off` | 드론 충돌과 불필요 렌더 비용 방지 |
| Runtime Actor / Tick / PCG delta | `0 / 0 / 0` | Editor-baked Static Mesh 유지 |

`DenseContactTiles`는 전 tile에 무조건 적용하지 않는다. 현재는 수치 감사에서 triangle 내부 관통이 확인된 `X04_Y04`만 대상으로 한다. 새로운 수로 또는 다른 tile에서 같은 증상이 수치로 확인되면 해당 tile을 명시적으로 추가하고, triangle 수를 늘리기 전에 local lift 방식으로 해결 가능한지 먼저 검사한다.

새 수로 추가에는 중심선만으로 임의의 고정 폭을 지정하지 않는다. 최소한 `CenterlineFeature`, `ValidatedWidthOrBankMask`, `FlowPhase`, `DenseContactTiles`, `RiparianInner/OuterDistance`, `ActiveTileSet`을 함께 확정해야 한다.

## 45. River Water Exclusion / PCG Mask Sync V70 고정값

| Parameter | Value | Purpose / Tradeoff |
|---|---:|---|
| Parent topology | V69 Cleanup | 승인된 수계·bridge·접지 구조 유지 |
| Filled hole tile | `X00_Y02` | 사용자 지정 중앙 hole만 수면화 |
| Filled hole signature | `47 cells`, bbox `[172,582,184,589]` | 다른 육지·섬의 우발적 제거 방지 |
| Approximate filled area | `42,300m²` | `30m×30m` source cell 기준 |
| Water cells | `32,143 → 32,190` | 정확히 47 cells 추가 |
| 4-neighbor / 8-neighbor components | `3 / 3` | 독립 수계 수 유지 |
| Remaining enclosed hole / diagonal-only contact | `0 / 0` | topology acceptance |
| River water mask | `6048×6048`, 약 `10m/pixel` | Lower Layer hard exclusion |
| Global water mask | `2017×2017`, 약 `30m/pixel` | Forest/기존 Riparian hard exclusion |
| Bank safety | `1 pixel ≈ 10m` | 수면 가장자리에서 grass 이격 |
| Near / Mid / Outer limit | `3 / 6 / 9 pixels` | 수면에서 약 `30/60/90m`까지 transition |
| Near / Mid / Outer density | `255 / 160 / 64` | 가까운 강변은 조밀, 바깥은 점진 감소 |
| Mid / Outer keep fraction | `0.70 / 0.30` | 전역 균일 띠와 instance 낭비 억제 |
| Bank water / safety overlap | `0 / 0 pixels` | 수면 및 safety band에 식생 후보 없음 |
| Rasterized high-water pixels | `307,645` | exact 10m audit authority |
| Bank candidate pixels | `234,076` | safety 제외 후 density band |
| V70 additions to global water | `858` coarse cells | 기존 global mask와 현재 River의 version drift 해소 |
| Regeneration targets | `26` | Lower Layer 20 + Forest 4 + Riparian 2 |
| Forest instances | `475,182` | 저장된 결정론적 결과 |
| Existing Riparian instances | `35,467` | 저장된 결정론적 결과 |
| Lower Layer instances | `1,623,247` | 저장된 결정론적 결과; ISM, grass collision/shadow off 유지 |
| Effective V70 water overlap | `0 / 0 / 0` | Forest / existing Riparian / Lower Layer |
| Coarse-only boundary candidates | `13` | exact 10m 검사상 0; 삭제하지 않는 false positive |
| River Actors / LOD0 triangles | `20 / 1,030,080` | 새 Actor/draw-call 단위 추가 없음 |
| Actor Z | `0cm`, `20/20` | baked grounding 유지 |
| Minimum contact gap | `8cm` | triangle center와 요청된 dense sample 통과 |
| LongitudinalSpeed | `-0.14 cycle/s`, `20/20` | V61 flow 유지 |
| Collision / Shadow | `Off / Off`, `20/20` | 드론 충돌과 불필요 렌더 비용 방지 |
| Mutation RHI | `RenderOffscreen` | 6048² PCG texture sampling 필요 |
| `NullRHI` policy | read-only audit only | mutation 시 Lower Layer가 0개로 생성될 수 있음 |
| Runtime Actor / Tick / PCG delta | `0 / 0 / 0` | Editor-baked production 유지 |

공유 graph 이름에 남은 `V40`/`V49`는 asset-reference 호환성을 위한 이름이다. texture sampler 입력은 V70이다. 재생성 스크립트는 위 세 계층 instance 총량이 정확히 복원되고 20개 Lower Layer Actor가 모두 non-empty일 때만 Level 저장을 허용한다.

신규 주황색 수로의 route 분리는 가능하지만 width tag가 없으므로 `ValidatedWidthOrBankMask`는 아직 확정되지 않았다. production 적용 전 신규 10개 tile의 폭, 연결, V61-compatible phase, dense contact, 양안 PCG 및 fixed-route frame profile을 별도 prototype에서 측정해야 한다.

## 46. Orange Route V71 — 사용자 승인 적용값

V70의 신규 수로 검토를 바탕으로 실제 승인 경로만 생성했다. 아래 값은 V71 저장본 기준이며 앞의 V70 항목은 복구/이력 기준이다.

| 항목 | V71 값 | 의미 |
|---|---:|---|
| River actors | 27 | 신규 7 + 기존 20 |
| Rebuilt existing junction tiles | 2 | X05_Y02, X07_Y04 |
| Unchanged existing river tiles | 18 | mesh/material/transform 유지 |
| Water source cells | 82,729 | V70 대비 50,539 추가, 삭제 0 |
| River LOD0 triangles | 2,647,328 | 접지 정확도를 유지한 실제 geometry 증가 |
| Minimum sampled terrain clearance | 8 cm | center/edge dense sample, 기존 border 214점 동기화 |
| Phase atlas | 737 × 1,024 | V61 shader/속도 -0.14 재사용 |
| Water/bank masks | 6,048 × 6,048 | 동일 수면 authority |
| Global water mask | 2,017 × 2,017 | 기존 제외 영역에 새 수면 union |
| Updated PCG cells/components | 9 / 27 | Forest, Riparian, LowerLayer |
| Added LowerLayer volumes | 7 | 나머지 기존 volume 재사용 |
| Forest / legacy Riparian / LowerLayer | 475,067 / 35,442 / 2,874,422 | 저장 및 fresh reload 확인 |
| Effective water overlap | 0 / 0 / 0 | 각 계층 실제 입력 mask 기준 |
| Grass end-cull | Dense 500 m / Accent 700 m | 기존 production 값 유지 |
| Grass WPO disable | 250 m | 근거리 바람만 평가 |
| Grass collision / shadow / ray tracing | Off / Off / Off | 새 9개 bank 구역에서 readback 확인 |
| New river collision / shadow / distance field | Off / Off / 0 | 신규 수면 부가 비용 억제 |

신규 bank의 seed는 기존 `4900 + Y*8 + X` 규칙이다. 밀도/기존 quality 설정은 임의로 낮추지 않았다. 전체 식생이 2,133,896 → 3,384,931개로 늘었으므로 추가가 무료라는 의미는 아니며, 실제 비행 경로 성능 검증은 별도다.

## 47. River Contact / Two Inlets V72

V72 현재 입력과 저장량이다. 위 V71 수치는 변경 전 비교용으로 유지한다.

| 항목 | V72 값 | 의미 |
|---|---:|---|
| Water source cells | 85,818 | +3,089, 기존 삭제 0 |
| 4/8-neighbor components | 2 / 2 | 새 고립 수역 없음 |
| XY 변경 River tiles | 3 | X05_Y02, X05_Y03, X06_Y02 |
| Ground-contact audit | 27 River tiles | Landscape 삼각 grid의 모든 critical intersections |
| Contact planning target | 8.1 cm | native 저장본에서 7.99 cm 이상을 검사 |
| Import residual local target | 8.5 cm | 기준 미달 triangle만, local lift <1 cm, 공유 edge 불변 |
| Landscape XY grid / collision MIP | 3000 cm / 0 | 현재 Landscape readback 확인 |
| River actors / total actors | 27 / 192 | V71과 동일 |
| River LOD0 triangles | 2,746,176 | +98,848 (+3.73%), 수역 증가분만 |
| Phase atlas | 737 × 1024 | 기존 water texel phase 보존 |
| LongitudinalSpeed | -0.14 | 기존 속도 유지 |
| PCG water/bank / global mask | 6048² / 2017² | 동일 V72 water authority |
| Bank candidate pixels | 401,670 | +11,312, water overlap 0 |
| Regenerated PCG cells/components | 4 / 12 | 기존 volume만 재사용 |
| Forest / legacy / LowerLayer instances | 475,062 / 35,442 / 2,964,789 | 총 3,475,293 |
| Target-exterior instance count | unchanged | 전역 밀도 감량 없음 |
| Grass end cull | 500 / 700 m | 기존 dense/accent descriptor 유지 |
| Grass collision / shadow | Off / Off | 영향 구역 실제 component 검사 |
| River collision / shadow / Nanite / distance field | Off / Off / Off / 0 | 기존 효율 정책 유지 |

추가 수역 source region polygon은 `river_v72_summary.json`에 고정한다. 구역 밖의 자잘한 지류까지 자동 확장하지 않는다. 주 경로 폭은 satellite 기반 시각 재구성이며 실제 수리학적 폭으로 해석하지 않는다. 실제 FPS/CPU/GPU/VRAM 예산은 별도 비행 경로 측정으로 판단해야 한다.

</details>
