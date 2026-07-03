#include "DroneTelemetryComponent.h"
#include "GameFramework/PlayerController.h"
#include "Scenario/ScenarioLog.h"

UDroneTelemetryComponent::UDroneTelemetryComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UDroneTelemetryComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                             FActorComponentTickFunction *ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    const APawn *Pawn = GetControlledPawn();
    if (Pawn == nullptr)
    {
        if (!bWarnedNoPawn)
        {
            bWarnedNoPawn = true;
            FScenarioLog::Error(TEXT("Drone telemetry: no pawn possessed; state frozen at last known values"));
        }
        return;
    }
    bWarnedNoPawn = false;

    const FVector Location = Pawn->GetActorLocation();
    const FRotator Rotation = Pawn->GetActorRotation();
    const FVector Velocity = Pawn->GetVelocity();

    AltitudeMeters = Location.Z / 100.0;
    SpeedMps = Velocity.Size() / 100.0;
    RollDeg = Rotation.Roll;
    PitchDeg = Rotation.Pitch;
    YawDeg = Rotation.Yaw;
    HeadingDeg = FMath::Fmod(static_cast<double>(YawDeg) + 360.0, 360.0);

    // flat-earth conversion around the origin; UE units are cm with +X = north, +Y = east,
    // matching the pawn's spawn facing (Yaw 0 faces +X, i.e. north)
    constexpr double MetersPerDegreeLatitude = 111320.0;
    const double MetersPerDegreeLongitude =
        MetersPerDegreeLatitude * FMath::Cos(FMath::DegreesToRadians(OriginLatitude));

    const double NorthMeters = Location.X / 100.0;
    const double EastMeters = Location.Y / 100.0;
    Latitude = OriginLatitude + NorthMeters / MetersPerDegreeLatitude;
    Longitude = OriginLongitude + EastMeters / MetersPerDegreeLongitude;
    AltitudeMslMeters = OriginAltitude + AltitudeMeters;

    CurrentPositionMeters = FVector2D(EastMeters, NorthMeters);

    TimeSinceLastSample += DeltaTime;
    if (TimeSinceLastSample >= TrailSampleInterval)
    {
        TimeSinceLastSample = 0.0f;
        TrailPoints.Add(CurrentPositionMeters);
        if (TrailPoints.Num() > MaxTrailPoints)
        {
            TrailPoints.RemoveAt(0);
        }
    }
}

auto UDroneTelemetryComponent::GetAltitudeMeters() const -> double
{
    return AltitudeMeters;
}

auto UDroneTelemetryComponent::GetSpeedMps() const -> double
{
    return SpeedMps;
}

auto UDroneTelemetryComponent::GetHeadingDeg() const -> double
{
    return HeadingDeg;
}

auto UDroneTelemetryComponent::GetRollDeg() const -> double
{
    return RollDeg;
}

auto UDroneTelemetryComponent::GetPitchDeg() const -> double
{
    return PitchDeg;
}

auto UDroneTelemetryComponent::GetYawDeg() const -> double
{
    return YawDeg;
}

auto UDroneTelemetryComponent::GetLatitude() const -> double
{
    return Latitude;
}

auto UDroneTelemetryComponent::GetLongitude() const -> double
{
    return Longitude;
}

auto UDroneTelemetryComponent::GetAltitudeMslMeters() const -> double
{
    return AltitudeMslMeters;
}

auto UDroneTelemetryComponent::GetTrailPoints() const -> TConstArrayView<FVector2D>
{
    return TrailPoints;
}

auto UDroneTelemetryComponent::GetCurrentPositionMeters() const -> FVector2D
{
    return CurrentPositionMeters;
}

auto UDroneTelemetryComponent::GetControlledPawn() const -> APawn *
{
    const APlayerController *Controller = Cast<APlayerController>(GetOwner());
    return Controller != nullptr ? Controller->GetPawn() : nullptr;
}
