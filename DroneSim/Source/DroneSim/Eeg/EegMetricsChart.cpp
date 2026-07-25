#include "EegMetricsChart.h"
#include "EegRunnerComponent.h"
#include "EegTypes.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"

namespace
{
/** Visible window for the reliability strip: 2 s of history, matching the electrode graph's
 *  own window; one action result arrives per completed EegFrame (EegConfig::SamplesPerFrame
 *  at EegConfig::SampleRateHz) */
constexpr int32 ActionResultsPerSecond = EegConfig::SampleRateHz / EegConfig::SamplesPerFrame;
constexpr float WindowSeconds = 2.0f;
constexpr int32 WindowSampleCount = static_cast<int32>(WindowSeconds * ActionResultsPerSecond);

/** COMM RELIABILITY is always a percentage, so its strip uses a fixed 0-100 scale like the
 *  dashboard's probability chart */
constexpr float ReliabilityMin = 0.0f;
constexpr float ReliabilityMax = 100.0f;

/** Pipeline line-chart segments, in device->control order */
constexpr int32 PipelineSegmentCount = 4;
} // namespace

/** Slate widget painting the device->infer->control pipeline as 4 line series (top row) and the
 *  COMM RELIABILITY time-series strip (bottom row) from the runner's metrics history. Labels are
 *  kept minimal (this is an OSD overlay) - color alone tells the segments apart, matching the
 *  dashboard's palette. */
class SEegMetricsChart final : public SLeafWidget
{
  public:
    SLATE_BEGIN_ARGS(SEegMetricsChart)
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

    void SetStyle(const FLinearColor &InDeviceToInferColor, const FLinearColor &InInferColor,
                  const FLinearColor &InInferToControlColor, const FLinearColor &InOverheadColor,
                  const FLinearColor &InTotalColor, const FLinearColor &InReliabilityColor,
                  const FLinearColor &InBackgroundColor, const FVector2D &InDesiredSize,
                  const FLinearColor &InLabelColor, int32 InLabelFontSize)
    {
        DeviceToInferColor = InDeviceToInferColor;
        InferColor = InInferColor;
        InferToControlColor = InInferToControlColor;
        OverheadColor = InOverheadColor;
        TotalColor = InTotalColor;
        ReliabilityColor = InReliabilityColor;
        BackgroundColor = InBackgroundColor;
        DesiredSize = InDesiredSize;
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
    /** Top row: device->infer->control pipeline, one line per segment over the visible
     *  history window */
    auto PaintPipeline(TConstArrayView<FEegMetrics> History, const FGeometry &AllottedGeometry, float RowTop,
                       float RowHeight, FSlateWindowElementList &OutDrawElements, int32 LayerId,
                       const FSlateFontInfo &Font) const -> void;

    /** Bottom row: COMM RELIABILITY over the visible history window */
    auto PaintReliability(TConstArrayView<FEegMetrics> History, const FGeometry &AllottedGeometry, float RowTop,
                          float RowHeight, FSlateWindowElementList &OutDrawElements, int32 LayerId,
                          const FSlateFontInfo &Font) const -> void;

    TWeakObjectPtr<const UEegRunnerComponent> Runner;
    FLinearColor DeviceToInferColor = FLinearColor(0.16f, 0.47f, 0.84f, 0.75f);
    FLinearColor InferColor = FLinearColor(0.75f, 0.35f, 0.02f, 0.9f);
    FLinearColor InferToControlColor = FLinearColor(0.03f, 0.55f, 0.2f, 0.75f);
    FLinearColor OverheadColor = FLinearColor(0.5f, 0.5f, 0.5f, 0.5f);
    FLinearColor TotalColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.85f);
    FLinearColor ReliabilityColor = FLinearColor(0.03f, 0.45f, 0.14f, 0.7f);
    FLinearColor BackgroundColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
    FVector2D DesiredSize = FVector2D(260.0f, 140.0f);
    FLinearColor LabelColor = FLinearColor(0.5f, 0.52f, 0.5f, 0.6f);
    int32 LabelFontSize = 10;
};

