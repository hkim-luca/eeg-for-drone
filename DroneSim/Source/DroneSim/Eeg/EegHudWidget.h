#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "EegHudWidget.generated.h"

class UDroneAttitudeIndicator;
class UDroneCompassTape;
class UDroneMinimapWidget;
class UDroneProbabilityPanel;
class UDroneTapeGauge;
class UDroneTelemetryComponent;
class UEegGraphPanel;
class UEegMetricsChart;
class UEegRunnerComponent;
class UTextBlock;
class UVerticalBox;

/**
 *  Overlay shown while EEG running mode is active. The layout is designed in a Widget
 *  Blueprint using this class as its parent; bindable elements (all optional):
 *   - GraphPanel (EegGraphPanel), top-right: the 32-electrode signal graphs
 *   - ProbabilityPanel (DroneProbabilityPanel), top-left: per-action inference probability bars
 *   - ConnectionText (TextBlock): EEG server link state (green CONNECTED / red RECONNECTING)
 *   - AltitudeGauge, SpeedGauge (DroneTapeGauge; set Field to Altitude/Speed respectively)
 *   - AttitudeIndicator (DroneAttitudeIndicator): roll/pitch artificial horizon
 *   - HeadingTape (DroneCompassTape): true-north heading tape
 *   - StatusBox (VerticalBox), bottom-left: filled by this class with WGS84 lat/lon and MSL
 *     altitude, one line per field (title, LAT, LON, ALT(MSL))
 *   - MinimapPanel (DroneMinimapWidget), bottom-right: north-up flight trail minimap
 *   - MetricsChart (EegMetricsChart): the same latency/reliability KPIs shown on the
 *     eeg-server dashboard, as rolling time-series strips with the live values drawn on top
 */
UCLASS(Abstract)
class UEegHudWidget : public UUserWidget
{
    GENERATED_BODY()

  public:
    /** Wires the graph panel and the status line to the running-mode driver */
    void SetRunner(UEegRunnerComponent *InRunner);

    /** Wires the status text, gauges and minimap to the flight telemetry source */
    void SetTelemetry(UDroneTelemetryComponent *InTelemetry);

    /** Font size of the generated flight status lines */
    UPROPERTY(EditAnywhere, Category = "EEG")
    int32 StatusFontSize = 16;

    /** Text color of the flight status lines (soft white, per typical FPV OSD defaults) */
    UPROPERTY(EditAnywhere, Category = "EEG")
    FLinearColor StatusColor = FLinearColor(0.5f, 0.52f, 0.5f, 0.6f);

  protected:
    void NativeOnInitialized() override;
    void NativeTick(const FGeometry &MyGeometry, float InDeltaTime) override;

  private:
    /** Electrode graph area; designed in the Widget Blueprint */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UEegGraphPanel> GraphPanel;

    /** Per-action inference probability bars, top-left; designed in the Widget Blueprint */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UDroneProbabilityPanel> ProbabilityPanel;

    /** EEG server link state; designed in the Widget Blueprint */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> ConnectionText;

    /** Altitude tape gauge; designed in the Widget Blueprint with Field = Altitude */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UDroneTapeGauge> AltitudeGauge;

    /** Speed tape gauge; designed in the Widget Blueprint with Field = Speed */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UDroneTapeGauge> SpeedGauge;

    /** Roll/pitch artificial horizon; designed in the Widget Blueprint */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UDroneAttitudeIndicator> AttitudeIndicator;

    /** True-north heading tape; designed in the Widget Blueprint */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UDroneCompassTape> HeadingTape;

    /** Container for the WGS84 position lines, bottom-left; the lines themselves are
     *  created in C++, one per field (title, LAT, LON, ALT(MSL)) */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UVerticalBox> StatusBox;

    /** One generated text line per position field: title, LAT, LON, ALT(MSL), in that order */
    UPROPERTY()
    TArray<TObjectPtr<UTextBlock>> StatusLines;

    /** North-up minimap of the flight trail, bottom-right; designed in the Widget Blueprint */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UDroneMinimapWidget> MinimapPanel;

    /** Rolling time-series chart of the dashboard KPIs, with the live values drawn on top;
     *  designed in the Widget Blueprint */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UEegMetricsChart> MetricsChart;

    /** Running-mode driver the HUD reports on */
    TWeakObjectPtr<UEegRunnerComponent> Runner;

    /** Flight telemetry source the status text, gauges and minimap report on */
    TWeakObjectPtr<UDroneTelemetryComponent> Telemetry;
};
