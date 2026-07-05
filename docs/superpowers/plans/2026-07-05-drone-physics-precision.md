# 드론 물리 정밀화 구현 계획

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 단순 CharacterMovement 튜닝을 6-DOF 강체 시뮬레이션(RK4, 모터/양력/토크/항력/자이로/지면효과/바람)으로 교체하고, 인게임 설정 UI와 대시보드 설정 패널을 추가한다.

**Architecture:** 순수 C++ 물리 코어(`DroneFlightModel`+`DroneFlightController`)를 기존 `FDronePhysics` facade 뒤에 넣어 러너/HUD API를 유지. 설정은 `UDronePhysicsConfig`(Config=Game) 단일 소스. proto `PhysicsSettings` 메시지로 대시보드에 전파.

**Tech Stack:** UE 5.7 C++23, UE AutomationTest, proto3 수기 코덱(EegProto), Python asyncio 서버 + vanilla JS 대시보드.

## Global Constraints

- 소스 파일 200라인 지향, 커밋(리뷰 단위) < 1000라인
- 빌드 검증: `& "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" DroneSim Win64 Development -Project="C:\Users\hksky\sources\eeg-for-drone\DroneSim\DroneSim.uproject" -WaitMutex` (에디터 타깃은 Live Coding으로 차단됨)
- DroneSim.exe를 직접 실행하지 않는다 — 실행 확인은 사용자에게 요청
- 편집한 cpp/hpp에 clang-format(-style=microsoft) 적용; clang-tidy --fix-errors는 컴파일 DB가 없어 코드 손상 이력 있음 → 적용 시 diff 검토 필수
- proto 변경은 3곳 동기화: `eeg-server/proto/eeg_link.proto`, `eeg_server/eeg_link_pb2.py`(재생성), `DroneSim/Source/DroneSim/Eeg/EegProto.cpp`(수기)
- 물리는 SI 단위(m, kg, s, rad)로 계산하고 UE 경계에서만 cm 변환 (1 m = 100 cm)
- 모든 새 Python 코드에 type hint (`from __future__ import annotations`)

---

### Task 1: 설정 구조체 + Config 단일 소스

**Files:**
- Create: `DroneSim/Source/DroneSim/Scenario/DronePhysicsSettings.h`
- Create: `DroneSim/Source/DroneSim/Scenario/DronePhysicsConfig.h`
- Create: `DroneSim/Source/DroneSim/Scenario/DronePhysicsConfig.cpp`
- Delete-content: `DronePhysics.h`의 기존 `FDronePhysicsSettings` (Task 4에서 facade 재작성과 함께 제거)

**Interfaces:**
- Produces: `FDronePhysicsSettings` (아래 전체 필드), `UDronePhysicsConfig::Get() -> UDronePhysicsConfig*` (GetMutableDefault), `UPROPERTY(Config) FDronePhysicsSettings Settings`, `void Save()` (SaveConfig 래퍼)

- [ ] **Step 1: `DronePhysicsSettings.h` 작성** — USTRUCT(BlueprintType), 전 필드 UPROPERTY(EditAnywhere, ClampMin/Max), 카테고리 Airframe/Motor/Environment/Control/Settle:

