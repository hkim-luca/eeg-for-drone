#pragma once

#include "Components/Widget.h"
#include "CoreMinimal.h"

#include "DroneTapeGauge.generated.h"

class SDroneTapePanel;
class UDroneTelemetryComponent;

/** Which telemetry field a tape gauge instance reads */
UENUM(BlueprintType)
enum class EDroneTapeField : uint8
{
    Altitude,
    Speed
};

/**
 *  UMG widget that draws a vertical scrolling tape gauge (like a flight HUD altimeter or
 *  airspeed indicator): tick marks and numbers scroll past a fixed center pointer showing
 *  the current value. Values increase upward. One class serves both altitude and speed;
 *  set Field to pick which telemetry value it reads. Altitude reads ground-relative height
 *  (AGL, via a downward line trace), not the MSL altitude shown in the position readout.
 *  Drop it into a Widget Blueprint and size it there.
 */
UCLASS()
class UDroneTapeGauge : public UWidget
{
    GENERATED_BODY()

  public:
    /** Sets the telemetry component this gauge reads its value from */
    void SetTelemetry(const UDroneTelemetryComponent *InTelemetry);

    /** Telemetry field shown by this instance */
    UPROPERTY(EditAnywhere, Category = "Gauge")
    EDroneTapeField Field = EDroneTapeField::Altitude;

    /** Short label drawn above the center pointer (e.g. "AGL") */
    UPROPERTY(EditAnywhere, Category = "Gauge")
    FString Label = TEXT("AGL");

    /** Unit suffix appended to the current-value readout (e.g. "m") */
    UPROPERTY(EditAnywhere, Category = "Gauge")
    FString Unit = TEXT("m");

    /** Value span visible on the tape at once, centered on the current value */
    UPROPERTY(EditAnywhere, Category = "Gauge", meta = (ClampMin = "1.0"))
    float VisibleRange = 60.0f;

    /** Value spacing between tick marks */
    UPROPERTY(EditAnywhere, Category = "Gauge", meta = (ClampMin = "0.1"))
    float TickInterval = 10.0f;

    /** Panel background color; transparent by default for an OSD-style overlay (text drawn
     *  straight over the scene) - raise the alpha to restore a boxed instrument panel */
    UPROPERTY(EditAnywhere, Category = "Gauge")
    FLinearColor BackgroundColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);

    /** Tick mark and tape text color (soft white, per typical FPV OSD defaults) */
    UPROPERTY(EditAnywhere, Category = "Gauge")
    FLinearColor TapeColor = FLinearColor(0.5f, 0.52f, 0.5f, 0.92f);

    /** Center pointer and current-value readout color (muted HUD green) */
    UPROPERTY(EditAnywhere, Category = "Gauge")
    FLinearColor PointerColor = FLinearColor(0.03f, 0.45f, 0.14f, 1.0f);

    /** Tick label font size */
    UPROPERTY(EditAnywhere, Category = "Gauge")
    int32 FontSize = 12;

    /** Size requested from the layout when the Blueprint does not force one */
    UPROPERTY(EditAnywhere, Category = "Gauge")
    FVector2D PanelDesiredSize = FVector2D(100.0f, 240.0f);

    void SynchronizeProperties() override;
    void ReleaseSlateResources(bool bReleaseChildren) override;

  protected:
    auto RebuildWidget() -> TSharedRef<SWidget> override;

  private:
    /** Underlying Slate widget that does the painting */
    TSharedPtr<SDroneTapePanel> Panel;

    /** Driver assigned before the Slate widget existed; applied in RebuildWidget() */
    TWeakObjectPtr<const UDroneTelemetryComponent> PendingTelemetry;
};
