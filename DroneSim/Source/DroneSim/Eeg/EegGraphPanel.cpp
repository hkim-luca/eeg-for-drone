#include "EegGraphPanel.h"
#include "EegRunnerComponent.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"

/** Slate widget painting the 32 electrode strips from the simulator's graph ring buffer */
class SEegGraph final : public SLeafWidget
{
  public:
    SLATE_BEGIN_ARGS(SEegGraph)
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
    FLinearColor LineColor = FLinearColor(0.03f, 0.45f, 0.14f, 1.0f);
    FLinearColor BackgroundColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.6f);
    FVector2D DesiredSize = FVector2D(340.0f, 520.0f);
    float LabelColumnWidth = 34.0f;
    FLinearColor LabelColor = FLinearColor(0.5f, 0.52f, 0.5f, 0.92f);
    int32 LabelFontSize = 8;
};

auto SEegGraph::OnPaint(const FPaintArgs &Args, const FGeometry &AllottedGeometry, const FSlateRect &MyCullingRect,
                        FSlateWindowElementList &OutDrawElements, int32 LayerId, const FWidgetStyle &InWidgetStyle,
                        bool /*bParentEnabled*/) const -> int32
{
    const FVector2f Size = AllottedGeometry.GetLocalSize();

    FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
                               FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, BackgroundColor);

    const UEegRunnerComponent *Pinned = Runner.Get();
    if (Pinned == nullptr || Size.X <= 0.0f || Size.Y <= 0.0f)
    {
        return LayerId + 1;
    }

    const TArray<float> &Buffer = Pinned->GetSimulator().GetGraphBuffer();
    const int32 OldestIndex = Pinned->GetSimulator().GetGraphWriteIndex();
    if (Buffer.Num() < EegConfig::ChannelCount * EegConfig::GraphWindowSamples)
    {
        return LayerId + 1;
    }

    // one point per PointStride samples keeps the draw call at ~125 points per channel
    constexpr int32 PointStride = 4;
    constexpr int32 PointCount = EegConfig::GraphWindowSamples / PointStride;

    // amplitude that maps to a strip half-height; the boosted rhythm peaks around ±40 uV
    constexpr float FullScaleMicrovolts = 40.0f;

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
}

void UEegGraphPanel::SetRunner(const UEegRunnerComponent *InRunner)
{
    PendingRunner = InRunner;
    if (Graph.IsValid())
    {
        Graph->SetRunner(PendingRunner);
    }
}

void UEegGraphPanel::SynchronizeProperties()
{
    Super::SynchronizeProperties();
    if (Graph.IsValid())
    {
        Graph->SetStyle(LineColor, BackgroundColor, PanelDesiredSize, LabelColumnWidth, LabelColor, LabelFontSize);
    }
}

void UEegGraphPanel::ReleaseSlateResources(bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);
    Graph.Reset();
}

auto UEegGraphPanel::RebuildWidget() -> TSharedRef<SWidget>
{
    Graph = SNew(SEegGraph);
    Graph->SetRunner(PendingRunner);
    Graph->SetStyle(LineColor, BackgroundColor, PanelDesiredSize, LabelColumnWidth, LabelColor, LabelFontSize);
    return Graph.ToSharedRef();
}
