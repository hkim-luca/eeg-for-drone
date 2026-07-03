#pragma once

#include "Components/Widget.h"
#include "CoreMinimal.h"

#include "DroneAttitudeIndicator.generated.h"

class SDroneAttitudePanel;
class UDroneTelemetryComponent;

/**
 *  UMG widget drawing a flight-HUD-style artificial horizon: a sky/ground horizon line that
 *  rotates with roll and slides with pitch, plus a fixed aircraft symbol marking the drone's
 *  own orientation. Drop it into a Widget Blueprint and size it there.
 */
UCLASS()
class UDroneAttitudeIndicator : public UWidget
{
    GENERATED_BODY()

  public:
    /** Sets the telemetry component this indicator reads roll/pitch from */
    void SetTelemetry(const UDroneTelemetryComponent *InTelemetry);

    /** Sky (upper) color */
    UPROPERTY(EditAnywhere, Category = "Gauge")
    FLinearColor SkyColor = FLinearColor(0.15f, 0.35f, 0.65f, 0.85f);

    /** Ground (lower) color */
    UPROPERTY(EditAnywhere, Category = "Gauge")
    FLinearColor GroundColor = FLinearColor(0.35f, 0.25f, 0.1f, 0.85f);

    /** Horizon line and aircraft symbol color */
    UPROPERTY(EditAnywhere, Category = "Gauge")
    FLinearColor LineColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

    /** Pixels the horizon shifts per degree of pitch */
    UPROPERTY(EditAnywhere, Category = "Gauge", meta = (ClampMin = "0.1"))
    float PixelsPerPitchDegree = 3.0f;

    /** Size requested from the layout when the Blueprint does not force one */
    UPROPERTY(EditAnywhere, Category = "Gauge")
    FVector2D PanelDesiredSize = FVector2D(140.0f, 140.0f);

    void SynchronizeProperties() override;
    void ReleaseSlateResources(bool bReleaseChildren) override;

  protected:
    auto RebuildWidget() -> TSharedRef<SWidget> override;

  private:
    /** Underlying Slate widget that does the painting */
    TSharedPtr<SDroneAttitudePanel> Panel;

    /** Driver assigned before the Slate widget existed; applied in RebuildWidget() */
    TWeakObjectPtr<const UDroneTelemetryComponent> PendingTelemetry;
};
