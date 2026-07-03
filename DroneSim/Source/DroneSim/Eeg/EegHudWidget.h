#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "EegHudWidget.generated.h"

class UEegGraphPanel;
class UEegRunnerComponent;
class UTextBlock;
class UVerticalBox;

/**
 *  Overlay shown while EEG running mode is active. The layout is designed in a Widget
 *  Blueprint using this class as its parent; bindable elements (all optional):
 *   - GraphPanel (EegGraphPanel), top-right: the 32-electrode signal graphs
 *   - ProbabilityBox (VerticalBox), top-left: filled by this class with one line per
 *     action (FORWARD/BACKWARD/LEFT/RIGHT/STOP %); the highest value is highlighted
 *   - ConnectionText (TextBlock): EEG server link state (green CONNECTED / red RECONNECTING)
 *  The rolling accuracy and the ground-truth action are shown on the web dashboard instead.
 */
UCLASS(Abstract)
class UEegHudWidget : public UUserWidget
{
    GENERATED_BODY()

  public:
    /** Wires the graph panel and the status line to the running-mode driver */
    void SetRunner(UEegRunnerComponent *InRunner);

    /** Font size of the generated probability lines */
    UPROPERTY(EditAnywhere, Category = "EEG")
    int32 ProbabilityFontSize = 18;

    /** Text color of the probability lines */
    UPROPERTY(EditAnywhere, Category = "EEG")
    FLinearColor ProbabilityColor = FLinearColor::White;

    /** Text color of the line with the highest probability */
    UPROPERTY(EditAnywhere, Category = "EEG")
    FLinearColor ProbabilityHighlightColor = FLinearColor(1.0f, 0.85f, 0.1f);

  protected:
    void NativeOnInitialized() override;
    void NativeTick(const FGeometry &MyGeometry, float InDeltaTime) override;

  private:
    /** Electrode graph area; designed in the Widget Blueprint */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UEegGraphPanel> GraphPanel;

    /** Container for the per-action probability lines, top-left; the lines themselves
     *  are created in C++ so the winning one can be recolored individually */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UVerticalBox> ProbabilityBox;

    /** One generated text line per action, ordered by EegConfig::ProbOrder */
    UPROPERTY()
    TArray<TObjectPtr<UTextBlock>> ProbabilityLines;

    /** EEG server link state; designed in the Widget Blueprint */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> ConnectionText;

    /** Running-mode driver the HUD reports on */
    TWeakObjectPtr<UEegRunnerComponent> Runner;
};
