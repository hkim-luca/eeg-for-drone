#include "DroneProbabilityPanel.h"
#include "Eeg/EegRunnerComponent.h"
#include "Eeg/EegTypes.h"
#include "Rendering/DrawElements.h"
#include "Scenario/ScenarioTypes.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"

/** Slate widget painting one horizontal bar per action probability */
class SDroneProbabilityPanel final : public SLeafWidget
{
  public:
    SLATE_BEGIN_ARGS(SDroneProbabilityPanel)
    {
    }
    SLATE_END_ARGS()

    void Construct(const FArguments &InArgs)
    {
        SetCanTick(false); // repainted every frame anyway because the game viewport is volatile
    }

    void SetRunner(const TWeakObjectPtr<const UEegRunnerComponent> &InRunner)
    {
        Runner = InRunner;
    }

    void SetStyle(const FLinearColor &InBackgroundColor, const FLinearColor &InTrackColor,
                  const FLinearColor &InBarColor, const FLinearColor &InHighlightColor, int32 InFontSize,
                  const FVector2D &InDesiredSize)
    {
        BackgroundColor = InBackgroundColor;
        TrackColor = InTrackColor;
        BarColor = InBarColor;
        HighlightColor = InHighlightColor;
        FontSize = InFontSize;
        DesiredSize = InDesiredSize;
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
    FLinearColor BackgroundColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
    FLinearColor TrackColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.15f);
    FLinearColor BarColor = FLinearColor(0.03f, 0.45f, 0.14f, 1.0f);
    FLinearColor HighlightColor = FLinearColor(0.75f, 0.35f, 0.02f, 1.0f);
    int32 FontSize = 14;
    FVector2D DesiredSize = FVector2D(220.0f, 130.0f);
};

auto SDroneProbabilityPanel::OnPaint(const FPaintArgs &Args, const FGeometry &AllottedGeometry,
                                     const FSlateRect &MyCullingRect, FSlateWindowElementList &OutDrawElements,
                                     int32 LayerId, const FWidgetStyle &InWidgetStyle, bool /*bParentEnabled*/) const
    -> int32
{
    const FVector2f Size = AllottedGeometry.GetLocalSize();

    if (BackgroundColor.A > 0.0f)
    {
        FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
                                   FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, BackgroundColor);
    }

    const UEegRunnerComponent *Pinned = Runner.Get();
    if (Pinned == nullptr || Size.X <= 0.0f || Size.Y <= 0.0f)
    {
        return LayerId + 1;
    }

    const TConstArrayView<float> Probs = Pinned->GetLastActionProbs();

    int32 BestIndex = INDEX_NONE; // stays INDEX_NONE until the first result arrives
    for (int32 Index = 0; Index < EegConfig::ProbCount; ++Index)
    {
        if (Probs[Index] > 0.0f && (BestIndex == INDEX_NONE || Probs[Index] > Probs[BestIndex]))
        {
            BestIndex = Index;
        }
    }

    // panel edge padding and label|bar|percent column gaps scale with the font so the rows
    // keep breathing room at any size instead of the three columns touching each other
    const float Padding = static_cast<float>(FontSize) * 0.5f;
    const float ColumnGap = static_cast<float>(FontSize) * 0.75f;
    const float RowHeight = (Size.Y - Padding * 2.0f) / static_cast<float>(EegConfig::ProbCount);
    // label fits "▼ BACKWARD" and percent fits "00.0%" at the configured font size; both are
    // capped to fractions of the panel so a narrow layout still leaves room for the bar
    const float LabelWidth = FMath::Min(static_cast<float>(FontSize) * 7.5f, Size.X * 0.4f);
    const float PercentWidth = FMath::Min(static_cast<float>(FontSize) * 3.6f, Size.X * 0.25f);
    const float BarOriginX = Padding + LabelWidth + ColumnGap;
    const float BarWidth = FMath::Max(Size.X - BarOriginX - ColumnGap - PercentWidth - Padding, 1.0f);
    const float BarHeight = RowHeight * 0.45f;
    // OSD-style black outline keeps the floating text readable over bright scenery
    FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold", FontSize);
    Font.OutlineSettings = FFontOutlineSettings(1, FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
    const FSlateBrush *WhiteBrush = FCoreStyle::Get().GetBrush("WhiteBrush");

    for (int32 Index = 0; Index < EegConfig::ProbCount; ++Index)
    {
        const EScenarioAction Action = EegConfig::ProbOrder[Index];
        const float Prob = FMath::Clamp(Probs[Index], 0.0f, 1.0f);
        const FLinearColor RowColor = Index == BestIndex ? HighlightColor : BarColor;
        const float CenterY = Padding + (static_cast<float>(Index) + 0.5f) * RowHeight;
        const float TextY = CenterY - static_cast<float>(FontSize) * 0.5f;

        const FString Label = ScenarioActionName(Action);
        FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1,
                                    AllottedGeometry.ToPaintGeometry(FVector2f(LabelWidth, RowHeight),
                                                                     FSlateLayoutTransform(FVector2f(Padding, TextY))),
                                    Label, Font, ESlateDrawEffect::None, RowColor);

        const FVector2f TrackPosition(BarOriginX, CenterY - BarHeight * 0.5f);
        FSlateDrawElement::MakeBox(
            OutDrawElements, LayerId + 1,
            AllottedGeometry.ToPaintGeometry(FVector2f(BarWidth, BarHeight), FSlateLayoutTransform(TrackPosition)),
            WhiteBrush, ESlateDrawEffect::None, TrackColor);

        const float FillWidth = Prob * BarWidth;
        if (FillWidth > 0.0f)
        {
            FSlateDrawElement::MakeBox(
                OutDrawElements, LayerId + 2,
                AllottedGeometry.ToPaintGeometry(FVector2f(FillWidth, BarHeight), FSlateLayoutTransform(TrackPosition)),
                WhiteBrush, ESlateDrawEffect::None, RowColor);
        }

        const FString PercentText = FString::Printf(TEXT("%04.1f%%"), Prob * 100.0f);
        FSlateDrawElement::MakeText(OutDrawElements, LayerId + 3,
                                    AllottedGeometry.ToPaintGeometry(
                                        FVector2f(PercentWidth, RowHeight),
                                        FSlateLayoutTransform(FVector2f(BarOriginX + BarWidth + ColumnGap, TextY))),
                                    PercentText, Font, ESlateDrawEffect::None, RowColor);
    }

    return LayerId + 4;
}

void UDroneProbabilityPanel::SetRunner(const UEegRunnerComponent *InRunner)
{
    PendingRunner = InRunner;
    if (Panel.IsValid())
    {
        Panel->SetRunner(PendingRunner);
    }
}

void UDroneProbabilityPanel::SynchronizeProperties()
{
    Super::SynchronizeProperties();
    if (Panel.IsValid())
    {
        Panel->SetStyle(BackgroundColor, TrackColor, BarColor, HighlightColor, FontSize, PanelDesiredSize);
    }
}

void UDroneProbabilityPanel::ReleaseSlateResources(bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);
    Panel.Reset();
}

auto UDroneProbabilityPanel::RebuildWidget() -> TSharedRef<SWidget>
{
    Panel = SNew(SDroneProbabilityPanel);
    Panel->SetRunner(PendingRunner);
    Panel->SetStyle(BackgroundColor, TrackColor, BarColor, HighlightColor, FontSize, PanelDesiredSize);
    return Panel.ToSharedRef();
}
