# Daejeon native level-streaming trial — 2026-09-05

## 범위와 원본 보존

- 시험 맵: `/Game/Environment/StreamingTrial/Daejeon_Streaming_Trial`
- 원본: `/Game/Maps/Daejeon_PCG_Work` — 수정하거나 덮어쓰지 않았다.
- 추가 프로젝트 콘텐츠는 `Content/Environment/StreamingTrial/` 아래 65개 맵 파일(지속 레벨 1 + 구역 레벨 64), 합계 487,019,418 bytes이다.
- 기존 PCG Graph, mask, vegetation mesh/material, river mesh/material, Config, Source, Plugins, .uproject는 변경하지 않았다. 기존 PCG를 재생성하지 않았다.
- 이전 performance-trial 백업을 유지했고, 작업 직전의 현재 맵도 별도로 복사하여 해시를 대조했다.
- 현재 맵 백업: `C:/Users/kdyde/Documents/Codex/2026-08-08/ue-5-7-4-pcg-agents-2/work/streaming_trial_20260905/Daejeon_PCG_Work_before_streaming.umap`

## 구현

기존 8×8, 한 변 7.56 km인 타일별로 Forest / Riparian / RiparianLowerLayer PCG Volume과 해당 river StaticMeshActor를 같은 서브레벨에 배치했다. 지형, 조명, 공통 액터는 지속 레벨에 남겼다.

- 네이티브 `LevelStreamingDynamic` + `LevelStreamingVolume`을 사용한다.
- 각 타일의 로딩 영역은 타일 경계에서 XY 각 방향으로 6 km 확장한다. 중심 기준 반폭은 9.78 km이다. 원형 반경 방식은 아니다.
- 기존 나무의 5 km 표시 거리를 유지하고 1 km의 선행 로딩 여유를 둔다.
- 로딩 기준은 플레이어의 시점이다. 높은 카메라 때문에 모두 내려가지 않도록 수직 범위는 ±100 km로 잡았다.
- `initially_loaded=false`, `initially_visible=false`, `SVB_LoadingAndVisibility`, unload 요청 cooldown 5초를 사용한다. cooldown은 실제 RAM 반환 시간을 보장하는 타이머가 아니다.
- 구역 로딩 시 기존의 저장된 인스턴스를 사용하며, PCG를 다시 생성하지 않는다.
- 시험 맵에만 네이티브 `GameModeBase`, 자동 소유되는 `SpectatorPawn`, `PlayerStart`를 설정했다. 기존 드론 GameMode/입력/게임 로직은 변경하지 않았다.
- 새 C++, Blueprint, 플러그인, Runtime Python 의존성은 없다. Python은 제작 및 자동 검증에만 사용했다.

Unreal 기본 MoveActorsToLevel은 텍스트 복사/붙여넣기를 사용한다. 큰 PCG Volume의 역스케일 값이 반올림되어 자식 ISM의 월드 스케일이 1.0017로 변하는 것을 사전 시험에서 발견했다. 원래 Actor/Component Transform, PerInstanceSMData 및 custom data를 메모리에 정확히 보관하여 네이티브 이동 후 되돌리고, 모든 월드 인스턴스 Transform의 SHA-256이 일치할 때만 저장했다.

## 검증 결과

- 새 프로세스에서 전체 시험 맵을 다시 읽고 원본 192개 액터의 스냅샷을 대조했다.
- 3,475,293개 인스턴스의 위치/회전/스케일 해시, 개수, 메시·머티리얼 참조, culling/WPO/충돌/그림자 정책이 원본과 일치했다.
- 27개 강 액터의 메시, LOD, 삼각형 수, 위치, 머티리얼 참조가 일치했다.
- 64개 스트리밍 레벨에 정확히 1개씩의 유효한 로딩 Volume이 연결되어 있는 것을 확인했다.
- **PIE가 아닌 별도 `-game` 실행**에서 A → B → A 왕복 검사를 수행했다. 각 이동 후 35초 이상 대기하고, 로딩 상태가 10초 이상 안정된 후 측정했다. 강제 GC는 사용하지 않았다.
- A의 6개 구역은 B에서 모두 unloaded 상태가 됐고, B의 9개 구역으로 교체됐다. A로 돌아오자 같은 599,943개 인스턴스가 같은 Transform으로 다시 로드됐다.
- 두 지점과 복귀 지점의 화면을 캡처했다. 이는 모든 타일을 근접해서 육안 검사했다는 뜻은 아니다.
- 원본/작업 콘텐츠 133개 SHA-256 일치, 보호 파일 5,275개 크기·수정 시각 일치. Git staging/commit은 하지 않았다.

## 메모리 비교

UE 5.7.4, Windows, 같은 PC, 각각 새 standalone 프로세스, 1280×720 offscreen, 동일 경로에서 비교했다. Shadow/GI/Reflection/PostProcess=High(2), Foliage=Epic(3), `t.MaxFPS=0`으로 동일하다. 같은 자동 관측 도구를 양쪽에 적용했다. 비교용 원본 맵은 저장하지 않고 실행 URL에서만 네이티브 GameMode를 지정했다.

| 위치 | 기존 인스턴스 | 로드된 시험 인스턴스 | 시험 로드 구역 | 기존 Working Set | 시험 Working Set | 감소 |
|---|---:|---:|---:|---:|---:|---:|
| A | 3,475,293 | 599,943 | 6 / 64 | 6.506 GiB | 4.389 GiB | 32.54% |
| B | 3,475,293 | 681,132 | 9 / 64 | 6.484 GiB | 4.456 GiB | 31.28% |
| A 복귀 | 3,475,293 | 599,943 | 6 / 64 | 6.490 GiB | 4.377 GiB | 32.56% |

