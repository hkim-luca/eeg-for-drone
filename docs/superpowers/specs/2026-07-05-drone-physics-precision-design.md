# 드론 물리 정밀화 설계 (2026-07-05)

브랜치: `feature/drone-physics-precision`

## 목표

현재 `FDronePhysics`는 `CharacterMovementComponent`의 가감속 파라미터를 바꾸고 메시를
시각적으로만 기울이는 단순 효과다. 이를 수치해석급 6-DOF 강체 시뮬레이션으로 교체한다.

1. 모터·기체·양력·토크 등 실제 물리 효과를 고정 서브스텝 RK4 적분으로 계산
2. 사용자가 물리 파라미터를 조정하는 인게임 설정 UI (P 키 토글)
3. 대시보드에 현재 물리 설정 표시 (proto 확장)

## 채택한 접근: 자체 6-DOF 시뮬레이션 (Chaos 물리 미사용)

UE Chaos(SimulatePhysics + AddForce)는 솔버 타임스텝과 적분 방식을 제어할 수 없어
"수치해석급" 요구에 못 미친다. 자체 구현은 결정론적이고 서브스텝·적분기를 직접 통제한다.
기존 `FDronePhysics` 공개 API(Begin/Tick/End/GetCurrentTilt/IsSettled/IsActive)를 유지하는
facade로 재작성하여 EEG 러너·시나리오 러너·HUD(자세계기 포함)가 무변경으로 동작한다.

## 물리 모델 (SI 단위로 계산, UE cm 좌표로 변환)

상태 벡터 (17차원): 위치(3) + 속도(3) + 자세 쿼터니언(4) + 체좌표 각속도(3) + 모터 각속도(4)

### 모터 (4× X-쿼드 배치)
- 1차 지연 동역학: dω/dt = (ω_cmd − ω) / τ  (τ = MotorTimeConstant)
- 추력: T_i = kT · ρ/ρ₀ · ω_i²   (kT = ThrustCoefficient, ρ = 공기밀도)
- 반토크: Q_i = ±kQ · ρ/ρ₀ · ω_i²  (kQ = TorqueCoefficient, 회전 방향 부호)
- 포화: 0 ≤ ω ≤ MotorMaxSpeed

### 기체 (강체, Newton-Euler)
- 질량 m, 대각 관성텐서 (Ixx, Iyy, Izz), 암 길이 L (모터 위치 X자 대각)
- 병진: m·dv/dt = R·(0,0,ΣT_i) + m·g + F_drag + F_wind
- 회전: I·dΩ/dt = τ_thrust + τ_yaw + τ_gyro − Ω×(I·Ω)

### 추가 발굴 속성 (사용자 지시: 제시 항목 외 필요 속성 반영)
- **중력** g (조정 가능)
- **공기밀도** ρ — 추력·항력에 공통 반영 (고도·환경 변화 표현)
- **공력 항력** F_drag = −(c₁·v + c₂·|v|·v) — 선형(로터 유도 항력) + 이차(형상 항력)
- **로터 자이로스코픽 토크** τ_gyro = −Σ J_r · (Ω × ẑ) · ω_i (로터 관성 J_r)
- **지면 효과** — 지면 고도 h < 로터반경 배수일 때 추력 계수 증가:
  T·(1 / (1 − ρ_ge·(R_rotor/4h)²)), 강도 조절 가능
- **바람 외란** — 정상풍 벡터 + 1차 마르코프 돌풍(강도·시정수), 0으로 끄기 가능
- **최대 기울기 제한** — 제어기의 자세 목표 클램프

### 수치 적분
- 고정 서브스텝 RK4 (기본 1000 Hz, 250–4000 Hz 설정 가능), 프레임당 서브스텝 상한
- 매 스텝 쿼터니언 정규화; 상태는 double 정밀도

### 캐스케이드 제어기 (EEG 액션 → 모터 명령)
액션(전/후/좌/우/정지) → 수평 속도 목표 (MaxSpeed) + 고도 유지(Begin 시점 Z)
→ 속도 PID → 목표 가속도 → 목표 자세(roll/pitch, yaw 고정) + 총추력
→ 자세 P → 각속도 목표 → 각속도 PID → X-믹서 역변환 → 모터별 ω_cmd

