# 드론 상태/미니맵 위젯 설계

## 목표

EEG 주행 모드(EEG running mode)에서 화면 좌측 하단에 드론 상태(ALT, SPD, HDG, ROLL, PITCH, YAW, 위경도/고도)를,
우측 하단에 비행 경로를 포함한 미니맵을 표시한다.

표시 예시 (좌하단 상태):

```
Drone state
ALT   127.3 m
SPD   12.4 m/s
HDG   342°
ROLL  -2.1°
PITCH 4.8°
YAW   342.0°
LAT   37°33'59.4"N
LON   126°58'40.8"E
ALT(MSL) 130.0 m
```

## 범위

- 표시 모드: **EEG 주행 모드에서만** 표시. 시나리오 기록/재생 모드에는 표시하지 않는다.
- 미니맵 배경: 단순 그리드/좌표축 (지형 텍스처나 SceneCapture 없음).
- 미니맵 방위: **North-up 고정** (지도는 항상 북쪽이 위, 드론 아이콘이 heading 방향으로 회전).
- 위경도 표기: **도분초(DMS)**.
- 궤적 보관: 최근 N개 샘플로 제한 (링버퍼), 전체 비행 기록 아님.

## 기존 아키텍처와의 정합성

프로젝트는 `ADroneSimPlayerController`에 `UEegRunnerComponent`(EEG 주행 로직), `UScenarioRunnerComponent`(시나리오 재생·위치 기록)를
나란히 두고, 각 기능마다 `UUserWidget` 파생 클래스 + Widget Blueprint 조합으로 HUD를 붙이는 패턴을 쓴다
(`EegHudWidget`/`WBP_EegHud`, `ScenarioHudWidget`/`WBP_ScenarioHud`). 커스텀 드로잉이 필요한 부분(그래프)은
`UWidget` 래퍼 안에 순수 Slate `SLeafWidget`(`SEegGraph`)을 두고 `OnPaint`로 직접 그린다
(`Source/DroneSim/Eeg/EegGraphPanel.h/.cpp`).

새 기능은 이 패턴을 그대로 따른다. `EegRunnerComponent`에는 현재 위치 기록 기능이 없으므로
(`Source/DroneSim/Eeg/EegRunnerComponent.h`), 텔레메트리 수집을 위한 컴포넌트를 새로 추가한다.

## 컴포넌트 구성

### 1. `UDroneTelemetryComponent` (신규)

- 위치: `Source/DroneSim/Telemetry/DroneTelemetryComponent.h/.cpp`
- `UActorComponent`, `ADroneSimPlayerController`에 `EegRunner`, `ScenarioRunner`와 나란히 추가
  (`ClassGroup = (Eeg)` 유사한 메타로 `BlueprintSpawnableComponent`)
- 매 틱, 조종 중인 폰(`GetControlledPawn()`)의 위치/회전/속도를 읽어 다음을 계산:
  - `AltitudeMeters` = `PawnLocation.Z / 100.0`
  - `SpeedMps` = `PawnVelocity.Size() / 100.0`
  - `HeadingDeg`(0~360 정규화), `RollDeg`, `PitchDeg`, `YawDeg` — `PawnRotation`에서 추출
  - `Latitude`, `Longitude` (도 단위 double) — origin 기준 flat-earth 변환
  - `AltitudeMslMeters` = `OriginAltitude + PawnLocation.Z / 100.0`
- lat/lon 변환 공식은 `ScenarioRunnerComponent::FlushSamples()`(ScenarioRunnerComponent.cpp:230-242)의
  flat-earth 공식과 동일하게 적용한다 (원본 코드는 CSV 기록용으로 그대로 두고, 동일 공식을 텔레메트리 쪽에도 적용):
  ```cpp
  constexpr double MetersPerDegreeLatitude = 111320.0;
  const double MetersPerDegreeLongitude = MetersPerDegreeLatitude * FMath::Cos(FMath::DegreesToRadians(OriginLatitude));
  const double EastMeters = PawnLocation.X / 100.0;
  const double NorthMeters = -PawnLocation.Y / 100.0;
  const double Latitude = OriginLatitude + NorthMeters / MetersPerDegreeLatitude;
  const double Longitude = OriginLongitude + EastMeters / MetersPerDegreeLongitude;
  ```
- `OriginLatitude`/`OriginLongitude`/`OriginAltitude` (EditAnywhere, double) — `ScenarioRunnerComponent`와
  동일 기본값(36.3504 / 127.3845 / 0.0, 대전)을 사용.
- 궤적(Trail) 샘플링: 시간 간격 기반. 기본 `TrailSampleInterval = 1.0`초, 최대 `MaxTrailPoints = 500`
  (둘 다 EditAnywhere로 조정 가능) → 기본값 기준 약 8분 20초 분량의 궤적 보관.
  `TArray<FVector2D>`에 East/North 미터 오프셋(origin 기준)을 저장, 500개 초과 시 가장 오래된 항목 제거(링버퍼).
- 폰이 없을 때(`GetControlledPawn() == nullptr`)는 마지막 값을 유지하고 갱신을 건너뛴다.
  `EegRunnerComponent::bWarnedNoPawn`과 동일한 패턴으로 경고 로그는 1회만 남긴다.