namespace
{
/** Per-sample segment breakdown for one point of the pipeline line chart */
struct FPipelineSegments
{
    float Ms[PipelineSegmentCount];
    /** Exact end-to-end device->control total (DroneSim's own clock), not a sum of Ms[] -
     *  drawn as its own line so the overall trend is visible alongside the segments */
    float TotalMs;
};

/** device->infer and infer->control are direct cross-clock subtractions the server computes
 *  from its own frame_received_ms/response_sent_ms against DroneSim's frame_sent_ms/control_ms;
 *  infer is a server-only duration. Overhead is whatever the 3 measured segments don't account
 *  for (scheduling/serialization jitter not captured by the 3 checkpoints). */
auto BuildPipelineSegments(const FEegMetrics &Metrics) -> FPipelineSegments
{
    FPipelineSegments Segments;
    Segments.Ms[0] = FMath::Max(Metrics.LatencyDeviceToInferLastMs, 0.0f);
    Segments.Ms[1] = FMath::Max(Metrics.InferDurationMs, 0.0f);
    Segments.Ms[2] = FMath::Max(Metrics.LatencyInferToControlLastMs, 0.0f);
    const float MeasuredMs = Segments.Ms[0] + Segments.Ms[1] + Segments.Ms[2];
    Segments.Ms[3] = FMath::Max(Metrics.LatencyDeviceToControlLastMs - MeasuredMs, 0.0f);
    Segments.TotalMs = FMath::Max(Metrics.LatencyDeviceToControlLastMs, 0.0f);
    return Segments;
}
} // namespace

