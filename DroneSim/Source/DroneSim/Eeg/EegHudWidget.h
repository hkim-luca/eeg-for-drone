#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "EegHudWidget.generated.h"

class UDroneMinimapWidget;
class UDroneTelemetryComponent;
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
 *   - StatusBox (VerticalBox), bottom-left: filled by this class with one line per field
 *     (title, ALT/SPD/HDG/ROLL/PITCH/YAW, WGS84 lat/lon, MSL altitude)
 *   - MinimapPanel (DroneMinimapWidget), bottom-right: north-up flight trail minimap
 */
UCLASS(Abstract)
class UEegHudWidget : public UUserWidget
{
    GENERATED_BODY()

  public:
    /** Wires the graph panel and the status line to the running-mode driver */
    void SetRunner(UEegRunnerComponent *InRunner);

    /** Wires the status text and minimap to the flight telemetry source */
    void SetTelemetry(UDroneTelemetryComponent *InTelemetry);

    /** Font size of the generated probability lines */
    UPROPERTY(EditAnywhere, Category = "EEG")
    int32 ProbabilityFontSize = 18;

    /** Text color of the probability lines */
    UPROPERTY(EditAnywhere, Category = "EEG")
    FLinearColor ProbabilityColor = FLinearColor::White;

    /** Text color of the line with the highest probability */
    UPROPERTY(EditAnywhere, Category = "EEG")
    FLinearColor ProbabilityHighlightColor = FLinearColor(1.0f, 0.85f, 0.1f);

    /** Font size of the generated flight status lines */
    UPROPERTY(EditAnywhere, Category = "EEG")
    int32 StatusFontSize = 16;

    /** Text color of the flight status lines */
    UPROPERTY(EditAnywhere, Category = "EEG")
    FLinearColor StatusColor = FLinearColor::White;

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

    /** Container for the flight status lines, bottom-left; the lines themselves are
     *  created in C++, one per field (title, ALT/SPD/HDG/ROLL/PITCH/YAW, lat/lon, MSL alt) */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UVerticalBox> StatusBox;

    /** One generated text line per status field: title, ALT, SPD, HDG, ROLL, PITCH, YAW,
     *  LAT, LON, ALT(MSL), in that order */
    UPROPERTY()
    TArray<TObjectPtr<UTextBlock>> StatusLines;

    /** North-up minimap of the flight trail, bottom-right; designed in the Widget Blueprint */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UDroneMinimapWidget> MinimapPanel;

    /** Running-mode driver the HUD reports on */
    TWeakObjectPtr<UEegRunnerComponent> Runner;

    /** Flight telemetry source the status text and minimap report on */
    TWeakObjectPtr<UDroneTelemetryComponent> Telemetry;
};