```cpp
// Airframe
double MassKg = 1.2;            // 0.1..10
double ArmLengthM = 0.18;       // 0.05..1
double InertiaXX = 0.012, InertiaYY = 0.012, InertiaZZ = 0.022; // kg*m^2, 1e-4..1
// Motor/Rotor (x4, X-quad)
double MotorTimeConstantS = 0.05;   // 0.005..0.5
double ThrustCoefficient = 1.2e-6;  // N/(rad/s)^2 @ rho0
double TorqueCoefficient = 1.9e-8;  // N*m/(rad/s)^2
double MotorMaxRadS = 2100.0;       // 100..10000
double RotorInertiaKgM2 = 6e-5;
double RotorRadiusM = 0.12;
// Environment
double GravityMS2 = 9.80665;
double AirDensity = 1.225;          // kg/m^3, 0..2
double DragLinear = 0.25;           // N/(m/s)
double DragQuadratic = 0.10;        // N/(m/s)^2
double GroundEffectStrength = 0.5;  // 0=off..1
double WindXMS = 0.0, WindYMS = 0.0;  // 정상풍 (UE X/Y축, m/s)
double GustIntensityMS = 0.0;       // 돌풍 표준편차
// Control
double MaxSpeedMS = 15.0;           // Start(MoveSpeed>0)가 덮어씀 (cm/s -> m/s)
double MaxClimbRateMS = 3.0;
double MaxTiltDeg = 25.0;
double TakeoffAltitudeM = 1.5;      // Begin Z 기준 호버 목표 고도
double VelPGain = 2.5, VelIGain = 0.4;
double AttPGain = 8.0;              // 자세각 오차 -> 각속도 목표 [1/s]
double RatePGain = 20.0, RateDGain = 0.4; // 각속도 오차 -> 각가속 [1/s]
int32 SubstepHz = 1000;             // 250..4000
// Settle (기존 의미 유지)
float SettleSpeedThreshold = 10.0f; // cm/s
float SettleTiltThreshold = 0.5f;   // deg
```

- [ ] **Step 2: `UDronePhysicsConfig` 작성** — `UCLASS(Config = Game, DefaultConfig 아님)`; `static UDronePhysicsConfig *Get()` = `GetMutableDefault<UDronePhysicsConfig>()`; `void Save()` = `SaveConfig(CPF_Config, *GGameUserSettingsIni)` 유사가 아닌 기본 `SaveConfig()`; `void ResetToDefaults()` = 새 `FDronePhysicsSettings{}` 대입.
- [ ] **Step 3: 게임 타깃 빌드로 컴파일 확인** (아직 사용처 없음 — 헤더 오류만 검증)
- [ ] **Step 4: Commit** `feat: 드론 물리 정밀화 설정 구조체와 Config 단일 소스 추가`

### Task 2: 6-DOF 비행 모델 (RK4)

**Files:**
- Create: `DroneSim/Source/DroneSim/Scenario/DroneFlightModel.h`
- Create: `DroneSim/Source/DroneSim/Scenario/DroneFlightModel.cpp`
- Create: `DroneSim/Source/DroneSim/Scenario/Tests/DroneFlightModelTest.cpp`

**Interfaces:**
- Consumes: `FDronePhysicsSettings`
- Produces:

```cpp
struct FDroneFlightState {           // SI, UE 축 (X전방, Y우, Z상)
    FVector Position = FVector::ZeroVector;   // m
    FVector Velocity = FVector::ZeroVector;   // m/s (월드)
    FQuat Attitude = FQuat::Identity;          // 체->월드
    FVector AngularVelocity = FVector::ZeroVector; // rad/s (체좌표)
    double MotorSpeed[4] = {};                 // rad/s
};
class FDroneFlightModel {
  public:
    void Reset(const FVector &PositionM, double YawRad);
    void SetSettings(const FDronePhysicsSettings &InSettings);
    /** MotorCommands[4]: 목표 모터 각속도(rad/s, 포화 전) */
    void Advance(double DeltaTimeS, const double MotorCommands[4], double GroundAltitudeM);
    auto GetState() const -> const FDroneFlightState &;
    auto HoverMotorSpeed() const -> double;    // sqrt(m*g / (4*kT*rho/rho0))
};
```