auto SEegMetricsChart::PaintPipeline(TConstArrayView<FEegMetrics> History, const FGeometry &AllottedGeometry,
                                     float RowTop, float RowHeight, FSlateWindowElementList &OutDrawElements,
                                     int32 LayerId, const FSlateFontInfo &Font) const -> void
{
    const int32 VisibleCount = FMath::Min(History.Num(), WindowSampleCount);
    if (VisibleCount < 2)
    {
        return;
    }
    const TConstArrayView<FEegMetrics> Visible(History.GetData() + (History.Num() - VisibleCount), VisibleCount);

    const float RowWidth = AllottedGeometry.GetLocalSize().X;

    TArray<FPipelineSegments> Samples;
    Samples.SetNumUninitialized(Visible.Num());
    float MaxValue = 1.0f;
    for (int32 Index = 0; Index < Visible.Num(); ++Index)
    {
        Samples[Index] = BuildPipelineSegments(Visible[Index]);
        for (float SegmentMs : Samples[Index].Ms)
        {
            MaxValue = FMath::Max(MaxValue, SegmentMs);
        }
        MaxValue = FMath::Max(MaxValue, Samples[Index].TotalMs);
    }
    MaxValue *= 1.1f; // headroom so the tallest peak doesn't touch the label row

    // 4 segment lines + the overall total, named exactly as the dashboard legend so the two
    // surfaces agree; laid out like UE's own "stat unit" HUD (Frame/Game/Draw, color-matched
    // name+value text), not tagged at the line's endpoint
    constexpr int32 LineCount = PipelineSegmentCount + 1;
    const FLinearColor LineColors[LineCount] = {DeviceToInferColor, InferColor, InferToControlColor, OverheadColor,
                                                TotalColor};
    const TCHAR *LineNames[LineCount] = {TEXT("device→추론"), TEXT("추론"), TEXT("추론→제어"), TEXT("overhead"),
                                         TEXT("전체")};
    const float LineWidths[LineCount] = {1.0f, 1.0f, 1.0f, 1.0f, 1.5f};
    float EndMs[LineCount];
    for (int32 Line = 0; Line < LineCount; ++Line)
    {
        EndMs[Line] = Line < PipelineSegmentCount ? Samples.Last().Ms[Line] : Samples.Last().TotalMs;
    }

    // measure each "name value" item and wrap them left-to-right into as many legend lines as
    // the row's own width needs, so the reserved label area (and therefore the plot start) is
    // known before any line gets drawn
    const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
    constexpr float ItemGap = 10.0f;
    FString Items[LineCount];
    float ItemWidths[LineCount];
    int32 ItemRow[LineCount];
    float CursorX = 0.0f;
    int32 RowCount = 1;
    for (int32 Line = 0; Line < LineCount; ++Line)
    {
        Items[Line] = FString::Printf(TEXT("%s %.0fms"), LineNames[Line], EndMs[Line]);
        ItemWidths[Line] = FontMeasure->Measure(Items[Line], Font).X;
        if (CursorX > 0.0f && CursorX + ItemWidths[Line] > RowWidth)
        {
            ++RowCount;
            CursorX = 0.0f;
        }
        ItemRow[Line] = RowCount - 1;
        CursorX += ItemWidths[Line] + ItemGap;
    }

    const float LegendLineHeight = static_cast<float>(LabelFontSize) + 4.0f;
    const float LabelAreaHeight = LegendLineHeight * static_cast<float>(RowCount);
    const float StepX = RowWidth / static_cast<float>(Visible.Num() - 1);
    const float PlotTop = RowTop + LabelAreaHeight;
    const float PlotHeight = FMath::Max(RowHeight - LabelAreaHeight, 1.0f);
    const float PlotBottom = PlotTop + PlotHeight;

    TArray<FVector2f> Points;
    Points.SetNumUninitialized(Visible.Num());
    for (int32 Line = 0; Line < LineCount; ++Line)
    {
        for (int32 Index = 0; Index < Visible.Num(); ++Index)
        {
            const float Ms = Line < PipelineSegmentCount ? Samples[Index].Ms[Line] : Samples[Index].TotalMs;
            const float Normalized = FMath::Clamp(Ms / MaxValue, 0.0f, 1.0f);
            Points[Index] = FVector2f(static_cast<float>(Index) * StepX, PlotBottom - Normalized * PlotHeight);
        }
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), Points,
                                     ESlateDrawEffect::None, LineColors[Line], true, LineWidths[Line]);
    }

    // draw the wrapped legend using the positions already computed above
    CursorX = 0.0f;
    int32 CurrentRow = 0;
    for (int32 Line = 0; Line < LineCount; ++Line)
    {
        if (ItemRow[Line] != CurrentRow)
        {
            CurrentRow = ItemRow[Line];
            CursorX = 0.0f;
        }
        const float ItemY = RowTop + static_cast<float>(CurrentRow) * LegendLineHeight;
        FSlateDrawElement::MakeText(OutDrawElements, LayerId + 2,
                                    AllottedGeometry.ToPaintGeometry(FVector2f(ItemWidths[Line], LegendLineHeight),
                                                                     FSlateLayoutTransform(FVector2f(CursorX, ItemY))),
                                    Items[Line], Font, ESlateDrawEffect::None, LineColors[Line]);
        CursorX += ItemWidths[Line] + ItemGap;
    }
}

