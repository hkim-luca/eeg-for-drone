# EEG HUD: GraphPanel 전극 라벨 + ProbabilityBox 우측 정렬 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** GraphPanel(EEG 32채널 파형 위젯)에 전극 이름 라벨을 표시하고, ProbabilityBox의 확률 텍스트 라인을 우측 정렬로 바꾼다.

**Architecture:** `EegConfig`(EegTypes.h)에 채널-전극명 매핑 상수 배열을 추가한다. `SEegGraph::OnPaint`(EegGraphPanel.cpp)는 좌측 고정 폭 여백을 확보해 채널 스트립마다 `FSlateDrawElement::MakeText`로 라벨을 그리고, 파형은 남은 폭에만 그리도록 좌표 계산을 조정한다. `UEegHudWidget::NativeOnInitialized()`(EegHudWidget.cpp)는 확률 텍스트 라인을 VerticalBox에 추가할 때 슬롯을 우측 정렬로 설정한다.

**Tech Stack:** Unreal Engine 5.7, C++23, Slate/UMG (SLeafWidget, UWidget, UVerticalBox)

## Global Constraints

- 소스 파일은 200줄을 넘으면 분리한다 (CLAUDE.md 원칙 5). 이번 변경으로 어떤 파일도 200줄을 넘지 않는다.
- 편집이 끝나면 clang-tidy --fix 및 clang-format(microsoft 스타일)을 diff 대상 `.cpp`/`.hpp`(`.h` 포함)에 적용한다 (프로젝트 CLAUDE.md).
- 이 프로젝트에는 Automation Test 프레임워크가 전혀 없음 (조사 결과: `IMPLEMENT_SIMPLE_AUTOMATION_TEST` 등 0건). 각 태스크의 검증은 "빌드 성공"으로 대체하고, 최종 시각적 확인은 마지막 태스크에서 사용자가 PIE로 수행한다.
- 빌드 명령 (에디터가 Live Coding으로 열려 있어도 게임 타겟은 빌드 가능):
  `& "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" DroneSim Win64 Development -Project="C:\Users\hksky\sources\eeg-for-drone\DroneSim\DroneSim.uproject" -WaitMutex`
- `UnrealEditor.exe`를 강제 종료하지 않는다 (저장되지 않은 맵 편집 손실 위험).
- 범위 밖: `WBP_EegHud.uasset`에서 `ProbabilityText` → `ProbabilityBox` 리네임은 UMG 에디터 수동 작업이며 이미 사용자에게 별도 안내됨. 이 플랜에서는 다루지 않는다.

---

### Task 1: EegConfig에 전극명 배열 추가

**Files:**
- Modify: `Source\DroneSim\Eeg\EegTypes.h:9-12` (ChannelCount 선언 바로 아래에 삽입)

**Interfaces:**
- Produces: `EegConfig::ChannelNames` — `constexpr const TCHAR *[EegConfig::ChannelCount]`, 인덱스는 기존 `EegConfig::ChannelCount`/`GroupStartChannel`과 동일한 채널 인덱스 체계를 따름. Task 2에서 `EegConfig::ChannelNames[Channel]`로 소비.

- [ ] **Step 1: 배열 추가 + 컴파일 타임 크기 검증**

`Source\DroneSim\Eeg\EegTypes.h`의 `inline constexpr int32 ChannelCount = 32;` (현재 11번째 줄) 바로 다음, `SampleRateHz` 선언 앞에 삽입:

```cpp
/** International 10-20/10-10 electrode names for the 32 simulated channels, in the same
 *  order as ChannelCount / GroupStartChannel. Approximates a standard actiCAP/EasyCap
 *  32-channel montage (matches MNE's easycap-M1 template channel order); the simulated
 *  device has no real montage, so this is a display label only. */
inline constexpr const TCHAR *ChannelNames[ChannelCount] = {
    TEXT("Fp1"), TEXT("Fp2"), TEXT("F7"), TEXT("F3"), TEXT("Fz"), TEXT("F4"), TEXT("F8"), TEXT("FC5"),
    TEXT("FC1"), TEXT("FC2"), TEXT("FC6"), TEXT("T7"), TEXT("C3"), TEXT("Cz"), TEXT("C4"), TEXT("T8"),
    TEXT("CP5"), TEXT("CP1"), TEXT("CP2"), TEXT("CP6"), TEXT("P7"), TEXT("P3"), TEXT("Pz"), TEXT("P4"),
    TEXT("P8"), TEXT("PO9"), TEXT("O1"), TEXT("Oz"), TEXT("O2"), TEXT("PO10"), TEXT("AF7"), TEXT("AF8"),
};
static_assert(UE_ARRAY_COUNT(ChannelNames) == ChannelCount, "ChannelNames must have one entry per channel");
```

