#pragma once

#include "Components/Widget.h"
#include "CoreMinimal.h"

#include "DroneCompassTape.generated.h"

class SDroneCompassPanel;
class UDroneTelemetryComponent;

/**
 *  UMG widget drawing a flight-HUD-style horizontal heading tape: compass degrees scroll
 *  past a fixed center pointer showing the current azimuth (0 = true north), with cardinal
 *  letters at N/E/S/W. Drop it into a Widget Blueprint and size it there.
 */
UCLASS()
class UDroneCompassTape : public UWidget
{
    GENERATED_BODY()

  public:
    /** Sets the telemetry component this tape reads heading from */
    void SetTelemetry(const UDroneTelemetryComponent *InTelemetry);

    /** Degrees of heading visible across the tape width at once */
    UPROPERTY(EditAnywhere, Category = "Gauge", meta = (ClampMin = "10.0", ClampMax = "360.0"))
    float VisibleRange = 120.0f;

    /** Degrees between tick marks */
    UPROPERTY(EditAnywhere, Category = "Gauge", meta = (ClampMin = "1.0"))
    float TickInterval = 30.0f;

    /** Panel background color; transparent by default for an OSD-style overlay (text drawn
     *  straight over the scene) - raise the alpha to restore a boxed instrument panel */
    UPROPERTY(EditAnywhere, Category = "Gauge")
    FLinearColor BackgroundColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);

    /** Tick mark and tape text color (soft white, per typical FPV OSD defaults) */
    UPROPERTY(EditAnywhere, Category = "Gauge")
    FLinearColor TapeColor = FLinearColor(0.5f, 0.52f, 0.5f, 0.6f);

    /** Center pointer and current-heading readout color (muted HUD green) */
    UPROPERTY(EditAnywhere, Category = "Gauge")
    FLinearColor PointerColor = FLinearColor(0.03f, 0.45f, 0.14f, 0.8f);

    /** Tick label font size */
    UPROPERTY(EditAnywhere, Category = "Gauge")
    int32 FontSize = 12;

    /** Size requested from the layout when the Blueprint does not force one */
    UPROPERTY(EditAnywhere, Category = "Gauge")
    FVector2D PanelDesiredSize = FVector2D(260.0f, 68.0f);

    void SynchronizeProperties() override;
    void ReleaseSlateResources(bool bReleaseChildren) override;

  protected:
    auto RebuildWidget() -> TSharedRef<SWidget> override;

  private:
    /** Underlying Slate widget that does the painting */
    TSharedPtr<SDroneCompassPanel> Panel;

    /** Driver assigned before the Slate widget existed; applied in RebuildWidget() */
    TWeakObjectPtr<const UDroneTelemetryComponent> PendingTelemetry;
};