auto SEegMetricsChart::PaintReliability(TConstArrayView<FEegMetrics> History, const FGeometry &AllottedGeometry,
                                        float RowTop, float RowHeight, FSlateWindowElementList &OutDrawElements,
                                        int32 LayerId, const FSlateFontInfo &Font) const -> void
{
    const int32 VisibleCount = FMath::Min(History.Num(), WindowSampleCount);
    if (VisibleCount < 2)
    {
        return;
    }
    const TConstArrayView<FEegMetrics> Visible(History.GetData() + (History.Num() - VisibleCount), VisibleCount);

    const float RowWidth = AllottedGeometry.GetLocalSize().X;
    const float StepX = RowWidth / static_cast<float>(Visible.Num() - 1);

    // reserve room for the label at the top so the line never draws over it
    const float LabelAreaHeight = static_cast<float>(LabelFontSize) + 6.0f;
    const float PlotTop = RowTop + LabelAreaHeight;
    const float PlotHeight = FMath::Max(RowHeight - LabelAreaHeight, 1.0f);

    TArray<FVector2f> Points;
    Points.SetNumUninitialized(Visible.Num());
    for (int32 Index = 0; Index < Visible.Num(); ++Index)
    {
        const float Value = Visible[Index].ReliabilityOverallPercent;
        const float Normalized = FMath::Clamp((Value - ReliabilityMin) / (ReliabilityMax - ReliabilityMin), 0.0f, 1.0f);
        Points[Index] = FVector2f(static_cast<float>(Index) * StepX, PlotTop + (1.0f - Normalized) * PlotHeight);
    }

    FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), Points,
                                 ESlateDrawEffect::None, ReliabilityColor, true, 1.0f);

    // title + current value + y-axis scale, all on the one reserved label line so it never
    // competes with the plot area for vertical space (a fixed 0-100% range)
    const FEegMetrics &Latest = Visible.Last();
    const FString Label =
        FString::Printf(TEXT("드론 제어 통신 신뢰도 %.0f%% (0~100%%)"), Latest.ReliabilityOverallPercent);
    FSlateDrawElement::MakeText(OutDrawElements, LayerId + 2,
                                AllottedGeometry.ToPaintGeometry(FVector2f(RowWidth, LabelAreaHeight),
                                                                 FSlateLayoutTransform(FVector2f(0.0f, RowTop))),
                                Label, Font, ESlateDrawEffect::None, LabelColor);
}

auto SEegMetricsChart::OnPaint(const FPaintArgs &Args, const FGeometry &AllottedGeometry,
                               const FSlateRect &MyCullingRect, FSlateWindowElementList &OutDrawElements, int32 LayerId,
                               const FWidgetStyle &InWidgetStyle, bool /*bParentEnabled*/) const -> int32
{
    const FVector2f Size = AllottedGeometry.GetLocalSize();

    FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
                               FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, BackgroundColor);

    const UEegRunnerComponent *Pinned = Runner.Get();
    if (Pinned == nullptr || Size.X <= 0.0f || Size.Y <= 0.0f)
    {
        return LayerId + 1;
    }

    const TConstArrayView<FEegMetrics> History = Pinned->GetMetricsHistory();
    if (History.Num() < 2)
    {
        return LayerId + 1;
    }

    const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", LabelFontSize);
    constexpr float RowGap = 8.0f; // breathing room between the pipeline and reliability rows
    // the pipeline row carries 5 lines' worth of information vs. reliability's 1, so it gets
    // the larger share of the available height
    constexpr float PipelineHeightRatio = 0.65f;
    const float AvailableHeight = Size.Y - RowGap;
    const float PipelineHeight = AvailableHeight * PipelineHeightRatio;
    const float ReliabilityHeight = AvailableHeight - PipelineHeight;

    PaintPipeline(History, AllottedGeometry, 0.0f, PipelineHeight, OutDrawElements, LayerId, Font);
    PaintReliability(History, AllottedGeometry, PipelineHeight + RowGap, ReliabilityHeight, OutDrawElements, LayerId,
                     Font);

    return LayerId + 3;
}

void UEegMetricsChart::SetRunner(const UEegRunnerComponent *InRunner)
{
    PendingRunner = InRunner;
    if (Chart.IsValid())
    {
        Chart->SetRunner(PendingRunner);
    }
}

void UEegMetricsChart::SynchronizeProperties()
{
    Super::SynchronizeProperties();
    if (Chart.IsValid())
    {
        Chart->SetStyle(DeviceToInferColor, InferColor, InferToControlColor, OverheadColor, TotalColor,
                        ReliabilityColor, BackgroundColor, PanelDesiredSize, LabelColor, LabelFontSize);
    }
}

void UEegMetricsChart::ReleaseSlateResources(bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);
    Chart.Reset();
}

auto UEegMetricsChart::RebuildWidget() -> TSharedRef<SWidget>
{
    Chart = SNew(SEegMetricsChart);
    Chart->SetRunner(PendingRunner);
    Chart->SetStyle(DeviceToInferColor, InferColor, InferToControlColor, OverheadColor, TotalColor, ReliabilityColor,
                    BackgroundColor, PanelDesiredSize, LabelColor, LabelFontSize);
    return Chart.ToSharedRef();
}
