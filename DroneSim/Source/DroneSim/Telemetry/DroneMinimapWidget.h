#pragma once

#include "Components/Widget.h"
#include "CoreMinimal.h"

#include "DroneMinimapWidget.generated.h"

class SDroneMinimapPanel;
class UDroneTelemetryComponent;

/**
 *  UMG widget that draws a north-up minimap of the controlled pawn's flight trail as a
 *  simple grid with the recorded path and a heading-oriented drone icon. Drop it into a
 *  Widget Blueprint (the drone status overlay places it in the bottom-right corner) and
 *  size it there; the drawing adapts to whatever geometry the layout gives it.
 */
UCLASS()
class UDroneMinimapWidget : public UWidget
{
    GENERATED_BODY()

  public:
    /** Sets the telemetry component whose trail and heading feed the minimap */
    void SetTelemetry(const UDroneTelemetryComponent *InTelemetry);

    /** Grid line color */
    UPROPERTY(EditAnywhere, Category = "Minimap")
    FLinearColor GridColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.06f);

    /** Panel background color */
    UPROPERTY(EditAnywhere, Category = "Minimap")
    FLinearColor BackgroundColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.6f);

    /** Flight trail line color (muted HUD green) */
    UPROPERTY(EditAnywhere, Category = "Minimap")
    FLinearColor TrailColor = FLinearColor(0.03f, 0.45f, 0.14f, 0.6f);

    /** Drone heading icon color (OSD amber) */
    UPROPERTY(EditAnywhere, Category = "Minimap")
    FLinearColor DroneIconColor = FLinearColor(0.75f, 0.35f, 0.02f, 0.8f);

    /** Home (takeoff) point marker color (soft white) */
    UPROPERTY(EditAnywhere, Category = "Minimap")
    FLinearColor HomeColor = FLinearColor(0.5f, 0.52f, 0.5f, 0.6f);

    /** Distance in meters between grid lines */
    UPROPERTY(EditAnywhere, Category = "Minimap")
    float GridSpacingMeters = 10.0f;

    /** Minimum view half-extent in meters; keeps a stationary drone from zooming in absurdly */
    UPROPERTY(EditAnywhere, Category = "Minimap")
    float MinViewRadiusMeters = 10.0f;

    /** Size requested from the layout when the Blueprint does not force one */
    UPROPERTY(EditAnywhere, Category = "Minimap")
    FVector2D PanelDesiredSize = FVector2D(260.0f, 260.0f);

    void SynchronizeProperties() override;
    void ReleaseSlateResources(bool bReleaseChildren) override;

  protected:
    auto RebuildWidget() -> TSharedRef<SWidget> override;

  private:
    /** Underlying Slate widget that does the painting */
    TSharedPtr<SDroneMinimapPanel> Panel;

    /** Driver assigned before the Slate widget existed; applied in RebuildWidget() */
    TWeakObjectPtr<const UDroneTelemetryComponent> PendingTelemetry;
};
