# EEG HUD: GraphPanel 전극 라벨 + ProbabilityBox 우측 정렬

## 배경

- `WBP_EegHud`의 C++ 부모(`UEegHudWidget`)는 이미 `ProbabilityBox`(VerticalBox) 기반으로
  리팩터링되었으나(커밋 `b770f47`), 블루프린트 에셋은 옛 `ProbabilityText`(TextBlock) 상태로
  남아있어 UMG 에디터에서 수동으로 맞춰야 한다. 이 부분은 코드 변경이 아니므로 별도 안내로 처리하고
  (에디터 작업), 본 spec은 아래 두 코드 변경 항목만 다룬다.
- `SEegGraph::OnPaint`(`Source\DroneSim\Eeg\EegGraphPanel.cpp`)는 32개 전극 신호를 인덱스로만
  그리며 전극 이름 라벨이 없다.
- 코드베이스 어디에도 전극 이름(국제 10-20/10-10 체계) 목록이 정의되어 있지 않다.

## 범위

1. GraphPanel에 좌측 고정 여백을 두고 채널당 1줄씩 전극 이름 라벨 표시.
2. ProbabilityBox 내 확률 라인(액션명 + `%04.1f%%`)을 우측 정렬로 변경.

## 1) GraphPanel 전극 라벨

### 전극명 정의 (`Source\DroneSim\Eeg\EegTypes.h`)

`EegConfig` 네임스페이스에 표준 32채널 actiCAP/EasyCap 몬타주(MNE `easycap-M1` 템플릿과 동일 순서)를
`ChannelCount` 순서와 나란히 추가한다:

```cpp
inline constexpr const TCHAR *ChannelNames[ChannelCount] = {
    TEXT("Fp1"), TEXT("Fp2"), TEXT("F7"), TEXT("F3"), TEXT("Fz"), TEXT("F4"), TEXT("F8"), TEXT("FC5"),
    TEXT("FC1"), TEXT("FC2"), TEXT("FC6"), TEXT("T7"), TEXT("C3"), TEXT("Cz"), TEXT("C4"), TEXT("T8"),
    TEXT("CP5"), TEXT("CP1"), TEXT("CP2"), TEXT("CP6"), TEXT("P7"), TEXT("P3"), TEXT("Pz"), TEXT("P4"),
    TEXT("P8"), TEXT("PO9"), TEXT("O1"), TEXT("Oz"), TEXT("O2"), TEXT("PO10"), TEXT("AF7"), TEXT("AF8"),
};
```

이 배열은 실제 하드웨어 몬타주와 정확히 일치하지 않을 수 있는 근사치이며, 시뮬레이션된 데모용 신호에
붙이는 표시 라벨이다. `GroupStartChannel`이 정의하는 액션 그룹 경계(Forward=0..5, Left=8..13,
Right=18..23, Backward=26..31)와 겹쳐 보여, 어떤 전극 그룹이 어떤 조작에 반응하는지 시각적으로
확인하기 쉬워지는 부수 효과가 있다.

### 렌더링 (`Source\DroneSim\Eeg\EegGraphPanel.cpp` / `.h`)

`UEegGraphPanel`에 `EditAnywhere` 프로퍼티 3개 추가:

- `LabelColumnWidth: float = 34.0f` — 좌측 라벨 여백 폭(px)
- `LabelColor: FLinearColor = FLinearColor(0.7, 0.7, 0.7, 1.0)` — 라벨 텍스트 색
- `LabelFontSize: int32 = 8` — 라벨 폰트 크기(pt)

`SEegGraph::SetStyle`에 위 세 값을 추가 인자로 받아 멤버로 저장한다.

`OnPaint` 변경:

- 파형을 그리는 X 범위를 `[LabelColumnWidth, Size.X]`로 좁힌다 (`StepX`는
  `(Size.X - LabelColumnWidth) / (PointCount - 1)`로, 각 포인트 X에 `LabelColumnWidth` 오프셋을
  더한다).
- 채널 루프 안에서, 파형을 그리기 전에 `FSlateDrawElement::MakeText`로 `EegConfig::ChannelNames[Channel]`을
  `CenterY` 위치(세로 중앙 정렬)에 좌측 여백(x=2px) 안에 그린다. 폰트는
  `FCoreStyle::GetDefaultFontStyle("Regular", LabelFontSize)`, 색은 `LabelColor`.

### 제약

- 채널당 스트립 높이가 기본 `PanelDesiredSize.Y=520 / 32 ≈ 16px`라 8pt 폰트가 실질적 하한이다.
  더 작아지면 겹침이 발생하므로 폰트 크기는 프로퍼티로 노출해 블루프린트에서 조정 가능하게 한다.
- 이 배열은 표준 몬타주 근사치임을 코드 주석으로 명시한다(실제 장비와 다를 수 있음).

## 2) ProbabilityBox 우측 정렬

`Source\DroneSim\Eeg\EegHudWidget.cpp`의 `NativeOnInitialized()`에서 각 라인을
`ProbabilityBox->AddChildToVerticalBox(Line)`로 추가할 때, 반환되는 `UVerticalBoxSlot*`의
`SetHorizontalAlignment(HAlign_Right)`를 호출한다. 또한 `Line->SetJustification(ETextJustify::Right)`를
함께 설정해 텍스트가 스트레치되는 경우에도 우측 정렬이 유지되도록 한다.

동작상 변화: 액션명 길이가 서로 다르므로(FORWARD/BACKWARD/LEFT/RIGHT/STOP) 좌측 정렬 시 `%` 값의
시작 위치가 줄마다 달라 보였다. 우측 정렬로 바꾸면 모든 줄의 우측 끝(`%` 값)이 정렬되어 더 읽기
쉬워진다.

## 영향 파일

- `Source\DroneSim\Eeg\EegTypes.h` — `ChannelNames` 배열 추가만 (기존 코드 영향 없음)
- `Source\DroneSim\Eeg\EegGraphPanel.h` — 프로퍼티 3개 추가
- `Source\DroneSim\Eeg\EegGraphPanel.cpp` — `SetStyle` 확장, `OnPaint`에 라벨 렌더링 추가
- `Source\DroneSim\Eeg\EegHudWidget.cpp` — `NativeOnInitialized()`에 슬롯 정렬 2줄 추가

총 변경량은 약 50~70줄로 CLAUDE.md의 200줄/파일, 1,000줄/리뷰 기준에 여유 있게 들어간다.

## 테스트

- 빌드 후 PIE(또는 Standalone)로 EEG Running Mode 진입.
- GraphPanel 좌측에 32개 전극명이 스트립마다 겹치지 않고 표시되는지 육안 확인.
- ProbabilityBox의 5줄이 우측 정렬되어 `%` 값이 세로로 정렬되는지 확인.
- 기존 확률 최고값 강조 색상(`b770f47`)이 계속 정상 동작하는지 회귀 확인.

## 범위 밖 (Out of scope)

- `WBP_EegHud.uasset`에서 `ProbabilityText` → `ProbabilityBox` 리네임 (UMG 에디터 수동 작업, 별도 안내로 처리)
- 실제 하드웨어 채널-전극 매핑 검증 (현재는 표준 몬타주 근사치)
