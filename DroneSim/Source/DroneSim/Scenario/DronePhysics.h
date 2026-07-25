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
 *  CharacterMovement for the telemetry HUD, the simulated heading turns the actor
 *  root (yaw only) and the body roll/pitch is applied to the skeletal mesh. The yaw
 *  setpoint is steered by SetYawRate() (turn actions) or, in mouse-yaw mode, by the
 *  control rotation read every Tick(). End() restores the movement mode saved in
 *  Begin().
 */
class FDronePhysics
{
  public:
    /** Saves the character's movement mode and starts the simulation at its current pose.
     *  MoveSpeed > 0 overrides the settings' maximum horizontal speed (cm/s). */
    void Begin(ACharacter &InCharacter, float MoveSpeed, const FDronePhysicsSettings &InSettings);

    /** World-space stick vector the controller should fly along: the direction is the
     *  travel direction and the length (clamped to 1) is the throttle fraction of the
     *  maximum speed, so both axes of a transmitter combine (zero = hold position);
     *  set every frame by the runner before Tick() */
    void SetMoveInput(const FVector &WorldInput);

    /** Yaw rate (rad/s, positive turns right) the controller should ramp its yaw
     *  setpoint at (zero = keep the current heading); set every frame by the runner
     *  before Tick(). Ignored in mouse-yaw mode, where the mouse owns the setpoint. */
    void SetYawRate(double InYawRateRadS);

    /** Active parameters (sanitized, including the Begin() speed override); the
     *  runners read the action mapping flags from here */
    auto GetSettings() const -> const FDronePhysicsSettings &;

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

    /** Stick vector requested for the current frame, world space (length 0..1) */
    FVector MoveInput = FVector::ZeroVector;

    /** Yaw rate (rad/s) requested for the current frame, forwarded to the controller */
    double YawRateRadS = 0.0;

    /** Body tilt of the last tick in the actor's yaw frame (yaw zeroed) */
    FRotator CurrentTilt = FRotator::ZeroRotator;

    /** Hover altitude (sim meters) held by the controller, captured in Begin();
     *  also the recovery hold point after a divergence reset */
    double HoldAltitudeM = 0.0;

    /** Frame time not yet consumed by whole fixed substeps */
    double TimeAccumulator = 0.0;

    /** Movement mode saved in Begin() and restored in End() */
    TEnumAsByte<EMovementMode> SavedMovementMode = MOVE_Walking;

    /** True between Begin() and End() */
    bool bActive = false;
};
