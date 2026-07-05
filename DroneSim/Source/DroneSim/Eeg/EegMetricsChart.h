#pragma once

#include "Components/Widget.h"
#include "CoreMinimal.h"

#include "EegMetricsChart.generated.h"

class SEegMetricsChart;
class UEegRunnerComponent;

/**
 *  UMG widget that draws the same latency/reliability KPIs as the eeg-server dashboard as two
 *  stacked time-series rows: a device->infer->control pipeline line chart (top, one line per
 *  segment: device->infer, infer processing, infer->control, and any unaccounted overhead) and
 *  a COMM RELIABILITY line strip (bottom), both fed by UEegRunnerComponent::GetMetricsHistory()
 *  over the same visible window. Text is kept minimal (this is an OSD overlay) - color alone
 *  distinguishes the pipeline segments, matching the dashboard's palette. Drop it into a Widget
 *  Blueprint and size it there; the drawing adapts to whatever geometry the layout gives it.
 */
UCLASS()
class UEegMetricsChart : public UWidget
{
    GENERATED_BODY()

  public:
    /** Sets the running-mode driver whose metrics history feeds the chart */
    void SetRunner(const UEegRunnerComponent *InRunner);

    /** Pipeline bar color: device->infer segment */
    UPROPERTY(EditAnywhere, Category = "EEG")
    FLinearColor DeviceToInferColor = FLinearColor(0.16f, 0.47f, 0.84f, 0.75f);

    /** Pipeline bar color: exact server-side inference segment */
    UPROPERTY(EditAnywhere, Category = "EEG")
    FLinearColor InferColor = FLinearColor(0.75f, 0.35f, 0.02f, 0.9f);

    /** Pipeline bar color: infer->control segment */
    UPROPERTY(EditAnywhere, Category = "EEG")
    FLinearColor InferToControlColor = FLinearColor(0.03f, 0.55f, 0.2f, 0.75f);

    /** Pipeline bar color: unaccounted overhead (device_to_control minus the other 3 segments) */
    UPROPERTY(EditAnywhere, Category = "EEG")
    FLinearColor OverheadColor = FLinearColor(0.5f, 0.5f, 0.5f, 0.5f);

    /** Pipeline line color: overall end-to-end device->control total, drawn on top of the
     *  4 segment lines so the aggregate trend stands out */
    UPROPERTY(EditAnywhere, Category = "EEG")
    FLinearColor TotalColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.85f);

    /** Line color for the COMM RELIABILITY strip */
    UPROPERTY(EditAnywhere, Category = "EEG")
    FLinearColor ReliabilityColor = FLinearColor(0.03f, 0.45f, 0.14f, 0.7f);

    /** Panel background color (default: transparent, per the OSD overlay style) */
    UPROPERTY(EditAnywhere, Category = "EEG")
    FLinearColor BackgroundColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);

    /** Size requested from the layout when the Blueprint does not force one */
    UPROPERTY(EditAnywhere, Category = "EEG")
    FVector2D PanelDesiredSize = FVector2D(260.0f, 140.0f);

    /** Strip label text color (soft white, per typical FPV OSD defaults) */
    UPROPERTY(EditAnywhere, Category = "EEG")
    FLinearColor LabelColor = FLinearColor(0.5f, 0.52f, 0.5f, 0.6f);

    /** Strip label font size */
    UPROPERTY(EditAnywhere, Category = "EEG")
    int32 LabelFontSize = 10;

    void SynchronizeProperties() override;
    void ReleaseSlateResources(bool bReleaseChildren) override;

  protected:
    auto RebuildWidget() -> TSharedRef<SWidget> override;

  private:
    /** Underlying Slate widget that does the painting */
    TSharedPtr<SEegMetricsChart> Chart;

    /** Driver assigned before the Slate widget existed; applied in RebuildWidget() */
    TWeakObjectPtr<const UEegRunnerComponent> PendingRunner;
};
