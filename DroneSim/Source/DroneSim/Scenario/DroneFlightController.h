#pragma once

#include "CoreMinimal.h"
#include "DroneFlightModel.h"
#include "DronePhysicsSettings.h"

/**
 *  Cascaded flight controller that turns a two-axis stick input into per-motor speed
 *  commands for FDroneFlightModel: velocity PI -> desired acceleration -> desired
 *  attitude and total thrust -> attitude P -> body-rate PD -> X-quad mixer.
 *  Altitude is held at the level given to Reset(); horizontal motion follows the
 *  stick vector passed to Compute(), whose length throttles the configured maximum
 *  speed. Yaw tracks a steerable setpoint: it starts at the yaw given to Reset()
 *  and moves via SetYawTargetRad() (mouse) or by integrating the SetYawRateRadS()
 *  command (turn-left/turn-right actions). */
class FDroneFlightController
{
  public:
    /** Starts a run: remembers the hover altitude and the initial yaw setpoint,
     *  clears the yaw-rate command, the integrator and the rate memory */
    void Reset(const FDronePhysicsSettings &InSettings, double HoldAltitudeM, double HoldYawRad);

    /** Applies new parameters mid-flight (settings UI live edit) */
    void SetSettings(const FDronePhysicsSettings &InSettings);

    /** Moves the yaw setpoint directly (mouse yaw control); the attitude loop turns
     *  the body toward it through the normal dynamics */
    void SetYawTargetRad(double InYawTargetRad);

    /** Yaw rate integrated into the setpoint every Compute() step (positive turns
     *  right); zero stops the setpoint where it is */
    void SetYawRateRadS(double InYawRateRadS);

    /** One control step. MoveInput is a world-frame XY stick vector: its direction is
     *  the travel direction and its length (clamped to 1) is the throttle fraction of
     *  the maximum speed, so two half-deflected axes combine into one diagonal at
     *  partial speed (zero = hold position). OutMotorCommands receives MotorCount
     *  target rotor speeds in rad/s ordered like FDroneFlightModel::MotorLayout;
     *  unused entries are zeroed. */
    void Compute(const FDroneFlightState &State, const FVector &MoveInput, double DeltaTimeS,
                 double OutMotorCommands[DroneMaxMotorCount]);

  private:
    FDronePhysicsSettings Settings;

    /** Altitude (sim meters) the controller keeps while no vertical input exists */
    double HoldAltitudeM = 0.0;

    /** Yaw setpoint (rad); starts at the yaw given to Reset() and is steered by
     *  SetYawTargetRad() or the integrated SetYawRateRadS() command */
    double YawTargetRad = 0.0;

    /** Yaw rate command (rad/s) integrated into YawTargetRad; zero holds it */
    double YawRateRadS = 0.0;

    /** Velocity-loop integrator, world frame m/s^2 */
    FVector VelocityIntegral = FVector::ZeroVector;

    /** Body rate of the previous step, for the rate-loop damping term */
    FVector PreviousRate = FVector::ZeroVector;

    /** Low-pass filtered measured angular acceleration; the raw one-step finite
     *  difference feeds its own output back with one step of delay and self-amplifies
     *  at damping gains above ~1, so the D term must act on this instead */
    FVector FilteredAngularAccel = FVector::ZeroVector;
};
