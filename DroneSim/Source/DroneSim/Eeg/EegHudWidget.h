#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "EegHudWidget.generated.h"

class UEegGraphPanel;
class UEegRunnerComponent;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEegStopRequested);

/**
 *  Overlay shown while EEG running mode is active. The layout is designed in a Widget
 *  Blueprint using this class as its parent; bindable elements (all optional):
 *   - GraphPanel (EegGraphPanel), top-right: the 32-electrode signal graphs
 *   - AccuracyText (TextBlock), top-left: rolling classification accuracy from the server
 *   - ConnectionText (TextBlock): EEG server link state (green CONNECTED / red RECONNECTING)
 *   - StatusText (TextBlock): ground-truth action and the stop shortcut hint
 *  The widget keeps keyboard focus while shown; R or Backspace stops running mode.
 */
UCLASS(Abstract)
class UEegHudWidget : public UUserWidget
{
    GENERATED_BODY()

  public:
    /** Fired when the user presses R or Backspace to leave running mode */
    UPROPERTY(BlueprintAssignable, Category = "EEG")
    FOnEegStopRequested OnStopRequested;

    /** Wires the graph panel and the status line to the running-mode driver */
    void SetRunner(UEegRunnerComponent *InRunner);

  protected:
    void NativeOnInitialized() override;
    void NativeTick(const FGeometry &MyGeometry, float InDeltaTime) override;
    auto NativeOnKeyDown(const FGeometry &InGeometry, const FKeyEvent &InKeyEvent) -> FReply override;

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

    /** Ground-truth action and stop hint; designed in the Widget Blueprint */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> StatusText;

    /** Running-mode driver the HUD reports on */
    TWeakObjectPtr<UEegRunnerComponent> Runner;
};
