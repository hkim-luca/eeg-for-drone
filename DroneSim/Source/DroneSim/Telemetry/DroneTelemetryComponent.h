#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"

#include "DroneTelemetryComponent.generated.h"

class APawn;

/**
 *  Tracks the controlled pawn's flight state (attitude, speed, position) every tick and
 *  derives WGS84 lat/lon plus a capped flight-path trail, for the drone status HUD shown
 *  during EEG running mode.
 */
UCLASS(ClassGroup = (Eeg), meta = (BlueprintSpawnableComponent))
class UDroneTelemetryComponent : public UActorComponent
{
    GENERATED_BODY()

  public:
    UDroneTelemetryComponent();

    void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

    /** WGS84 latitude of the UE world origin (defaults to Daejeon, matches ScenarioRunnerComponent) */
    UPROPERTY(EditAnywhere, Category = "Telemetry|Geo")
    double OriginLatitude = 36.3504;

    /** WGS84 longitude of the UE world origin (defaults to Daejeon, matches ScenarioRunnerComponent) */
    UPROPERTY(EditAnywhere, Category = "Telemetry|Geo")
    double OriginLongitude = 127.3845;

    /** Altitude of the UE world origin in meters */
    UPROPERTY(EditAnywhere, Category = "Telemetry|Geo")
    double OriginAltitude = 0.0;

    /** Seconds between flight-trail samples */
    UPROPERTY(EditAnywhere, Category = "Telemetry|Trail", meta = (ClampMin = "0.05", ForceUnits = "s"))
    float TrailSampleInterval = 1.0f;

    /** Maximum number of trail points kept; oldest points are dropped past this */
    UPROPERTY(EditAnywhere, Category = "Telemetry|Trail", meta = (ClampMin = "2"))
    int32 MaxTrailPoints = 500;

    auto GetAltitudeMeters() const -> double;
    auto GetSpeedMps() const -> double;
    auto GetHeadingDeg() const -> double;
    auto GetRollDeg() const -> double;
    auto GetPitchDeg() const -> double;
    auto GetYawDeg() const -> double;
    auto GetLatitude() const -> double;
    auto GetLongitude() const -> double;
    auto GetAltitudeMslMeters() const -> double;

    /** East/North meter offsets from the origin, oldest first */
    auto GetTrailPoints() const -> TConstArrayView<FVector2D>;

    /** Current East/North meter offset from the origin */
    auto GetCurrentPositionMeters() const -> FVector2D;

  private:
    /** Returns the pawn currently possessed by the owning player controller */
    auto GetControlledPawn() const -> APawn *;

    double AltitudeMeters = 0.0;
    double SpeedMps = 0.0;
    double HeadingDeg = 0.0;
    double RollDeg = 0.0;
    double PitchDeg = 0.0;
    double YawDeg = 0.0;
    double Latitude = 0.0;
    double Longitude = 0.0;
    double AltitudeMslMeters = 0.0;

    /** Current East/North meter offset from the origin, updated every tick */
    FVector2D CurrentPositionMeters = FVector2D::ZeroVector;

    /** Flight trail, oldest first, capped at MaxTrailPoints */
    TArray<FVector2D> TrailPoints;

    /** Time accumulated since the last trail sample was recorded */
    float TimeSinceLastSample = 0.0f;

    /** Prevents the missing-pawn error from being logged every tick */
    bool bWarnedNoPawn = false;
};
