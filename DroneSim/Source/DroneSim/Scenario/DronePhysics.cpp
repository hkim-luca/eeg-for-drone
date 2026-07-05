#include "DronePhysics.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

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

    Character = &InCharacter;

    // the simulation owns the pawn: freeze CharacterMovement and restore it in End()
    UCharacterMovementComponent *Movement = InCharacter.GetCharacterMovement();
    SavedMovementMode = Movement->MovementMode;
    Movement->SetMovementMode(MOVE_None);
    Movement->Velocity = FVector::ZeroVector;

    if (const USkeletalMeshComponent *Mesh = InCharacter.GetMesh())
    {
        MeshBaseRotation = Mesh->GetRelativeRotation().Quaternion();
    }

    HoldYawRad = FMath::DegreesToRadians(InCharacter.GetActorRotation().Yaw);
    Model.SetSettings(Settings);
    Model.Reset(InCharacter.GetActorLocation() / CmPerMeter, HoldYawRad);
    Controller.Reset(Settings, Model.GetState().Position.Z + Settings.TakeoffAltitudeM, HoldYawRad);

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
    Model.SetSettings(Settings);
    Controller.SetSettings(Settings);
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
        double MotorCommands[4] = {};
        Controller.Compute(Model.GetState(), MoveDirection, StepS, MotorCommands);
        Model.Advance(StepS, MotorCommands, GroundZM);
        TimeAccumulator -= StepS;
    }

    ApplyToActor(*Drone);
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
