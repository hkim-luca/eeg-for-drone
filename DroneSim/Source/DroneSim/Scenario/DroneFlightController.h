#pragma once

#include "CoreMinimal.h"
#include "DroneFlightModel.h"
#include "DronePhysicsSettings.h"

/**
 *  Cascaded flight controller that turns a movement direction into per-motor speed
 *  commands for FDroneFlightModel: velocity PI -> desired acceleration -> desired
 *  attitude (yaw held) and total thrust -> attitude P -> body-rate PD -> X-quad
 *  mixer. Altitude is held at the level given to Reset(); horizontal motion follows
 *  the direction passed to Compute() at the configured maximum speed.
 */
class FDroneFlightController
{
  public:
    /** Starts a run: remembers the hover altitude and the yaw to hold, clears the
     *  integrator and rate memory */
    void Reset(const FDronePhysicsSettings &InSettings, double HoldAltitudeM, double HoldYawRad);

    /** Applies new parameters mid-flight (settings UI live edit) */
    void SetSettings(const FDronePhysicsSettings &InSettings);

    /** One control step. MoveDirection is a world-frame XY unit vector (zero = hold
     *  position); OutMotorCommands receives 4 target rotor speeds in rad/s, ordered
     *  like FDroneFlightModel (FR, FL, BL, BR). */
    void Compute(const FDroneFlightState &State, const FVector &MoveDirection, double DeltaTimeS,
                 double OutMotorCommands[4]);

  private:
    FDronePhysicsSettings Settings;

    /** Altitude (sim meters) the controller keeps while no vertical input exists */
    double HoldAltitudeM = 0.0;

    /** Yaw (rad) held for the whole run; EEG actions strafe instead of turning */
    double HoldYawRad = 0.0;

    /** Velocity-loop integrator, world frame m/s^2 */
    FVector VelocityIntegral = FVector::ZeroVector;

    /** Body rate of the previous step, for the rate-loop damping term */
    FVector PreviousRate = FVector::ZeroVector;
};
