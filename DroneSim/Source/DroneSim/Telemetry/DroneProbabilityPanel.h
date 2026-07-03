#pragma once

#include "Components/Widget.h"
#include "CoreMinimal.h"

#include "DroneProbabilityPanel.generated.h"

class SDroneProbabilityPanel;
class UEegRunnerComponent;

/**
 *  UMG widget that draws the per-action inference probabilities (FORWARD/BACKWARD/LEFT/
 *  RIGHT/STOP) as horizontal bars, one row per action, ordered by EegConfig::ProbOrder; the
 *  winning action's bar and label are highlighted. Drop it into a Widget Blueprint (the EEG
 *  HUD places it top-left) and size it there.
 */
UCLASS()
class UDroneProbabilityPanel : public UWidget
{
    GENERATED_BODY()

  public:
    /** Sets the running-mode driver whose inference results feed the bars */
    void SetRunner(const UEegRunnerComponent *InRunner);

    /** Panel background color; transparent by default for an OSD-style overlay (text drawn
     *  straight over the scene) - raise the alpha to restore a boxed instrument panel */
    UPROPERTY(EditAnywhere, Category = "EEG")
    FLinearColor BackgroundColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);

    /** Empty bar track color */
    UPROPERTY(EditAnywhere, Category = "EEG")
    FLinearColor TrackColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.15f);

    /** Bar fill color for non-winning actions (muted HUD green) */
    UPROPERTY(EditAnywhere, Category = "EEG")
    FLinearColor BarColor = FLinearColor(0.03f, 0.45f, 0.14f, 1.0f);

    /** Bar fill and label color for the highest-probability action (OSD amber) */
    UPROPERTY(EditAnywhere, Category = "EEG")
    FLinearColor HighlightColor = FLinearColor(0.75f, 0.35f, 0.02f, 1.0f);

    /** Label/percentage font size */
    UPROPERTY(EditAnywhere, Category = "EEG")
    int32 FontSize = 12;

    /** Size requested from the layout when the Blueprint does not force one */
    UPROPERTY(EditAnywhere, Category = "EEG")
    FVector2D PanelDesiredSize = FVector2D(240.0f, 150.0f);

    void SynchronizeProperties() override;
    void ReleaseSlateResources(bool bReleaseChildren) override;

  protected:
    auto RebuildWidget() -> TSharedRef<SWidget> override;

  private:
    /** Underlying Slate widget that does the painting */
    TSharedPtr<SDroneProbabilityPanel> Panel;

    /** Driver assigned before the Slate widget existed; applied in RebuildWidget() */
    TWeakObjectPtr<const UEegRunnerComponent> PendingRunner;
};
