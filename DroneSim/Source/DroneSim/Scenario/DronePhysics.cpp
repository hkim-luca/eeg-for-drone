#include "DronePhysics.h"
#include "Components/SkeletalMeshComponent.h"
#include "DroneSimCharacter.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ScenarioLog.h"

namespace
{
/** Ground Z reported when the trace hits nothing; deep enough to disable ground effect */
constexpr double NoGroundZM = -1.0e6;

/** Longest frame time integrated per tick; hitches beyond this are dropped, not simulated */
constexpr double MaxFrameTimeS = 0.25;

constexpr double CmPerMeter = 100.0;
} // namespace

void FDronePhysics::Begin(ACharacter &InCharacter, float MoveSpeed, const FDronePhysicsSettings &InSettings)
{
    Settings = InSettings;
    if (MoveSpeed > 0.0f)
    {
        Settings.MaxSpeedMS = MoveSpeed / CmPerMeter;
    }
    // the JSON preset file and the config ini bypass the UI clamps; a zero or NaN
    // parameter must never reach the integrator
    Settings.Sanitize();

    Character = &InCharacter;

    // the simulation owns the pawn: freeze CharacterMovement and restore it in End()
    UCharacterMovementComponent *Movement = InCharacter.GetCharacterMovement();
    SavedMovementMode = Movement->MovementMode;
    Movement->SetMovementMode(MOVE_None);
    Movement->Velocity = FVector::ZeroVector;

    // show the airframe matching the rotor layout before the rest pose is captured
    if (ADroneSimCharacter *DroneCharacter = Cast<ADroneSimCharacter>(&InCharacter))
    {
        DroneCharacter->ApplyAirframeMesh(Settings.AirframeMeshPath);
    }

    if (const USkeletalMeshComponent *Mesh = InCharacter.GetMesh())
    {
        MeshBaseRotation = Mesh->GetRelativeRotation().Quaternion();
    }

    HoldYawRad = FMath::DegreesToRadians(InCharacter.GetActorRotation().Yaw);
    Model.SetSettings(Settings);
    Model.Reset(InCharacter.GetActorLocation() / CmPerMeter, HoldYawRad);
    HoldAltitudeM = Model.GetState().Position.Z + Settings.TakeoffAltitudeM;
    Controller.Reset(Settings, HoldAltitudeM, HoldYawRad);

    MoveDirection = FVector::ZeroVector;
    CurrentTilt = FRotator::ZeroRotator;
    TimeAccumulator = 0.0;
    bActive = true;
}

void FDronePhysics::SetMoveDirection(const FVector &WorldDirection)
{
    MoveDirection = WorldDirection;
}

void FDronePhysics::UpdateSettings(const FDronePhysicsSettings &InSettings)
{
    // a live edit must not undo the MoveSpeed override Begin() applied
    const double SpeedOverride = Settings.MaxSpeedMS;
    Settings = InSettings;
    Settings.MaxSpeedMS = SpeedOverride;
    Settings.Sanitize();
    Model.SetSettings(Settings);
    Controller.SetSettings(Settings);

    // a live preset switch may change the rotor layout; swap the body to match
    if (ADroneSimCharacter *DroneCharacter = Cast<ADroneSimCharacter>(Character.Get()))
    {
        DroneCharacter->ApplyAirframeMesh(Settings.AirframeMeshPath);
    }
}

void FDronePhysics::Tick(float DeltaTime)
{
    ACharacter *Drone = Character.Get();
    if (!bActive || Drone == nullptr || DeltaTime <= UE_KINDA_SMALL_NUMBER)
    {
        return;
    }

    const double GroundZM = TraceGroundZ();
    const double StepS = 1.0 / FMath::Max(Settings.SubstepHz, 1);
    TimeAccumulator = FMath::Min(TimeAccumulator + DeltaTime, MaxFrameTimeS);

    while (TimeAccumulator >= StepS)
    {
        double MotorCommands[DroneMaxMotorCount] = {};
        Controller.Compute(Model.GetState(), MoveDirection, StepS, MotorCommands);
        Model.Advance(StepS, MotorCommands, GroundZM);
        TimeAccumulator -= StepS;
    }

    // divergence guard: a non-finite state must never reach SetActorLocation (UE would
    // ensure/crash) and cannot heal itself; restart the simulation at the actor's pose
    const FDroneFlightState &State = Model.GetState();
    if (!State.Position.ContainsNaN() && !State.Velocity.ContainsNaN() && !State.Attitude.ContainsNaN() &&
        !State.AngularVelocity.ContainsNaN())
    {
        ApplyToActor(*Drone);
    }
    else
    {
        FScenarioLog::Error(TEXT("Drone physics diverged (non-finite state); resetting at the current pose"));
        Model.Reset(Drone->GetActorLocation() / CmPerMeter, HoldYawRad);
        Controller.Reset(Settings, HoldAltitudeM, HoldYawRad);
        CurrentTilt = FRotator::ZeroRotator;
    }
}

