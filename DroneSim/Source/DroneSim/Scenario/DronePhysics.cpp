#include "DronePhysics.h"
#include "Components/SkeletalMeshComponent.h"
#include "DroneSimCharacter.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "ScenarioLog.h"

namespace
{
/** Ground Z reported when the trace hits nothing; deep enough to disable ground effect */
constexpr double NoGroundZM = -1.0e6;

/** Longest frame time integrated per tick; hitches beyond this are dropped, not simulated */
constexpr double MaxFrameTimeS = 0.25;

/** Yaw rate below which a turn counts as finished when settling (rad/s, ~5 deg/s) */
constexpr double SettleYawRateRadS = 0.0873;

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
        DroneCharacter->ApplyYawControlMode(Settings.bMouseYawControl);
    }

    if (const USkeletalMeshComponent *Mesh = InCharacter.GetMesh())
    {
        MeshBaseRotation = Mesh->GetRelativeRotation().Quaternion();
    }

    const double StartYawRad = FMath::DegreesToRadians(InCharacter.GetActorRotation().Yaw);
    Model.SetSettings(Settings);
    Model.Reset(InCharacter.GetActorLocation() / CmPerMeter, StartYawRad);
    HoldAltitudeM = Model.GetState().Position.Z + Settings.TakeoffAltitudeM;
    Controller.Reset(Settings, HoldAltitudeM, StartYawRad);

    MoveInput = FVector::ZeroVector;
    YawRateRadS = 0.0;
    CurrentTilt = FRotator::ZeroRotator;
    TimeAccumulator = 0.0;
    bActive = true;
}

void FDronePhysics::SetMoveInput(const FVector &WorldInput)
{
    MoveInput = WorldInput;
}

void FDronePhysics::SetYawRate(double InYawRateRadS)
{
    YawRateRadS = InYawRateRadS;
}

auto FDronePhysics::GetSettings() const -> const FDronePhysicsSettings &
{
    return Settings;
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
        DroneCharacter->ApplyYawControlMode(Settings.bMouseYawControl);
    }
}

void FDronePhysics::Tick(float DeltaTime)
{
    ACharacter *Drone = Character.Get();
    if (!bActive || Drone == nullptr || DeltaTime <= UE_KINDA_SMALL_NUMBER)
    {
        return;
    }

    // mouse-yaw mode: the control rotation is the yaw setpoint and the body turns to
    // follow it physically; otherwise the setpoint ramps at the commanded turn rate
    if (Settings.bMouseYawControl)
    {
        const APlayerController *PC = Drone->GetWorld()->GetFirstPlayerController();
        if (PC != nullptr)
        {
            Controller.SetYawTargetRad(FMath::DegreesToRadians(PC->GetControlRotation().Yaw));
        }
        Controller.SetYawRateRadS(0.0);
    }
    else
    {
        Controller.SetYawRateRadS(YawRateRadS);
    }

    const double GroundZM = TraceGroundZ();
    const double StepS = 1.0 / FMath::Max(Settings.SubstepHz, 1);
    TimeAccumulator = FMath::Min(TimeAccumulator + DeltaTime, MaxFrameTimeS);

    while (TimeAccumulator >= StepS)
    {
        double MotorCommands[DroneMaxMotorCount] = {};
        Controller.Compute(Model.GetState(), MoveInput, StepS, MotorCommands);
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
        const double ResetYawRad = FMath::DegreesToRadians(Drone->GetActorRotation().Yaw);
        Model.Reset(Drone->GetActorLocation() / CmPerMeter, ResetYawRad);
        Controller.Reset(Settings, HoldAltitudeM, ResetYawRad);
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

    // the actor root carries the simulated heading (yaw only); the roll/pitch tilt is
    // decomposed in that yaw frame and applied to the mesh on top of it
    const double YawDeg = State.Attitude.Rotator().Yaw;
    Drone.SetActorRotation(FRotator(0.0, YawDeg, 0.0));

    const FQuat YawQuat(FVector::UpVector, FMath::DegreesToRadians(YawDeg));
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

    // a turn-in-place keeps the body level and slow, so the yaw rate must also die
    // down before the drone counts as settled
    const double SpeedCmS = Model.GetState().Velocity.Size2D() * CmPerMeter;
    const double YawRateAbs = FMath::Abs(Model.GetState().AngularVelocity.Z);
    return SpeedCmS < Settings.SettleSpeedThreshold && CurrentTilt.IsNearlyZero(Settings.SettleTiltThreshold) &&
           YawRateAbs < SettleYawRateRadS;
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
