#include "DronePhysics.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void FDronePhysics::Begin(ACharacter &InCharacter, float MoveSpeed, const FDronePhysicsSettings &InSettings)
{
    Settings = InSettings;
    Character = &InCharacter;

    UCharacterMovementComponent *Movement = InCharacter.GetCharacterMovement();
    SavedMaxWalkSpeed = Movement->MaxWalkSpeed;
    SavedMaxFlySpeed = Movement->MaxFlySpeed;
    SavedMaxAcceleration = Movement->MaxAcceleration;
    SavedBrakingDecelerationWalking = Movement->BrakingDecelerationWalking;
    SavedBrakingDecelerationFlying = Movement->BrakingDecelerationFlying;
    SavedBrakingFrictionFactor = Movement->BrakingFrictionFactor;
    SavedGroundFriction = Movement->GroundFriction;
    bSavedOrientRotationToMovement = Movement->bOrientRotationToMovement;

    if (MoveSpeed > 0.0f)
    {
        Movement->MaxWalkSpeed = MoveSpeed;
        Movement->MaxFlySpeed = MoveSpeed;
    }

    // inertia: ramp up with thrust acceleration, drift to a stop with braking deceleration
    Movement->MaxAcceleration = Settings.Acceleration;
    Movement->BrakingDecelerationWalking = Settings.BrakingDeceleration;
    Movement->BrakingDecelerationFlying = Settings.BrakingDeceleration;
    Movement->BrakingFrictionFactor = 0.0f;
    Movement->GroundFriction = Settings.GroundFriction;

    // a drone strafes while keeping its heading instead of turning toward the movement
    Movement->bOrientRotationToMovement = false;

    if (const USkeletalMeshComponent *Mesh = InCharacter.GetMesh())
    {
        MeshBaseRotation = Mesh->GetRelativeRotation().Quaternion();
    }

    PreviousVelocity = InCharacter.GetVelocity();
    CurrentTilt = FRotator::ZeroRotator;
    bActive = true;
}

void FDronePhysics::Tick(float DeltaTime)
{
    ACharacter *Drone = Character.Get();
    if (!bActive || Drone == nullptr || DeltaTime <= UE_KINDA_SMALL_NUMBER)
    {
        return;
    }

    // estimate the horizontal acceleration from the velocity change
    const FVector Velocity = Drone->GetVelocity();
    FVector Acceleration = (Velocity - PreviousVelocity) / DeltaTime;
    Acceleration.Z = 0.0;
    PreviousVelocity = Velocity;

    // tilt the body toward the acceleration like a quadcopter: nose down when accelerating
    // forward, banked sideways when strafing, nose up while braking
    const float ForwardRatio = FMath::Clamp(
        static_cast<float>(Acceleration.Dot(Drone->GetActorForwardVector())) / Settings.Acceleration, -1.0f, 1.0f);
    const float RightRatio = FMath::Clamp(
        static_cast<float>(Acceleration.Dot(Drone->GetActorRightVector())) / Settings.Acceleration, -1.0f, 1.0f);
    const FRotator TargetTilt(-Settings.MaxTiltAngle * ForwardRatio, 0.0f, Settings.MaxTiltAngle * RightRatio);
    CurrentTilt = FMath::RInterpTo(CurrentTilt, TargetTilt, DeltaTime, Settings.TiltInterpSpeed);

    if (USkeletalMeshComponent *Mesh = Drone->GetMesh())
    {
        Mesh->SetRelativeRotation(CurrentTilt.Quaternion() * MeshBaseRotation);
    }
}

auto FDronePhysics::IsSettled() const -> bool
{
    const ACharacter *Drone = Character.Get();
    if (!bActive || Drone == nullptr)
    {
        return true;
    }

    return Drone->GetVelocity().Size2D() < Settings.SettleSpeedThreshold &&
           CurrentTilt.IsNearlyZero(Settings.SettleTiltThreshold);
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

    UCharacterMovementComponent *Movement = Drone->GetCharacterMovement();
    Movement->MaxWalkSpeed = SavedMaxWalkSpeed;
    Movement->MaxFlySpeed = SavedMaxFlySpeed;
    Movement->MaxAcceleration = SavedMaxAcceleration;
    Movement->BrakingDecelerationWalking = SavedBrakingDecelerationWalking;
    Movement->BrakingDecelerationFlying = SavedBrakingDecelerationFlying;
    Movement->BrakingFrictionFactor = SavedBrakingFrictionFactor;
    Movement->GroundFriction = SavedGroundFriction;
    Movement->bOrientRotationToMovement = bSavedOrientRotationToMovement;

    if (USkeletalMeshComponent *Mesh = Drone->GetMesh())
    {
        Mesh->SetRelativeRotation(MeshBaseRotation);
    }
}