### 액터 연동
- Begin: CharacterMovement를 MOVE_None으로 정지 (기존 파라미터 저장/복원 유지)
- Tick: 시뮬레이션 위치로 `SetActorLocation(sweep)`; 충돌 시 법선 방향 속도 소거(슬라이드)
- 자세: roll/pitch는 메시 상대회전으로 적용 (액터 루트는 yaw만 — 기존 HUD 규약 유지),
  `GetCurrentTilt()`는 시뮬레이션 자세를 반환
- 텔레메트리 속도: CharacterMovement Velocity에 시뮬레이션 속도를 미러링

## 파일 구성 (각 ≤ 200라인 지향)

| 파일 | 내용 |
|---|---|
| `Scenario/DronePhysicsSettings.h` | 확장 `FDronePhysicsSettings` USTRUCT (전 파라미터) |
| `Scenario/DroneFlightModel.h/.cpp` | 상태·힘/토크 계산·RK4 적분기 |
| `Scenario/DroneFlightController.h/.cpp` | 캐스케이드 PID + 믹서 |
| `Scenario/DronePhysics.h/.cpp` | facade 재작성 (API 유지 + SetMoveDirection) |
| `Scenario/DronePhysicsConfig.h/.cpp` | `UDronePhysicsConfig` (Config=Game, 단일 설정 소스, SaveConfig 지속화) |
| `Eeg/DronePhysicsSettingsWidget.h/.cpp` | 설정 UI — C++ WidgetTree로 전부 생성 (에디터 수작업 불필요) |

- 러너 컴포넌트의 개별 `PhysicsSettings` UPROPERTY는 제거하고 `UDronePhysicsConfig`를 참조
  (새 파라미터 집합과 호환되지 않는 구 필드이므로 정리가 맞다)
- `ApplyScenarioActionInput`은 액션→월드 방향 벡터 변환으로 바꾸고 두 러너가
  `Physics.SetMoveDirection()`에 전달

## 설정 UI

- `ADroneSimPlayerController::SetupInputComponent`에서 `BindKey(P)` (레거시 바인딩 — 에디터
  IMC 자산 불필요)
- 열림: UIOnly 입력 모드 + 커서 표시 / 닫힘: GameOnly 복귀
- 파라미터별 행: 라벨 + SpinBox (범위 클램프), 변경 즉시 활성 물리에 라이브 반영
- [저장] SaveConfig → Saved/Config 지속화, [기본값] 리셋
- 변경 시 EEG 러너가 서버로 설정 재전송

## 대시보드 연동 (proto 3-way 동기화)

`eeg_link.proto`에 추가, `eeg_link_pb2.py` 재생성, `EegProto.cpp` 인코더 수기 구현:

```proto
message PhysicsSettings {           // DroneSim -> server, 접속 시 + 변경 시
  double mass_kg = 1;
  double arm_length_m = 2;
  double inertia_xx = 3;  double inertia_yy = 4;  double inertia_zz = 5;
  double motor_time_constant_s = 6;
  double thrust_coefficient = 7;    // kT
  double torque_coefficient = 8;    // kQ
  double motor_max_rad_s = 9;
  double rotor_inertia = 10;        // J_r
  double rotor_radius_m = 11;
  double air_density = 12;
  double drag_linear = 13;  double drag_quadratic = 14;
  double gravity = 15;
  double ground_effect_strength = 16;
  double wind_x = 17; double wind_y = 18; double gust_intensity = 19;
  double max_tilt_deg = 20;
  double max_speed_ms = 21;
  int32 substep_hz = 22;
}
// ClientMessage oneof에 PhysicsSettings physics = 3; 추가
```

서버: `protocol.py` 디코드 → `state.py`에 dict 저장 → snapshot에 `physics_settings` 포함
→ `index.html`/`app.js`에 "드론 물리 설정" 패널 (파라미터 그리드, 값 변경 시 갱신).

## 검증

- 빌드: `Build.bat DroneSim Win64 Development` (게임 타깃 — Live Coding 회피)
- 수치 검증: 호버 평형(ΣT=mg에서 고도 유지), 정지 액션 시 감쇠 수렴, 스텝 응답 과도
  특성을 로그로 확인
- 실행 확인은 사용자에게 요청 (exe 직접 실행 금지 규칙)

## 단계별 커밋 (리뷰 단위 <1000라인)

1. 물리 코어 (Settings/Model/Controller) + facade 교체 + 러너 연동
2. 설정 UI + Config 지속화
3. proto 확장 + 서버/대시보드 패널