- [ ] **Step 1: 모델 구현** — 핵심 수식 (`Derivative(State, MotorCommands, GroundAlt)`):
  - 모터: `dW[i] = (clamp(Cmd[i],0,MotorMaxRadS) - W[i]) / Tau`
  - 로터 회전방향 부호 `Dir[] = {+1,-1,-1,+1}` (FR,BL은 CCW=+1; FL,BR은 CW=-1)
  - 밀도비 `RhoRatio = AirDensity / 1.225`
  - 지면효과: `Ge = 1 / (1 - GroundEffectStrength * min(0.25, (RotorRadiusM/(4*max(h,RotorRadiusM)))^2))` (h = 로터-지면 거리)
  - 추력(체 z): `T = Σ kT * RhoRatio * Ge * W[i]^2`
  - 토크(체): X-쿼드 모터 위치 `P0=(+L/√2,+L/√2) FR, P1=(-L/√2,-L/√2) BL, P2=(+L/√2,-L/√2) FL, P3=(-L/√2,+L/√2) BR`;
    `τx(roll) = Σ Py_i * T_i`? — 아니: `τ = Σ (P_i × (0,0,T_i))` + `τz = Σ -Dir[i]*kQ*RhoRatio*W[i]^2`
  - 자이로: `τ_gyro = -Σ RotorInertia * Dir[i] * W[i] * (Ω × ẑ)`
  - 병진: `a = Attitude.RotateVector(FVector(0,0,T))/m + (0,0,-g) + (F_drag + F_wind_drag)/m`,
    항력은 대기속도 `Va = V - Wind` 기준 `F = -(c1*Va + c2*|Va|*Va)`
  - 회전: `dΩ = I^-1 * (τ - Ω × (I*Ω))`
  - 자세: `dQ = 0.5 * Q * FQuat(Ωx,Ωy,Ωz,0)`
  - RK4: 상태 17차원 전체를 k1..k4로 적분, 스텝 후 `Attitude.Normalize()`
  - 돌풍: 1차 마르코프 `Gust += (-Gust/2.0 + N(0,GustIntensityMS)*sqrt(2/2.0)) * dt` (시정수 2 s, FRandomStream) — Advance에서 스텝별 갱신, Wind에 합산
- [ ] **Step 2: 자동화 테스트 작성** (`IMPLEMENT_SIMPLE_AUTOMATION_TEST`, `DroneSim.Physics.FlightModel.*`):
  - Hover: 4모터에 `HoverMotorSpeed()` 명령, 지면효과/바람 0, 5 s 적분 → |ΔZ| < 0.01 m, |V| < 0.01 m/s
  - ThrustAsymmetry: 좌측 2모터 +5% → roll 각속도 부호 확인
  - QuatNorm: 10 s 적분 후 |1-|Q|| < 1e-9
  - EnergyDecay: 모터 0, 초기 수평속도 5 m/s → 속도 단조 감소 (항력)
- [ ] **Step 3: 게임 타깃 빌드 확인** (테스트 실행은 Task 8에서 사용자/에디터로)
- [ ] **Step 4: Commit** `feat: 6-DOF 드론 비행 모델(RK4, 모터/양력/토크/항력/자이로/지면효과/바람) 추가`

### Task 3: 캐스케이드 비행 제어기

**Files:**
- Create: `DroneSim/Source/DroneSim/Scenario/DroneFlightController.h`
- Create: `DroneSim/Source/DroneSim/Scenario/DroneFlightController.cpp`
- Test: `DroneSim/Source/DroneSim/Scenario/Tests/DroneFlightControllerTest.cpp`

**Interfaces:**
- Consumes: `FDroneFlightState`, `FDronePhysicsSettings`
- Produces:

```cpp
class FDroneFlightController {
  public:
    void Reset(const FDronePhysicsSettings &InSettings, double HoldAltitudeM, double HoldYawRad);
    /** MoveDirection: 월드 XY 단위벡터(무이동 = Zero); OutMotorCommands[4] = rad/s */
    void Compute(const FDroneFlightState &State, const FVector &MoveDirection, double DeltaTimeS,
                 double OutMotorCommands[4]);
};
```