- [ ] **Step 2: 빌드로 컴파일 및 static_assert 검증**

Run:
```powershell
& "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" DroneSim Win64 Development -Project="C:\Users\hksky\sources\eeg-for-drone\DroneSim\DroneSim.uproject" -WaitMutex
```
Expected: `Result: Succeeded` (배열 길이가 32와 다르면 `static_assert` 실패로 컴파일 에러가 나야 하므로, 의도적으로 항목 하나를 지워서 빌드가 실패하는지 먼저 확인한 뒤 원복해도 됨 — 필수는 아님).

- [ ] **Step 3: clang-tidy/clang-format 적용**

```powershell
$files = git diff --name-only HEAD -- '*.cpp' '*.hpp' '*.h'
& "C:\Program Files\LLVM\bin\clang-tidy.exe" --fix --fix-errors `
  --checks='readability-braces-around-statements,modernize-use-trailing-return-type,modernize-use-nullptr,modernize-use-auto,modernize-use-override,misc-unused-parameters,bugprone-*,performance-*' `
  $files -- -std=c++23
& "C:\Program Files\LLVM\bin\clang-format.exe" -i -style=microsoft $files
```

- [ ] **Step 4: Commit**

```bash
git add Source/DroneSim/Eeg/EegTypes.h
git commit -m "feat: EegConfig에 32채널 전극명 배열 추가"
```

---

### Task 2: GraphPanel에 전극 라벨 렌더링

**Files:**
- Modify: `Source\DroneSim\Eeg\EegGraphPanel.h:26-47` (프로퍼티 추가)
- Modify: `Source\DroneSim\Eeg\EegGraphPanel.cpp` (`SetStyle`, `OnPaint`, `SynchronizeProperties`)

**Interfaces:**
- Consumes: `EegConfig::ChannelNames`(Task 1), `EegConfig::ChannelCount`, `EegConfig::GraphWindowSamples`
- Produces: `UEegGraphPanel::LabelColumnWidth`(float), `LabelColor`(FLinearColor), `LabelFontSize`(int32) — 블루프린트에서 조정 가능한 `EditAnywhere` 프로퍼티. 이후 태스크에서 소비하지 않음(최종 태스크).

- [ ] **Step 1: 헤더에 라벨 스타일 프로퍼티 추가**

`Source\DroneSim\Eeg\EegGraphPanel.h`에서 `PanelDesiredSize` 선언(34-36번째 줄) 바로 다음에 삽입:

```cpp
    /** Width in pixels reserved on the left for the per-channel electrode name labels */
    UPROPERTY(EditAnywhere, Category = "EEG")
    float LabelColumnWidth = 34.0f;

    /** Electrode label text color */
    UPROPERTY(EditAnywhere, Category = "EEG")
    FLinearColor LabelColor = FLinearColor(0.7f, 0.7f, 0.7f, 1.0f);

    /** Electrode label font size; strips are ~16px tall at the default panel size, so this
     *  has a practical floor around 8 before labels overlap */
    UPROPERTY(EditAnywhere, Category = "EEG")
    int32 LabelFontSize = 8;
```

- [ ] **Step 2: SEegGraph에 라벨 스타일 멤버 추가 및 SetStyle 시그니처 확장**

`Source\DroneSim\Eeg\EegGraphPanel.cpp`에서 `SEegGraph` 클래스의 `SetStyle`과 멤버 변수(현재 26-47번째 줄)를 아래로 교체:

```cpp
    void SetStyle(const FLinearColor &InLineColor, const FLinearColor &InBackgroundColor,
                  const FVector2D &InDesiredSize, float InLabelColumnWidth, const FLinearColor &InLabelColor,
                  int32 InLabelFontSize)
    {
        LineColor = InLineColor;
        BackgroundColor = InBackgroundColor;
        DesiredSize = InDesiredSize;
        LabelColumnWidth = InLabelColumnWidth;
        LabelColor = InLabelColor;
        LabelFontSize = InLabelFontSize;
    }

    auto ComputeDesiredSize(float) const -> FVector2D override
    {
        return DesiredSize;
    }

    auto OnPaint(const FPaintArgs &Args, const FGeometry &AllottedGeometry, const FSlateRect &MyCullingRect,
                 FSlateWindowElementList &OutDrawElements, int32 LayerId, const FWidgetStyle &InWidgetStyle,
                 bool bParentEnabled) const -> int32 override;

  private:
    TWeakObjectPtr<const UEegRunnerComponent> Runner;
    FLinearColor LineColor = FLinearColor::Green;
    FLinearColor BackgroundColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.6f);
    FVector2D DesiredSize = FVector2D(340.0f, 520.0f);
    float LabelColumnWidth = 34.0f;
    FLinearColor LabelColor = FLinearColor(0.7f, 0.7f, 0.7f, 1.0f);
    int32 LabelFontSize = 8;
};
```

