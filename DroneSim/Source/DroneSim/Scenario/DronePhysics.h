#pragma once

#include "CoreMinimal.h"
#include "DronePhysics.generated.h"

class ACharacter;

/** Tuning values for the drone-like movement physics during scenario playback */
USTRUCT(BlueprintType)
struct FDronePhysicsSettings
{
    GENERATED_BODY()

    /** Thrust acceleration; lower values make the drone ramp up to speed more slowly */
    UPROPERTY(EditAnywhere, Category = "Drone Physics", meta = (ClampMin = "1.0", ForceUnits = "cm/s^2"))
    float Acceleration = 5000.0f;

    /** Braking deceleration while no action is active; lower values give a longer drift */
    UPROPERTY(EditAnywhere, Category = "Drone Physics", meta = (ClampMin = "0.0", ForceUnits = "cm/s^2"))
    float BrakingDeceleration = 5000.0f;

    /** Ground friction during playback; lower values make direction changes looser, like a drone */
    UPROPERTY(EditAnywhere, Category = "Drone Physics", meta = (ClampMin = "0.0"))
    float GroundFriction = 1.5f;

    /** Maximum visual tilt of the body toward the acceleration direction */
    UPROPERTY(EditAnywhere, Category = "Drone Physics",
              meta = (ClampMin = "0.0", ClampMax = "45.0", ForceUnits = "deg"))
    float MaxTiltAngle = 15.0f;

    /** Interpolation speed of the visual tilt; higher values react faster */
    UPROPERTY(EditAnywhere, Category = "Drone Physics", meta = (ClampMin = "0.1"))
    float TiltInterpSpeed = 12.0f;

    /** Speed below which the drone counts as stopped when settling between actions */
    UPROPERTY(EditAnywhere, Category = "Drone Physics", meta = (ClampMin = "0.1", ForceUnits = "cm/s"))
    float SettleSpeedThreshold = 10.0f;

    /** Tilt below which the body counts as level when settling between actions */
    UPROPERTY(EditAnywhere, Category = "Drone Physics", meta = (ClampMin = "0.01", ForceUnits = "deg"))
    float SettleTiltThreshold = 0.5f;
};

/**
 *  Applies drone-like flight physics to a character during scenario playback:
 *  gradual thrust acceleration, momentum drift while braking, and a visual body
 *  tilt toward the acceleration direction like a real quadcopter.
 *  Begin() saves the character's movement setup and End() restores it.
 */
class FDronePhysics
{
  public:
    /** Saves the character's movement setup and applies the drone parameters.
     *  MoveSpeed > 0 overrides the max walk/fly speed (cm/s) for the run. */
    void Begin(ACharacter &InCharacter, float MoveSpeed, const FDronePhysicsSettings &InSettings);

    /** Updates the visual body tilt from the measured acceleration; call every frame while active */
    void Tick(float DeltaTime);

    /** Restores the movement setup and the body orientation saved in Begin() */
    void End();

    /** True when the drone has physically settled: nearly stopped and the body is level again */
    auto IsSettled() const -> bool;

  private:
    FDronePhysicsSettings Settings;

    /** Character being driven; weak so a destroyed pawn is handled safely */
    TWeakObjectPtr<ACharacter> Character;

    /** Mesh relative rotation before playback; the tilt is composed on top of this */
    FQuat MeshBaseRotation = FQuat::Identity;

    /** Velocity of the previous frame, used to estimate the acceleration */
    FVector PreviousVelocity = FVector::ZeroVector;

    /** Smoothed tilt currently applied to the mesh */
    FRotator CurrentTilt = FRotator::ZeroRotator;

    /** Movement values saved in Begin() and restored in End() */
    float SavedMaxWalkSpeed = 0.0f;
    float SavedMaxFlySpeed = 0.0f;
    float SavedMaxAcceleration = 0.0f;
    float SavedBrakingDecelerationWalking = 0.0f;
    float SavedBrakingDecelerationFlying = 0.0f;
    float SavedBrakingFrictionFactor = 0.0f;
    float SavedGroundFriction = 0.0f;
    bool bSavedOrientRotationToMovement = false;

    /** True between Begin() and End() */
    bool bActive = false;
};