- [ ] **Step 1: 구현** —
  - 속도 목표: `Vdes = MoveDirection * MaxSpeedMS`; `Vzdes = clamp(1.0*(AltDes - Z), ±MaxClimbRateMS)`
  - 가속 목표: `Acc = VelP*(Vdes-V) + VelI*∫` (적분 ±2 m/s² 클램프), 수평 성분 `tan(MaxTiltDeg)*g` 클램프
  - 총추력: `T = m*(g + AccZ) / (cosRoll*cosPitch)` (기울기 보상, `cos` 하한 0.5)
  - 목표 자세: `pitch_des = atan2(-AccX_body?, ...)` — 월드 가속을 yaw 프레임으로 회전 후
    `pitch = atan(ax/g)`(전방 가속 = 기수 하강 -pitch), `roll = atan(ay/g)`; yaw = HoldYaw 고정
  - 자세 P: 오차각(체좌표 소회전 벡터) × AttP → Ω_des; 각속도 PID: `α = RateP*(Ω_des-Ω) - RateD*dΩ/dt`; `τ = I*α`
  - 믹서 역변환 (모터 배치 Task 2와 동일):
    `T_i = T/4 + [±]τx/(4*Ly) + [±]τy/(4*Lx) + [-Dir_i]*τz/(4*kQ/kT)`; `W_cmd_i = sqrt(max(T_i,0)/(kT*RhoRatio))`
- [ ] **Step 2: 테스트** — HoverCommand: 평형 상태 입력 시 4모터 명령 ≈ HoverMotorSpeed ±2%; TiltClamp: 극단 속도 오차에도 목표 기울기 ≤ MaxTiltDeg
- [ ] **Step 3: 빌드 확인 후 Commit** `feat: 캐스케이드 PID 비행 제어기(속도-자세-각속도-믹서) 추가`

### Task 4: FDronePhysics facade 재작성 + 러너 연동

**Files:**
- Modify: `DroneSim/Source/DroneSim/Scenario/DronePhysics.h` (구 FDronePhysicsSettings 제거, 새 facade)
- Modify: `DroneSim/Source/DroneSim/Scenario/DronePhysics.cpp` (전면 재작성)
- Modify: `DroneSim/Source/DroneSim/Scenario/ScenarioActionInput.h` — `ScenarioActionDirection(Action, YawRotation) -> FVector` 로 대체
- Modify: `DroneSim/Source/DroneSim/Eeg/EegRunnerComponent.h/.cpp` — `PhysicsSettings` UPROPERTY 제거, `UDronePhysicsConfig::Get()->Settings` 사용, `ApplyScenarioActionInput` 대신 `Physics.SetMoveDirection(ScenarioActionDirection(...))`
- Modify: `DroneSim/Source/DroneSim/Scenario/ScenarioRunnerComponent.h/.cpp` — 동일 치환

**Interfaces:**
- Consumes: Task 1–3 전부
- Produces (기존 유지 + 추가):

```cpp
void Begin(ACharacter &InCharacter, float MoveSpeed, const FDronePhysicsSettings &InSettings); // 시그니처 유지
void SetMoveDirection(const FVector &WorldDirection);   // 매 틱 러너가 설정
void UpdateSettings(const FDronePhysicsSettings &InSettings); // 설정 UI 라이브 반영
void Tick(float DeltaTime); void End();
auto GetCurrentTilt() const -> FRotator;  // 시뮬레이션 roll/pitch
auto IsSettled() const -> bool; auto IsActive() const -> bool;
auto GetStateForTelemetry() const -> const FDroneFlightState &;
```