- [ ] **Step 3: OnPaint에서 좌측 여백을 확보하고 라벨을 그린다**

`Source\DroneSim\Eeg\EegGraphPanel.cpp`의 `SEegGraph::OnPaint` 본문(현재 79-103번째 줄)을 아래로 교체:

```cpp
    const float StripHeight = Size.Y / static_cast<float>(EegConfig::ChannelCount);
    const float GraphOriginX = FMath::Min(LabelColumnWidth, Size.X * 0.5f);
    const float GraphWidth = FMath::Max(Size.X - GraphOriginX, 1.0f);
    const float AmplitudeScale = (StripHeight * 0.45f) / FullScaleMicrovolts;
    const float StepX = GraphWidth / static_cast<float>(PointCount - 1);

    const FSlateFontInfo LabelFont = FCoreStyle::GetDefaultFontStyle("Regular", LabelFontSize);

    TArray<FVector2f> Points;
    Points.SetNumUninitialized(PointCount);

    for (int32 Channel = 0; Channel < EegConfig::ChannelCount; ++Channel)
    {
        const float CenterY = (static_cast<float>(Channel) + 0.5f) * StripHeight;
        const int32 ChannelOffset = Channel * EegConfig::GraphWindowSamples;

        const FVector2f LabelPosition(2.0f, CenterY - static_cast<float>(LabelFontSize) * 0.5f);
        FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1,
                                    AllottedGeometry.ToPaintGeometry(FVector2f(GraphOriginX, StripHeight),
                                                                     FSlateLayoutTransform(LabelPosition)),
                                    FString(EegConfig::ChannelNames[Channel]), LabelFont, ESlateDrawEffect::None,
                                    LabelColor);

        for (int32 Point = 0; Point < PointCount; ++Point)
        {
            // ring order: OldestIndex is the oldest sample, so time flows left to right
            const int32 SampleIndex = (OldestIndex + Point * PointStride) % EegConfig::GraphWindowSamples;
            const float Value = Buffer[ChannelOffset + SampleIndex];
            Points[Point] = FVector2f(GraphOriginX + static_cast<float>(Point) * StepX,
                                      FMath::Clamp(CenterY - Value * AmplitudeScale, CenterY - StripHeight * 0.5f,
                                                   CenterY + StripHeight * 0.5f));
        }

        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(), Points,
                                     ESlateDrawEffect::None, LineColor, false, 1.0f);
    }

    return LayerId + 3;
```

`Source\DroneSim\Eeg\EegGraphPanel.cpp` 상단 include에 `#include "Fonts/SlateFontInfo.h"`가 필요하면 추가한다 (`Styling/CoreStyle.h`가 보통 함께 끌어오지만, 컴파일 에러 시 명시적으로 추가).

- [ ] **Step 4: SynchronizeProperties에서 새 인자 전달**

`Source\DroneSim\Eeg\EegGraphPanel.cpp`의 `UEegGraphPanel::SynchronizeProperties()`(현재 117-124번째 줄) 본문 교체:

```cpp
void UEegGraphPanel::SynchronizeProperties()
{
    Super::SynchronizeProperties();
    if (Graph.IsValid())
    {
        Graph->SetStyle(LineColor, BackgroundColor, PanelDesiredSize, LabelColumnWidth, LabelColor, LabelFontSize);
    }
}
```

`UEegGraphPanel::RebuildWidget()`의 `Graph->SetStyle(LineColor, BackgroundColor, PanelDesiredSize);` 호출(현재 136번째 줄)도 동일하게 인자를 확장:

```cpp
    Graph->SetStyle(LineColor, BackgroundColor, PanelDesiredSize, LabelColumnWidth, LabelColor, LabelFontSize);
```

- [ ] **Step 5: 빌드로 컴파일 검증**

Run:
```powershell
& "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" DroneSim Win64 Development -Project="C:\Users\hksky\sources\eeg-for-drone\DroneSim\DroneSim.uproject" -WaitMutex
```
Expected: `Result: Succeeded`

- [ ] **Step 6: clang-tidy/clang-format 적용**

```powershell
$files = git diff --name-only HEAD -- '*.cpp' '*.hpp' '*.h'
& "C:\Program Files\LLVM\bin\clang-tidy.exe" --fix --fix-errors `
  --checks='readability-braces-around-statements,modernize-use-trailing-return-type,modernize-use-nullptr,modernize-use-auto,modernize-use-override,misc-unused-parameters,bugprone-*,performance-*' `
  $files -- -std=c++23
& "C:\Program Files\LLVM\bin\clang-format.exe" -i -style=microsoft $files
```

- [ ] **Step 7: Commit**

```bash
git add Source/DroneSim/Eeg/EegGraphPanel.h Source/DroneSim/Eeg/EegGraphPanel.cpp
git commit -m "feat: GraphPanel 좌측에 전극명 라벨 렌더링 추가"
```