프로세스 Private Bytes도 약 11.83–11.86 GiB에서 7.65–7.88 GiB로 감소했다. **Private Bytes는 물리 RAM 상주량이 아니며 Working Set과 구별해야 한다.** 위 수치는 시스템 전체 메모리 사용률이나 VRAM 수치가 아니다. GiB는 bytes / 1024³이다. 한 번의 통제된 왕복 비교이며, 모든 PC/경로의 보장값이나 장시간 누수 검사의 결과는 아니다.

## 직접 시험하기

1. 작업을 저장하고 Unreal Editor 및 기존 게임 프로세스를 닫는다. 에디터와 게임을 동시에 켜면 OS 전체 RAM 비교가 오염된다.
2. 다음 로컬 PowerShell 스크립트를 실행한다.

   `C:/Users/kdyde/Documents/Codex/2026-08-08/ue-5-7-4-pcg-agents-2/work/Launch_Streaming_Trial.ps1`

3. 별도의 1600×900 게임 창에서 마우스로 시점을 돌리고 WASD로 이동한다. E/Space는 위, Q/C/Ctrl은 아래이다. 이동 속도는 300 m/s이며 시험 맵의 `STREAM_TrialCamera` Movement Component에서 바꿀 수 있다. Alt+F4로 종료한다.
4. 몇 km 이상 이동해 새 구역을 오간 뒤, 같은 위치로 돌아와 수치와 모습을 비교한다. 아주 멀리 순간이동하면 새 구역이 준비되기 전 잠시 비어 보일 수 있다.
5. FPS/프레임 시간 표시가 켜져 있다. GPU 점유율 %만으로 향상 여부를 판단하지 않는다.

실행 스크립트는 원본 맵/프로젝트 설정을 저장하지 않으며, 기존 High 시험과 동일한 네 가지 화질 항목만 실행 세션에 지정한다. Python 테스트 도구는 이 수동 실행에 필요하지 않다.

**일반 에디터 뷰포트 이동과 게임 실행은 다르다.** 이 시험의 자동 로딩·해제는 실행 월드에 적용된다. 일반 에디터에서 시험 맵을 열면 편집을 위해 서브레벨/자산을 메모리에 유지할 수 있다. 에디터의 streaming-volume previs는 가시성 미리보기이며 RAM 해제 기능으로 간주하면 안 된다. PIE 또한 에디터에 이미 존재하는 데이터 때문에 메모리 비교용으로 적절하지 않다. [Epic의 Level Streaming Volumes 설명](https://dev.epicgames.com/documentation/en-us/unreal-engine/level-streaming-volumes-reference-in-unreal-engine)을 참고하되, 실제 사용 API/동작은 설치된 UE 5.7.4 소스로 확인했다.

## 남은 한계와 되돌리기

- Landscape 전체 및 공통 메시·머티리얼·텍스처의 상당 부분은 계속 로드된다. 구역 밖의 모든 메모리가 0이 되는 구조는 아니다.
- 언로드 이후에도 allocator/렌더링 캐시가 메모리를 유지할 수 있다. 위치마다 OS 수치가 즉시 줄어들 필요는 없다.
- 새 구역의 컴포넌트 등록이 약 100 ms 수준으로 기록된 구간이 있다. 선행 로딩 여유를 두었지만 저사양 PC/빠른 이동에서 순간 끊김이 없다고 보장하지 않는다. 추후 더 작은 구역 분할/등록 비용을 검토할 수 있다.
- 먼 강도 서브레벨과 함께 내려간다. 고고도에서 전체 강망을 보는 용도에는 더 큰 로딩 범위 또는 검증된 원거리 대체 표현이 필요할 수 있다. 이번에는 강 LOD나 형태를 바꾸지 않았다.
- GPU 사용률·VRAM·프레임 시간 개선은 이번 메모리 검사의 확정 결과가 아니다. 프레임 제한이 없으면 GPU 점유율은 계속 높을 수 있다.
- 새로운 streaming code를 컴파일할 필요는 없었지만, packaged/cooked/offline 실행은 이번 시험에서 검증하지 않았다. 확인한 것은 uncooked standalone 동작이다.
- 공급된 Huckleberry Oak 자산의 기존 custom-version 호환 경고는 원본 비교 실행에도 존재하며, 보호 범위 밖 정리를 하지 않았다.
- 원상태로 작업하려면 기존 `/Game/Maps/Daejeon_PCG_Work`를 열면 된다. 기존 맵을 바꾸지 않았으므로 이번 스트리밍 시험을 취소하기 위해 백업을 덮어쓸 필요가 없다. 시험 폴더 삭제는 이번에 수행하지 않았다.

## 재현 자료

로컬 작업 폴더 `work/streaming_trial_20260905/`의 `finalize.json`, `native_streamed_reload.json`, `runtime_streamed.json`, `runtime_baseline.json`, `comparison.json`, `source_guard.json`에 검증 결과가 있다. `streaming_trial_run_qa.ps1`은 관측용 standalone 실행, `streaming_trial_compare.ps1`은 비교표, `streaming_trial_guard.ps1`은 원본 보호 확인을 재실행한다. 제작 스크립트를 이미 구성된 맵에 무작정 재실행하지 않는다.
