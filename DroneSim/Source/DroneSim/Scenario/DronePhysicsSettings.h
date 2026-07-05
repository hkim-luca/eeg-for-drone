#pragma once

#include "CoreMinimal.h"
#include "DronePhysicsSettings.generated.h"

/**
 *  Full parameter set of the 6-DOF drone simulation. All values are SI (m, kg, s, rad)
 *  unless the name says otherwise; the simulation converts to UE centimeters only at
 *  the actor boundary. Edited live by the in-game physics settings UI (P key) and
 *  mirrored to the EEG dashboard whenever a value changes.
 */
USTRUCT(BlueprintType)
struct FDronePhysicsSettings
{
    GENERATED_BODY()

    // --- Airframe (rigid body) ---

    /** Total takeoff mass */
    UPROPERTY(EditAnywhere, Category = "Airframe", meta = (ClampMin = "0.1", ClampMax = "30.0", ForceUnits = "kg"))
    double MassKg = 1.2;

    /** Distance from the body center to each motor axis */
    UPROPERTY(EditAnywhere, Category = "Airframe", meta = (ClampMin = "0.05", ClampMax = "1.0", ForceUnits = "m"))
    double ArmLengthM = 0.18;

    /** Body inertia around the roll (forward) axis */
    UPROPERTY(EditAnywhere, Category = "Airframe", meta = (ClampMin = "0.0001", ClampMax = "10.0"))
    double InertiaXX = 0.012;

    /** Body inertia around the pitch (right) axis */
    UPROPERTY(EditAnywhere, Category = "Airframe", meta = (ClampMin = "0.0001", ClampMax = "10.0"))
    double InertiaYY = 0.012;

    /** Body inertia around the yaw (up) axis */
    UPROPERTY(EditAnywhere, Category = "Airframe", meta = (ClampMin = "0.0001", ClampMax = "10.0"))
    double InertiaZZ = 0.022;

    // --- Motor / rotor (4x, X-quad layout) ---

    /** First-order motor lag; the rotor reaches a new command in roughly 3x this time */
    UPROPERTY(EditAnywhere, Category = "Motor", meta = (ClampMin = "0.005", ClampMax = "0.5", ForceUnits = "s"))
    double MotorTimeConstantS = 0.05;

    /** Rotor thrust per squared angular speed at sea-level density, T = kT * w^2 [N/(rad/s)^2] */
    UPROPERTY(EditAnywhere, Category = "Motor", meta = (ClampMin = "0.0000001", ClampMax = "0.001"))
    double ThrustCoefficient = 1.2e-6;

    /** Rotor reaction torque per squared angular speed, Q = kQ * w^2 [N*m/(rad/s)^2] */
    UPROPERTY(EditAnywhere, Category = "Motor", meta = (ClampMin = "0.000000001", ClampMax = "0.0001"))
    double TorqueCoefficient = 1.9e-8;

    /** Maximum rotor angular speed the motor can hold */
    UPROPERTY(EditAnywhere, Category = "Motor", meta = (ClampMin = "100.0", ClampMax = "10000.0"))
    double MotorMaxRadS = 2100.0;

    /** Spin inertia of one rotor, source of the gyroscopic torque [kg*m^2] */
    UPROPERTY(EditAnywhere, Category = "Motor", meta = (ClampMin = "0.0", ClampMax = "0.01"))
    double RotorInertiaKgM2 = 6e-5;

    /** Rotor blade radius, used by the ground-effect model */
    UPROPERTY(EditAnywhere, Category = "Motor", meta = (ClampMin = "0.01", ClampMax = "1.0", ForceUnits = "m"))
    double RotorRadiusM = 0.12;

    // --- Environment (aerodynamics and disturbances) ---

    /** Gravitational acceleration */
    UPROPERTY(EditAnywhere, Category = "Environment", meta = (ClampMin = "0.0", ClampMax = "30.0"))
    double GravityMS2 = 9.80665;

