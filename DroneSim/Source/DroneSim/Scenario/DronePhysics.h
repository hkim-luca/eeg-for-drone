#pragma once

#include "CoreMinimal.h"
#include "DroneFlightController.h"
#include "DroneFlightModel.h"
#include "DronePhysicsSettings.h"

class ACharacter;

/**
 *  Drives a character as a physically simulated X-quad drone. Begin() takes the pawn
 *  out of CharacterMovement (MOVE_None) and hands it to the 6-DOF model + cascaded
 *  controller pair, integrated at a fixed substep; every Tick() the simulated pose is
 *  swept onto the actor (collisions slide), the simulated velocity is mirrored into
 *  CharacterMovement for the telemetry HUD, and the body roll/pitch is applied to the
 *  skeletal mesh (the actor root itself never rolls or pitches, only yaw). End()
 *  restores the movement mode saved in Begin().
 */
class FDronePhysics
{
  public:
    /** Saves the character's movement mode and starts the simulation at its current pose.
     *  MoveSpeed > 0 overrides the settings' maximum horizontal speed (cm/s). */
    void Begin(ACharacter &InCharacter, float MoveSpeed, const FDronePhysicsSettings &InSettings);

    /** World-space movement direction the controller should fly toward (zero = hold
     *  position); set every frame by the runner before Tick() */
    void SetMoveDirection(const FVector &WorldDirection);

    /** Applies new parameters mid-flight (settings UI live edit) */
    void UpdateSettings(const FDronePhysicsSettings &InSettings);

    /** Integrates the simulation up to the frame time and applies the result to the actor */
    void Tick(float DeltaTime);

    /** Restores the movement mode and the mesh orientation saved in Begin() */
    void End();

    /** True when the drone has physically settled: nearly stopped and the body is level */
    auto IsSettled() const -> bool;

    /** True between Begin() and End() */
    auto IsActive() const -> bool;

    /** Current body tilt (banking roll, nose pitch) in the actor's yaw frame; applied to
     *  the mesh only, so HUD attitude instruments read it from here */
    auto GetCurrentTilt() const -> FRotator;

  private:
    /** Terrain Z below the drone in simulation meters; very deep when nothing is hit,
     *  which keeps the ground-effect term inert */
    auto TraceGroundZ() const -> double;

    /** Sweeps the actor to the simulated position and slides the model along collisions */
    void ApplyToActor(ACharacter &Drone);

    FDronePhysicsSettings Settings;
    FDroneFlightModel Model;
    FDroneFlightController Controller;

    /** Character being driven; weak so a destroyed pawn is handled safely */
    TWeakObjectPtr<ACharacter> Character;

    /** Mesh relative rotation before the run; the body tilt is composed on top of this */
    FQuat MeshBaseRotation = FQuat::Identity;

    /** Direction requested for the current frame, world space */
    FVector MoveDirection = FVector::ZeroVector;

    /** Body tilt of the last tick in the actor's yaw frame (yaw zeroed) */
    FRotator CurrentTilt = FRotator::ZeroRotator;

    /** Yaw (rad) held for the whole run, captured from the actor in Begin() */
    double HoldYawRad = 0.0;

    /** Frame time not yet consumed by whole fixed substeps */
    double TimeAccumulator = 0.0;

    /** Movement mode saved in Begin() and restored in End() */
    TEnumAsByte<EMovementMode> SavedMovementMode = MOVE_Walking;

    /** True between Begin() and End() */
    bool bActive = false;
};
