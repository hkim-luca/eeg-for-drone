#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "DroneStatusWidget.generated.h"

class UDroneMinimapWidget;
class UDroneTelemetryComponent;
class UTextBlock;

/**
 *  Overlay shown while EEG running mode is active, reporting the controlled pawn's flight
 *  state. The layout is designed in a Widget Blueprint using this class as its parent;
 *  bindable elements (all optional):
 *   - StatusText (TextBlock), bottom-left: ALT/SPD/HDG/ROLL/PITCH/YAW and WGS84 position
 *   - MinimapPanel (DroneMinimapWidget), bottom-right: north-up flight trail minimap
 */
UCLASS(Abstract)
class UDroneStatusWidget : public UUserWidget
{
    GENERATED_BODY()

  public:
    /** Wires the status text and minimap to the telemetry source */
    void SetTelemetry(UDroneTelemetryComponent *InTelemetry);

  protected:
    void NativeTick(const FGeometry &MyGeometry, float InDeltaTime) override;

  private:
    /** Multi-line flight status report; designed in the Widget Blueprint */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> StatusText;

    /** North-up minimap of the flight trail; designed in the Widget Blueprint */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UDroneMinimapWidget> MinimapPanel;

    /** Telemetry source this overlay reports on */
    TWeakObjectPtr<UDroneTelemetryComponent> Telemetry;
};
