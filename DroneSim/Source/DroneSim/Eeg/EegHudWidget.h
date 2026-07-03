#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "EegHudWidget.generated.h"

class UEegGraphPanel;
class UEegRunnerComponent;
class UTextBlock;

/**
 *  Overlay shown while EEG running mode is active. The layout is designed in a Widget
 *  Blueprint using this class as its parent; bindable elements (all optional):
 *   - GraphPanel (EegGraphPanel), top-right: the 32-electrode signal graphs
 *   - AccuracyText (TextBlock), top-left: rolling classification accuracy from the server
 *   - ConnectionText (TextBlock): EEG server link state (green CONNECTED / red RECONNECTING)
 *   - StatusText (TextBlock): ground-truth action embedded in the simulated signal
 */
UCLASS(Abstract)
class UEegHudWidget : public UUserWidget
{
    GENERATED_BODY()

  public:
    /** Wires the graph panel and the status line to the running-mode driver */
    void SetRunner(UEegRunnerComponent *InRunner);

  protected:
    void NativeTick(const FGeometry &MyGeometry, float InDeltaTime) override;

  private:
    /** Electrode graph area; designed in the Widget Blueprint */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UEegGraphPanel> GraphPanel;

    /** Rolling classification accuracy, top-left; designed in the Widget Blueprint */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> AccuracyText;

    /** EEG server link state; designed in the Widget Blueprint */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> ConnectionText;

    /** Ground-truth action line; designed in the Widget Blueprint */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> StatusText;

    /** Running-mode driver the HUD reports on */
    TWeakObjectPtr<UEegRunnerComponent> Runner;
};