---

### Task 3: ProbabilityBox 확률 라인 우측 정렬

**Files:**
- Modify: `Source\DroneSim\Eeg\EegHudWidget.cpp:23-34` (`NativeOnInitialized`)

**Interfaces:**
- Consumes: 없음 (독립적인 정렬 변경, Task 1/2와 데이터 의존성 없음)
- Produces: 없음 (최종 사용자 확인 대상)

- [ ] **Step 1: 슬롯 정렬을 우측으로 설정**

`Source\DroneSim\Eeg\EegHudWidget.cpp`의 `NativeOnInitialized()`(현재 17-35번째 줄) 본문 교체:

```cpp
void UEegHudWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // one text line per action inside the designed box, so the winning line can be
    // recolored on its own (a single multi-line TextBlock has only one color); right-aligned
    // so the trailing "%" value lines up regardless of action-name length
    if (ProbabilityBox != nullptr)
    {
        ProbabilityBox->ClearChildren();
        for (int32 Index = 0; Index < EegConfig::ProbCount; ++Index)
        {
            UTextBlock *Line = NewObject<UTextBlock>(this);
            Line->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", ProbabilityFontSize));
            Line->SetColorAndOpacity(FSlateColor(ProbabilityColor));
            Line->SetJustification(ETextJustify::Right);
            UVerticalBoxSlot *Slot = ProbabilityBox->AddChildToVerticalBox(Line);
            if (Slot != nullptr)
            {
                Slot->SetHorizontalAlignment(HAlign_Right);
            }
            ProbabilityLines.Add(Line);
        }
    }
}
```

- [ ] **Step 2: 빌드로 컴파일 검증**

Run:
```powershell
& "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" DroneSim Win64 Development -Project="C:\Users\hksky\sources\eeg-for-drone\DroneSim\DroneSim.uproject" -WaitMutex
```
Expected: `Result: Succeeded`

- [ ] **Step 3: clang-tidy/clang-format 적용**

```powershell
$files = git diff --name-only HEAD -- '*.cpp' '*.hpp' '*.h'
& "C:\Program Files\LLVM\bin\clang-tidy.exe" --fix --fix-errors `
  --checks='readability-braces-around-statements,modernize-use-trailing-return-type,modernize-use-nullptr,modernize-use-auto,modernize-use-override,misc-unused-parameters,bugprone-*,performance-*' `
  $files -- -std=c++23
& "C:\Program Files\LLVM\bin\clang-format.exe" -i -style=microsoft $files
```

- [ ] **Step 4: Commit**

```bash
git add Source/DroneSim/Eeg/EegHudWidget.cpp
git commit -m "feat: ProbabilityBox 확률 라인을 우측 정렬로 변경"
```

---

### Task 4: PIE 수동 시각 확인 (사용자 수행)

**Files:** 없음 (코드 변경 없음, 검증 전용)

**Interfaces:**
- Consumes: Task 1-3의 모든 산출물
- Produces: 없음

이 프로젝트에는 Automation Test가 없고 Slate `OnPaint` 렌더링은 실제 화면에서만 확인 가능하므로, 아래는 사용자가 직접 수행한다 (에디터를 강제 종료하지 않기 위해 자동화하지 않음):

- [ ] **Step 1: Live Coding으로 변경사항 반영**

에디터가 열려 있다면 `Ctrl+Alt+F11`로 Live Coding 컴파일을 트리거한다. (Task 1-3에서 이미 `Build.bat DroneSim`으로 컴파일 성공을 확인했으므로 문법 오류는 없는 상태.)

- [ ] **Step 2: PIE로 EEG Running Mode 진입**

씬을 Play(PIE)하여 EEG Running Mode HUD가 뜨는 상태로 진입한다.

- [ ] **Step 3: GraphPanel 라벨 확인**

GraphPanel 좌측에 32개 전극명(Fp1, Fp2, F7, ... AF8)이 각 파형 스트립과 겹치지 않고 표시되는지 확인한다. 겹치거나 잘리면 `LabelFontSize`(기본 8) 또는 `LabelColumnWidth`(기본 34)를 블루프린트 디테일 패널에서 조정한다.

- [ ] **Step 4: ProbabilityBox 정렬 확인**

ProbabilityBox의 5줄(FORWARD/BACKWARD/LEFT/RIGHT/STOP)이 우측 끝(`%` 값)을 기준으로 정렬되어 표시되는지, 최고 확률 줄이 여전히 강조색으로 표시되는지 확인한다.

- [ ] **Step 5: 문제 없으면 완료 보고**

문제가 없으면 작업이 끝난 것으로 간주한다. 문제가 있으면 구체적 증상(스크린샷 또는 설명)을 알려준다.
