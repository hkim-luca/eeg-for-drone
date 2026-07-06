# 검은 배경 옵션(지형 숨김) 설계 — 2026-07-06

## 목적

고객 요청: 지형 배경을 지우고 **검은색 바탕에 드론과 위젯만** 보이도록 한다.
기본 동작은 기존과 동일하며, 이 기능은 **옵션**으로 제공한다.

## 요구사항

- 지형(랜드스케이프), 건물/구조물, 하늘, 구름, 안개 등 배경 요소를 모두 숨긴다.
- 드론(폰)과 UMG 오버레이(EEG HUD, 시나리오 HUD, 설정 패널)는 그대로 보인다.
- 드론이 계속 보이도록 조명은 유지한다(라이트 액터를 숨기면 조명 자체가 꺼짐).
- 물리/충돌은 변경하지 않는다 — 순수 렌더링 옵션.
- 옵션 제공 방식: 런타임 토글 키 `B` — 실행 중 켜고 끄기 (기존 `P` 키 물리
  설정 패턴과 동일). 실행 옵션(`-blackbg`)은 검토했으나 사용자 결정으로 제외.

## 검토한 접근

1. **런타임 액터 숨김 (채택)** — 월드의 모든 액터를 순회하며 폰(및 부착물)과
   라이트를 제외한 전부를 `SetActorHiddenInGame(true)`. 하늘이 사라지면 뷰포트는
   검은색으로 클리어된다. 클래스 목록을 나열하지 않는 역방향 규칙이라 BP 하늘
   구체 등 임의 배경 액터도 자동으로 잡힌다.
2. 배경 없는 별도 맵 복제 — 에셋 이중 관리와 패키지 크기 증가로 기각.
3. 숨길 클래스 열거(ALandscape, ASkyAtmosphere, …) — BP 배경 액터를 놓치고
   Landscape 모듈 의존성이 추가되어 기각.

## 구성

- **`FEnvironmentBlackout`** (`Source/DroneSim/EnvironmentBlackout.h/.cpp`, 신규)
  - `Apply(World, KeepVisiblePawn)` : 폰·부착물·라이트(ALight, ASkyLight)·이미
    숨겨진 액터를 제외한 전 액터를 숨기고 목록(TWeakObjectPtr)을 기록.
  - `Restore()` : Apply가 숨긴 액터만 되살림(레벨에서 원래 숨겨져 있던 액터 보존).
  - `IsActive()` : 현재 상태.
- **`ADroneSimPlayerController`** (수정)
  - `SetupInputComponent` : `B` 키 레거시 바인딩 → `ToggleEnvironmentBlackout()`.
  - 멤버 `FEnvironmentBlackout EnvironmentBlackout`.

## 오류 처리 / 엣지 케이스

- Restore 시 파괴된 액터는 weak pointer로 건너뜀.
- 시나리오 종료 후 레벨 리로드 시 상태는 초기화됨(기본 배경으로 시작);
  필요하면 B 키로 다시 켠다.

## 테스트

- 기존 테스트는 순수 물리 로직 대상이라 월드 렌더링 토글은 단위 테스트 범위 밖.
- 검증: 게임 타깃 컴파일 + 사용자 실행 확인(B 키 토글, 위젯 표시, 드론 조명
  유지).
