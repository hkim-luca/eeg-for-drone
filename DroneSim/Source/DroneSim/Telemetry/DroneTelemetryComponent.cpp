#include "DroneTelemetryComponent.h"
#include "Eeg/EegRunnerComponent.h"
#include "Engine/World.h"
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

    // height above whatever ground is directly below the drone; falls back to AltitudeMeters
    // (height above the world origin plane) when the trace finds no ground, e.g. over a void
    AltitudeAglMeters = AltitudeMeters;
    if (UWorld *World = GetWorld(); World != nullptr)
    {
        FHitResult GroundHit;
        FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(DroneTelemetryGroundTrace), false);
        QueryParams.AddIgnoredActor(Pawn);
        const FVector TraceEnd = Location - FVector(0.0, 0.0, static_cast<double>(GroundTraceMaxDistance));
        if (World->LineTraceSingleByChannel(GroundHit, Location, TraceEnd, ECC_Visibility, QueryParams))
        {
            AltitudeAglMeters = (Location.Z - GroundHit.Location.Z) / 100.0;
        }
    }

    HorizontalSpeedMps = Velocity.Size2D() / 100.0;
    VerticalSpeedMps = Velocity.Z / 100.0;
    FlightTimeSeconds += DeltaTime;
    YawDeg = Rotation.Yaw;

    // the pawn's actor rotation never rolls or pitches (DronePhysics only tilts the skeletal
    // mesh cosmetically), so read the real banking tilt from the EEG runner physics driver
    // when it is active; falls back to the (always level) actor rotation otherwise
    const UEegRunnerComponent *EegRunner =
        GetOwner() != nullptr ? GetOwner()->FindComponentByClass<UEegRunnerComponent>() : nullptr;
    if (EegRunner != nullptr && EegRunner->IsRunning())
    {
        const FRotator Tilt = EegRunner->GetCurrentTilt();
        RollDeg = Tilt.Roll;
        PitchDeg = Tilt.Pitch;
    }
    else
    {
        RollDeg = Rotation.Roll;
        PitchDeg = Rotation.Pitch;
    }

    // compass azimuth from true north (0 = north, 90 = east, clockwise), not the raw engine
    // Yaw: Yaw 0 faces world +X, which is east under the +X=east/+Y=south convention below,
    // so true north sits at Yaw -90 and the azimuth is offset by +90 from Yaw
    HeadingDeg = FMath::Fmod(static_cast<double>(YawDeg) + 90.0 + 360.0, 360.0);

    // flat-earth conversion around the origin; UE units are cm with +X = east, +Y = south (left-handed)
    constexpr double MetersPerDegreeLatitude = 111320.0;
    const double MetersPerDegreeLongitude =
        MetersPerDegreeLatitude * FMath::Cos(FMath::DegreesToRadians(OriginLatitude));

    const double EastMeters = Location.X / 100.0;
    const double NorthMeters = -Location.Y / 100.0;
    Latitude = OriginLatitude + NorthMeters / MetersPerDegreeLatitude;
    Longitude = OriginLongitude + EastMeters / MetersPerDegreeLongitude;
    AltitudeMslMeters = OriginAltitude + AltitudeMeters;

    CurrentPositionMeters = FVector2D(EastMeters, NorthMeters);

    // home = where the first telemetry tick found the drone (i.e. the takeoff point)
    if (!bHomeSet)
    {
        bHomeSet = true;
        HomePositionMeters = CurrentPositionMeters;
    }
    DistanceToHomeMeters = FVector2D::Distance(CurrentPositionMeters, HomePositionMeters);

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

auto UDroneTelemetryComponent::GetAltitudeAglMeters() const -> double
{
    return AltitudeAglMeters;
}

auto UDroneTelemetryComponent::GetHorizontalSpeedMps() const -> double
{
    return HorizontalSpeedMps;
}

auto UDroneTelemetryComponent::GetVerticalSpeedMps() const -> double
{
    return VerticalSpeedMps;
}

auto UDroneTelemetryComponent::GetDistanceToHomeMeters() const -> double
{
    return DistanceToHomeMeters;
}

auto UDroneTelemetryComponent::GetFlightTimeSeconds() const -> double
{
    return FlightTimeSeconds;
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

auto UDroneTelemetryComponent::GetHomePositionMeters() const -> FVector2D
{
    return HomePositionMeters;
}

auto UDroneTelemetryComponent::GetControlledPawn() const -> APawn *
{
    // hosted on the systems actor now, not the controller: track whichever pawn the
    // local player currently possesses
    const UWorld *World = GetWorld();
    const APlayerController *Controller = World != nullptr ? World->GetFirstPlayerController() : nullptr;
    return Controller != nullptr ? Controller->GetPawn() : nullptr;
}