- Public API:
  - `GetAltitudeMeters/GetSpeedMps/GetHeadingDeg/GetRollDeg/GetPitchDeg/GetYawDeg`
  - `GetLatitude/GetLongitude/GetAltitudeMslMeters` (double, 도 단위 — DMS 변환은 위젯이 표시 시점에 수행)
  - `GetTrailPoints() -> TConstArrayView<FVector2D>` (East/North 미터, origin 기준)
  - `GetCurrentPositionMeters() -> FVector2D` (미니맵 중심/아이콘 위치용, 궤적의 마지막 포인트와 별개로 매 틱 갱신)

### 2. `UDroneStateWidget` (신규)

- 위치: `Source/DroneSim/Telemetry/DroneStateWidget.h/.cpp`
- `UUserWidget` 파생, `EegHudWidget::ConnectionText`와 동일한 패턴으로
  `BindWidgetOptional UTextBlock* StatusText` 하나에 여러 줄 텍스트를 포맷팅하여 매 틱 갱신.
- `NativeOnInitialized()`에서 소유 플레이어 폰/컨트롤러를 통해 `UDroneTelemetryComponent`를 캐시.
- `NativeTick()`에서 텔레메트리 값을 읽어 위 표시 예시 형식으로 포맷 (단위 표기 필수: m, m/s, °).
  위경도는 DMS로 변환해 표시 (예: `37°33'59.4"N`).
- 텔레메트리 컴포넌트를 찾지 못하면(폰 미소유 등) 마지막 표시값을 유지.

### 3. `UDroneMinimapWidget` + `SDroneMinimapPanel` (신규)

- 위치: `Source/DroneSim/Telemetry/DroneMinimapWidget.h/.cpp` (UWidget 래퍼),
  같은 파일에 `SDroneMinimapPanel : public SLeafWidget` 정의 — `UEegGraphPanel`/`SEegGraph` 패턴과 동일
  (`Source/DroneSim/Eeg/EegGraphPanel.h/.cpp` 참조).
- `RebuildWidget()`에서 `SNew(SDroneMinimapPanel)`로 Slate 위젯 생성, 텔레메트리 컴포넌트를 델리게이트/포인터로 연결.
- `OnPaint()`에서 `FSlateDrawElement`로 직접 그림:
  - 그리드/좌표축 (고정 간격, 예: 10m 단위 격자선)
  - 궤적 폴리라인 (`GetTrailPoints()`)
  - 현재 위치 아이콘 — heading 방향으로 회전하는 화살표, north-up 고정이므로 지도 자체는 회전하지 않음
  - 궤적의 bounding box + 여백으로 자동 스케일링, 최소 스케일(반경 10m)을 두어 정지 시 과도한 확대 방지
- 궤적이 비어있으면 그리드만 그리고 폴리라인/아이콘은 생략(또는 중심에 아이콘만 표시).

### 4. `ADroneSimPlayerController` 수정

- `Source/DroneSim/DroneSimPlayerController.h/.cpp`
- 추가 프로퍼티:
  - `TObjectPtr<UDroneTelemetryComponent> DroneTelemetry` (VisibleAnywhere)
  - `TSubclassOf<UDroneStateWidget> DroneStateWidgetClass` (EditAnywhere)
  - `TSubclassOf<UDroneMinimapWidget> DroneMinimapWidgetClass` (EditAnywhere)
  - `TObjectPtr<UDroneStateWidget> DroneStateWidget`, `TObjectPtr<UDroneMinimapWidget> DroneMinimapWidget` (UPROPERTY())
- `StartEegRunningMode()`에서 `EegHudWidget`과 함께 두 위젯을 생성하고 `AddToViewport()` (EegHudWidget과 같은 ZOrder 대역, 예: 12/13).
- EEG 주행 모드 종료/`ReturnToInitialScreen()` 시 다른 EEG 전용 위젯과 함께 정리(RemoveFromParent 또는 레벨 리로드에 위임 — 기존 EegHudWidget 처리 방식을 그대로 따름).

## 범위 밖 (Out of scope)

- WBP 에셋(`WBP_DroneStateHud`, `WBP_DroneMinimap`) 생성과 좌하단/우하단 anchor 배치는 **Unreal 에디터에서 사용자가 직접 작업**한다.
  기존 관례(`EegHudWidgetClass` 등)와 동일하게, C++ 클래스와 `TSubclassOf` 프로퍼티까지만 이 작업 범위이며
  실제 Widget Blueprint 디자인은 코드 작업 밖이다.
- DMS 변환 유틸리티는 이 기능 전용으로 최소 구현하며, 다른 곳에서 재사용할 공용 유틸로 일반화하지 않는다 (YAGNI).

## 테스트 방법

- 유닛 테스트보다 PIE(Play In Editor)에서 EEG 주행 모드 진입 후 육안 확인이 중심.
- lat/lon 변환 공식은 기존 시나리오 CSV 출력값(`lat,lon` 컬럼)과 동일 위치에서 대조해 검증 가능.
- 텔레메트리 컴포넌트의 궤적 링버퍼 동작(500개 초과 시 오래된 항목 제거)은 코드 리뷰로 확인.

## 미확정 사항 (기본값으로 진행, 사용자 확인 대기 중)

- 궤적 샘플링 주기: 기본 1.0초 간격, 최대 500개 (약 8분 20초 분량). 사용자가 자리를 비운 동안 추천값으로 확정하고 진행하되,
  스펙 리뷰 시 재확인이 필요하다.