- [ ] **Step 1: facade 구현** —
  - Begin: 무브먼트 저장 후 `SetMovementMode(MOVE_None)`; `MoveSpeed>0`이면 `Settings.MaxSpeedMS = MoveSpeed/100`; 라인트레이스로 지면 높이 → `FlightModel.Reset(ActorLoc/100, ControlYaw)`, `Controller.Reset(...)`
  - Tick: DeltaTime 누적 → `SubstepHz` 고정 스텝 루프(상한 200스텝): `Controller.Compute` → `Model.Advance`;
    지면고도는 틱당 1회 라인트레이스(ECC_Visibility, 아래로 50 m)
  - 위치 반영: `SetActorLocation(NewLoc*100, /*bSweep=*/true, &Hit)`; blocked 시 모델 속도에서 법선 성분 제거·위치를 실제 액터 위치로 되돌림
  - CharacterMovement `Velocity`에 시뮬 속도(cm/s) 미러링 (텔레메트리/애님 호환)
  - 메시: 시뮬 자세에서 yaw 제거한 roll/pitch를 `MeshBaseRotation` 위에 합성 (기존 방식), `CurrentTilt` 갱신
  - End: MOVE_Walking 복귀 + 저장값 복원 (기존 로직 유지)
- [ ] **Step 2: 러너 2곳 치환 + 빌드** — 기대: 컴파일 성공, 기존 헤더 참조(`EegRunnerComponent.h`의 `FDronePhysicsSettings PhysicsSettings`) 제거 반영
- [ ] **Step 3: clang-format 적용, Commit** `feat: 드론 물리를 6-DOF 시뮬레이션 facade로 교체하고 러너를 연동`

### Task 5: 물리 설정 UI (P 키)

**Files:**
- Create: `DroneSim/Source/DroneSim/Eeg/DronePhysicsSettingsWidget.h`
- Create: `DroneSim/Source/DroneSim/Eeg/DronePhysicsSettingsWidget.cpp` (200라인 초과 시 Rows 빌더 분리: `DronePhysicsSettingsRows.cpp`)
- Modify: `DroneSim/Source/DroneSim/DroneSimPlayerController.h/.cpp` — P 키 토글, 위젯 생성(C++ 클래스 직접 `CreateWidget<UDronePhysicsSettingsWidget>(this, UDronePhysicsSettingsWidget::StaticClass())`), 입력 모드 전환
- Modify: `DroneSim/Source/DroneSim/Eeg/EegRunnerComponent.h/.cpp` — `NotifySettingsChanged()` (라이브 반영 + Task 6에서 서버 전송 추가)

**Interfaces:**
- Consumes: `UDronePhysicsConfig`, `FDronePhysics::UpdateSettings`
- Produces: `UDronePhysicsSettingsWidget` — `DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPhysicsSettingsChanged)` `OnSettingsChanged`; `RebuildFromConfig()`

- [ ] **Step 1: 위젯 구현** — `UUserWidget` 파생, WBP 불필요: `NativeConstruct`에서 `WidgetTree->ConstructWidget<UBorder>` 루트 + `UScrollBox` + 파라미터별 `UHorizontalBox{UTextBlock 라벨, USpinBox}`; 행 정의는 `{라벨, double* 값 포인터, Min, Max}` 테이블 기반; SpinBox `OnValueChanged` → Config 값 갱신 + `OnSettingsChanged` 브로드캐스트; 하단 버튼 [저장]=`Config->Save()`, [기본값]=`ResetToDefaults()+RebuildFromConfig()`, [닫기]. 스타일은 OSD 팔레트(어두운 반투명 배경, soft white 텍스트).
- [ ] **Step 2: 컨트롤러 연동** — `SetupInputComponent`에 `InputComponent->BindKey(EKeys::P, IE_Pressed, ...)`; 토글 시 `FInputModeUIOnly`+커서 표시 ↔ `FInputModeGameOnly`; `OnSettingsChanged` → `EegRunner->NotifySettingsChanged()` (활성 러너의 `Physics.UpdateSettings` + ScenarioRunner에도 동일)
- [ ] **Step 3: 빌드 + clang-format, Commit** `feat: 인게임 드론 물리 설정 UI(P 키, 라이브 반영, Config 저장) 추가`

### Task 6: proto 확장 + UE 인코더 + 전송