void FDronePhysics::ApplyToActor(ACharacter &Drone)
{
    FDroneFlightState &State = Model.GetMutableState();

    // sweep so walls and terrain still block; on impact the model slides along the surface
    FHitResult Hit;
    Drone.SetActorLocation(State.Position * CmPerMeter, /*bSweep=*/true, &Hit, ETeleportType::None);
    if (Hit.bBlockingHit)
    {
        State.Position = Drone.GetActorLocation() / CmPerMeter;
        const double IntoSurface = FVector::DotProduct(State.Velocity, Hit.Normal);
        if (IntoSurface < 0.0)
        {
            State.Velocity -= IntoSurface * Hit.Normal;
        }
    }

    // telemetry and animation read CharacterMovement velocity, keep it in sync (cm/s)
    if (UCharacterMovementComponent *Movement = Drone.GetCharacterMovement())
    {
        Movement->Velocity = State.Velocity * CmPerMeter;
    }

    // body tilt in the actor's yaw frame; the actor root keeps its yaw-only rotation
    const FQuat YawQuat(FVector::UpVector, HoldYawRad);
    FRotator Tilt = (YawQuat.Inverse() * State.Attitude).Rotator();
    Tilt.Yaw = 0.0;
    CurrentTilt = Tilt;

    if (USkeletalMeshComponent *Mesh = Drone.GetMesh())
    {
        Mesh->SetRelativeRotation(CurrentTilt.Quaternion() * MeshBaseRotation);
    }
}

auto FDronePhysics::TraceGroundZ() const -> double
{
    const ACharacter *Drone = Character.Get();
    const UWorld *World = Drone != nullptr ? Drone->GetWorld() : nullptr;
    if (World == nullptr)
    {
        return NoGroundZM;
    }

    const FVector Start = Drone->GetActorLocation();
    const FVector End = Start - FVector(0.0, 0.0, 200.0 * CmPerMeter);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(DronePhysicsGround), /*bTraceComplex=*/false, Drone);
    FHitResult Hit;
    if (!World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        return NoGroundZM;
    }
    return Hit.ImpactPoint.Z / CmPerMeter;
}

auto FDronePhysics::GetCurrentTilt() const -> FRotator
{
    return CurrentTilt;
}

auto FDronePhysics::IsActive() const -> bool
{
    return bActive;
}

auto FDronePhysics::IsSettled() const -> bool
{
    const ACharacter *Drone = Character.Get();
    if (!bActive || Drone == nullptr)
    {
        return true;
    }

    const double SpeedCmS = Model.GetState().Velocity.Size2D() * CmPerMeter;
    return SpeedCmS < Settings.SettleSpeedThreshold && CurrentTilt.IsNearlyZero(Settings.SettleTiltThreshold);
}

void FDronePhysics::End()
{
    if (!bActive)
    {
        return;
    }
    bActive = false;

    ACharacter *Drone = Character.Get();
    if (Drone == nullptr)
    {
        return;
    }

    if (UCharacterMovementComponent *Movement = Drone->GetCharacterMovement())
    {
        Movement->SetMovementMode(SavedMovementMode);
        Movement->Velocity = Model.GetState().Velocity * CmPerMeter;
    }

    if (USkeletalMeshComponent *Mesh = Drone->GetMesh())
    {
        Mesh->SetRelativeRotation(MeshBaseRotation);
    }
}