    /** Air density; thrust and reaction torque scale with the ratio to sea level (1.225) */
    UPROPERTY(EditAnywhere, Category = "Environment", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    double AirDensity = 1.225;

    /** Linear (rotor-induced) drag force per airspeed [N/(m/s)] */
    UPROPERTY(EditAnywhere, Category = "Environment", meta = (ClampMin = "0.0", ClampMax = "10.0"))
    double DragLinear = 0.25;

    /** Quadratic (body form) drag force per squared airspeed [N/(m/s)^2] */
    UPROPERTY(EditAnywhere, Category = "Environment", meta = (ClampMin = "0.0", ClampMax = "10.0"))
    double DragQuadratic = 0.10;

    /** Thrust gain when hovering near the ground; 0 disables the ground-effect model */
    UPROPERTY(EditAnywhere, Category = "Environment", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    double GroundEffectStrength = 0.5;

    /** Steady wind along the world X (east/forward) axis */
    UPROPERTY(EditAnywhere, Category = "Environment", meta = (ClampMin = "-30.0", ClampMax = "30.0"))
    double WindXMS = 0.0;

    /** Steady wind along the world Y (right) axis */
    UPROPERTY(EditAnywhere, Category = "Environment", meta = (ClampMin = "-30.0", ClampMax = "30.0"))
    double WindYMS = 0.0;

    /** Standard deviation of the first-order Markov gust added to the steady wind; 0 disables */
    UPROPERTY(EditAnywhere, Category = "Environment", meta = (ClampMin = "0.0", ClampMax = "15.0"))
    double GustIntensityMS = 0.0;

    // --- Control (flight controller limits and gains) ---

    /** Horizontal speed the controller aims for while an action is active;
     *  a positive MoveSpeed passed to Begin() overrides this (converted from cm/s) */
    UPROPERTY(EditAnywhere, Category = "Control", meta = (ClampMin = "0.5", ClampMax = "40.0"))
    double MaxSpeedMS = 15.0;

    /** Maximum climb/descent rate of the altitude hold */
    UPROPERTY(EditAnywhere, Category = "Control", meta = (ClampMin = "0.1", ClampMax = "10.0"))
    double MaxClimbRateMS = 3.0;

    /** Maximum body tilt the controller may request */
    UPROPERTY(EditAnywhere, Category = "Control", meta = (ClampMin = "1.0", ClampMax = "45.0", ForceUnits = "deg"))
    double MaxTiltDeg = 25.0;

    /** Hover altitude above the position where the physics began */
    UPROPERTY(EditAnywhere, Category = "Control", meta = (ClampMin = "0.0", ClampMax = "50.0", ForceUnits = "m"))
    double TakeoffAltitudeM = 1.5;

    /** Velocity-loop proportional gain: commanded acceleration per m/s of velocity error */
    UPROPERTY(EditAnywhere, Category = "Control", meta = (ClampMin = "0.1", ClampMax = "20.0"))
    double VelPGain = 2.5;

    /** Velocity-loop integral gain; removes steady-state drift against wind */
    UPROPERTY(EditAnywhere, Category = "Control", meta = (ClampMin = "0.0", ClampMax = "10.0"))
    double VelIGain = 0.4;

    /** Attitude-loop gain: commanded body rate per radian of attitude error [1/s] */
    UPROPERTY(EditAnywhere, Category = "Control", meta = (ClampMin = "0.5", ClampMax = "50.0"))
    double AttPGain = 8.0;

    /** Rate-loop proportional gain: angular acceleration per rad/s of rate error [1/s] */
    UPROPERTY(EditAnywhere, Category = "Control", meta = (ClampMin = "1.0", ClampMax = "200.0"))
    double RatePGain = 20.0;

    /** Rate-loop damping on the measured angular acceleration */
    UPROPERTY(EditAnywhere, Category = "Control", meta = (ClampMin = "0.0", ClampMax = "10.0"))
    double RateDGain = 0.4;

    /** Fixed integration substep rate of the RK4 solver */
    UPROPERTY(EditAnywhere, Category = "Control", meta = (ClampMin = "250", ClampMax = "4000"))
    int32 SubstepHz = 1000;

    // --- Settle detection (meaning unchanged from the legacy physics) ---

    /** Speed below which the drone counts as stopped when settling between actions */
    UPROPERTY(EditAnywhere, Category = "Settle", meta = (ClampMin = "0.1", ForceUnits = "cm/s"))
    float SettleSpeedThreshold = 10.0f;

    /** Tilt below which the body counts as level when settling between actions */
    UPROPERTY(EditAnywhere, Category = "Settle", meta = (ClampMin = "0.01", ForceUnits = "deg"))
    float SettleTiltThreshold = 0.5f;
};