**Files:**
- Modify: `eeg-server/proto/eeg_link.proto` — `message PhysicsSettings` (spec의 22필드) + `ClientMessage.oneof msg { ... PhysicsSettings physics = 3; }`
- Regenerate: `eeg-server/eeg_server/eeg_link_pb2.py` — `eeg-server/.venv`에서 `python -m grpc_tools.protoc -I proto --python_out=eeg_server proto/eeg_link.proto` (grpc_tools 없으면 `pip install grpcio-tools`)
- Modify: `DroneSim/Source/DroneSim/Eeg/EegProto.h/.cpp` — `EncodePhysicsSettings(const FDronePhysicsSettings &) -> TArray<uint8>` (기존 EncodeEegFrame 패턴, double=wire type 1 fixed64 LE)
- Modify: `DroneSim/Source/DroneSim/Eeg/EegRunnerComponent.cpp` — 연결 성립 전이 시(`!bWasConnected -> true`) + `NotifySettingsChanged()` 시 전송

**Interfaces:**
- Consumes: `FDronePhysicsSettings`
- Produces: proto `PhysicsSettings` 필드 번호 1..22 (spec 문서 그대로), Python측 `ClientMessage.physics`

- [ ] **Step 1: proto 수정 + pb2 재생성** — 검증: `python -c "from eeg_server import eeg_link_pb2; m=eeg_link_pb2.ClientMessage(); m.physics.mass_kg=1.2; print(len(m.SerializeToString()))"` 출력 > 0
- [ ] **Step 2: EegProto 인코더 + 왕복 검증** — Python으로 UE 인코딩 바이트를 파싱하는 스크립트(scratchpad)로 필드값 일치 확인은 Task 7 서버 디코드 후 통합 확인으로 대체
- [ ] **Step 3: 러너 전송 연동 + 빌드, Commit** `feat: 물리 설정 proto 메시지 추가 및 DroneSim 접속/변경 시 전송`

### Task 7: 서버 수신 + 대시보드 패널

**Files:**
- Modify: `eeg-server/eeg_server/protocol.py` — physics oneof 분기
- Modify: `eeg-server/eeg_server/tcp_server.py` — `state.on_physics_settings(dict)` 호출
- Modify: `eeg-server/eeg_server/state.py` — `_physics_settings: dict[str, float]` 저장, snapshot에 `physics_settings` 포함
- Modify: `eeg-server/eeg_server/static/index.html` + `app.js` + `style.css` — "드론 물리 설정" 패널(파라미터 그리드: 한글 라벨+값+단위, 미수신 시 "—")

**Interfaces:**
- Consumes: Task 6 proto
- Produces: snapshot 키 `physics_settings: dict[str, float]` (proto 필드명 그대로)

- [ ] **Step 1: 서버 파이프라인 구현** — 단위 확인: `python -m eeg_server` 기동 후 `curl http://127.0.0.1:8800/api/state` 에 `physics_settings` 키 존재
- [ ] **Step 2: 대시보드 UI** — dataviz 스킬 참조, 기존 KPI 타일/패널 스타일 재사용; 22개 값을 기체/모터/환경/제어 4그룹 그리드로 표시
- [ ] **Step 3: Commit** `feat: 대시보드에 드론 물리 설정 패널 추가`

### Task 8: 통합 검증 + 마무리

- [ ] **Step 1: 전체 빌드** (게임 타깃) — 성공 확인
- [ ] **Step 2: 편집 파일 clang-format 일괄 적용, diff 검토**
- [ ] **Step 3: 사용자 실행 요청** — DroneSim 실행 + eeg-server 기동 후: 이륙(1.5 m 호버), P 키 설정 UI, 파라미터 변경 반응(예: 질량↑ → 처짐 후 회복), 대시보드 설정 패널 갱신 확인
- [ ] **Step 4: 자동화 테스트 실행 안내** — 에디터 Session Frontend > Automation에서 `DroneSim.Physics.*` 실행 요청
