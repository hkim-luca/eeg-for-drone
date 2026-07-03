#pragma once

#include "Components/Widget.h"
#include "CoreMinimal.h"

#include "EegGraphPanel.generated.h"

class SEegGraph;
class UEegRunnerComponent;

/**
 *  UMG widget that draws the live waveforms of all 32 simulated EEG electrodes as
 *  stacked strips, newest sample on the right. Drop it into a Widget Blueprint (the
 *  EEG HUD places it in the top-right corner) and size it there; the drawing adapts
 *  to whatever geometry the layout gives it.
 */
UCLASS()
class UEegGraphPanel : public UWidget
{
    GENERATED_BODY()

  public:
    /** Sets the running-mode driver whose simulated device feeds the graphs */
    void SetRunner(const UEegRunnerComponent *InRunner);

    /** Waveform line color */
    UPROPERTY(EditAnywhere, Category = "EEG")
    FLinearColor LineColor = FLinearColor(0.1f, 0.9f, 0.4f, 1.0f);

    /** Panel background color (default: mostly opaque dark) */
    UPROPERTY(EditAnywhere, Category = "EEG")
    FLinearColor BackgroundColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.6f);

    /** Size requested from the layout when the Blueprint does not force one */
    UPROPERTY(EditAnywhere, Category = "EEG")
    FVector2D PanelDesiredSize = FVector2D(340.0f, 520.0f);

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

    void SynchronizeProperties() override;
    void ReleaseSlateResources(bool bReleaseChildren) override;

  protected:
    auto RebuildWidget() -> TSharedRef<SWidget> override;

  private:
    /** Underlying Slate widget that does the painting */
    TSharedPtr<SEegGraph> Graph;

    /** Driver assigned before the Slate widget existed; applied in RebuildWidget() */
    TWeakObjectPtr<const UEegRunnerComponent> PendingRunner;
};
