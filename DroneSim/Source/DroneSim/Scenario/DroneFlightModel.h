#pragma once

#include "CoreMinimal.h"
#include "DronePhysicsSettings.h"
#include "Math/RandomStream.h"

/** Largest rotor count any airframe layout supports (flat octo-X) */
inline constexpr int32 DroneMaxMotorCount = 8;

/**
 *  Full simulation state of the drone in SI units on UE axes (X forward, Y right, Z up).
 *  Position/velocity are world frame; angular velocity is body frame.
 */
struct FDroneFlightState
{
    FVector Position = FVector::ZeroVector;        // m
    FVector Velocity = FVector::ZeroVector;        // m/s
    FQuat Attitude = FQuat::Identity;              // body -> world
    FVector AngularVelocity = FVector::ZeroVector; // rad/s, body frame
    double MotorSpeed[DroneMaxMotorCount] = {};    // rad/s; first MotorCount entries are live
};

/**
 *  6-DOF rigid-body model of a multirotor drone (X-quad, hexa-X or flat octo-X), integrated
 *  with fixed-step RK4. Forces and torques per step: per-rotor thrust kT*w^2
 *  (air-density scaled, with ground effect near the terrain), rotor reaction torque
 *  kQ*w^2, first-order motor lag, gravity, linear+quadratic aerodynamic drag against
 *  the wind-relative airspeed, rotor gyroscopic torque, and the rigid-body Euler term
 *  -W x (I*W). Steady wind plus a first-order Markov gust act as external disturbances.
 */
class FDroneFlightModel
{
  public:
    /** Per-motor thrust geometry: lever arms in units of ArmLengthM (roll arm = body-Y
     *  offset, pitch arm = -body-X offset) and spin direction (+1 = CCW from above) */
    struct FMotorGeometry
    {
        double RollArm;
        double PitchArm;
        double SpinDir;
    };

    /** Layout table for the given rotor count: 4 = X-quad (FR, FL, BL, BR), 6 = hexa-X,
     *  8 = flat octo-X (evenly spaced arms, alternating spin - the even symmetric
     *  layouts whose mixer columns stay mutually orthogonal). Values for
     *  FDronePhysicsSettings::MotorCount, which Sanitize() snaps to 4, 6 or 8. */
    static auto MotorLayout(int32 MotorCount) -> const FMotorGeometry *;

    /** Sea-level air density the thrust/torque coefficients are referenced to */
    static constexpr double SeaLevelAirDensity = 1.225;

    /** Places the drone at rest with the given yaw; motors start at zero speed */
    void Reset(const FVector &PositionM, double YawRad);

    /** Applies new parameters; safe to call while flying (settings UI live edit) */
    void SetSettings(const FDronePhysicsSettings &InSettings);

    /** Advances one RK4 step. MotorCommands are MotorCount target rotor speeds in
     *  rad/s (clamped to [0, MotorMaxRadS]); GroundAltitudeM is the terrain Z below
     *  the drone in simulation meters, used by the ground-effect model. */
    void Advance(double DeltaTimeS, const double MotorCommands[DroneMaxMotorCount], double GroundAltitudeM);

    auto GetState() const -> const FDroneFlightState &;

    /** Mutable access for the facade to write back collision corrections */
    auto GetMutableState() -> FDroneFlightState &;

    /** Rotor speed at which the MotorCount motors exactly balance gravity */
    auto HoverMotorSpeed() const -> double;

    /** Current wind including the gust component, world frame m/s */
    auto GetWind() const -> FVector;

  private:
    /** Time derivative of every state component at the given state */
    struct FDerivative
    {
        FVector Velocity = FVector::ZeroVector;
        FVector Acceleration = FVector::ZeroVector;
        FQuat AttitudeRate = FQuat(0, 0, 0, 0);
        FVector AngularAcceleration = FVector::ZeroVector;
        double MotorAcceleration[DroneMaxMotorCount] = {};
    };

    auto Derivative(const FDroneFlightState &At, const double MotorCommands[DroneMaxMotorCount],
                    double GroundAltitudeM) const -> FDerivative;

    static auto AddScaled(const FDroneFlightState &Base, const FDerivative &Rate, double Scale) -> FDroneFlightState;

    /** Advances the first-order Markov gust; called once per Advance, not per RK4 stage */
    void UpdateGust(double DeltaTimeS);

    FDronePhysicsSettings Settings;
    FDroneFlightState State;
    FVector Gust = FVector::ZeroVector;
    FRandomStream GustRandom;
};
