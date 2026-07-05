#include "DroneFlightController.h"

namespace
{
/** Proportional gain of the altitude hold: climb rate per meter of altitude error [1/s] */
constexpr double AltitudeGain = 1.0;

/** Anti-windup bound of the velocity integrator, per axis [m/s^2] */
constexpr double IntegralLimit = 2.0;
} // namespace

void FDroneFlightController::Reset(const FDronePhysicsSettings &InSettings, double HoldAltitudeM_, double HoldYawRad_)
{
    Settings = InSettings;
    HoldAltitudeM = HoldAltitudeM_;
    HoldYawRad = HoldYawRad_;
    VelocityIntegral = FVector::ZeroVector;
    PreviousRate = FVector::ZeroVector;
}

void FDroneFlightController::SetSettings(const FDronePhysicsSettings &InSettings)
{
    Settings = InSettings;
}

void FDroneFlightController::Compute(const FDroneFlightState &State, const FVector &MoveDirection, double DeltaTimeS,
                                     double OutMotorCommands[4])
{
    // --- velocity setpoint: horizontal from the action, vertical from the altitude hold ---
    FVector DesiredVelocity = MoveDirection.GetSafeNormal2D() * Settings.MaxSpeedMS;
    DesiredVelocity.Z = FMath::Clamp(AltitudeGain * (HoldAltitudeM - State.Position.Z), -Settings.MaxClimbRateMS,
                                     Settings.MaxClimbRateMS);

    // --- velocity PI -> desired acceleration, tilt-limited horizontally ---
    const FVector VelocityError = DesiredVelocity - State.Velocity;
    VelocityIntegral += VelocityError * (Settings.VelIGain * DeltaTimeS);
    VelocityIntegral.X = FMath::Clamp(VelocityIntegral.X, -IntegralLimit, IntegralLimit);
    VelocityIntegral.Y = FMath::Clamp(VelocityIntegral.Y, -IntegralLimit, IntegralLimit);
    VelocityIntegral.Z = FMath::Clamp(VelocityIntegral.Z, -IntegralLimit, IntegralLimit);

    FVector Acceleration = Settings.VelPGain * VelocityError + VelocityIntegral;
    const double MaxHorizontal = FMath::Tan(FMath::DegreesToRadians(Settings.MaxTiltDeg)) * Settings.GravityMS2;
    const double Horizontal = FVector2D(Acceleration.X, Acceleration.Y).Size();
    if (Horizontal > MaxHorizontal)
    {
        const double Scale = MaxHorizontal / Horizontal;
        Acceleration.X *= Scale;
        Acceleration.Y *= Scale;
    }
    Acceleration.Z = FMath::Clamp(Acceleration.Z, -0.8 * Settings.GravityMS2, 2.0 * Settings.GravityMS2);

    // --- desired attitude: tilt that points body z along the required specific force, yaw held ---
    const FVector SpecificForce(Acceleration.X, Acceleration.Y, Acceleration.Z + Settings.GravityMS2);
    const FVector ThrustDir = SpecificForce.IsNearlyZero() ? FVector::UpVector : SpecificForce.GetUnsafeNormal();
    const FQuat DesiredAttitude =
        FQuat::FindBetweenNormals(FVector::UpVector, ThrustDir) * FQuat(FVector::UpVector, HoldYawRad);

    // --- attitude P: quaternion error as a body-frame rotation vector -> rate setpoint ---
    FQuat ErrorQuat = State.Attitude.Inverse() * DesiredAttitude;
    ErrorQuat.EnforceShortestArcWith(FQuat::Identity);
    FVector ErrorAxis;
    double ErrorAngle = 0.0;
    ErrorQuat.ToAxisAndAngle(ErrorAxis, ErrorAngle);
    const FVector DesiredRate = Settings.AttPGain * ErrorAngle * ErrorAxis;

    // --- rate PD (D on the measured angular acceleration) -> torque with Euler feedforward ---
    const FVector MeasuredAngularAccel =
        DeltaTimeS > 0.0 ? (State.AngularVelocity - PreviousRate) / DeltaTimeS : FVector::ZeroVector;
    PreviousRate = State.AngularVelocity;
    const FVector DesiredAngularAccel =
        Settings.RatePGain * (DesiredRate - State.AngularVelocity) - Settings.RateDGain * MeasuredAngularAccel;
    const FVector Inertia(Settings.InertiaXX, Settings.InertiaYY, Settings.InertiaZZ);
    const FVector Torque =
        Inertia * DesiredAngularAccel + FVector::CrossProduct(State.AngularVelocity, Inertia * State.AngularVelocity);

    // --- total thrust: keep the vertical force target despite the current tilt ---
    const double CosTilt = FMath::Max(State.Attitude.RotateVector(FVector::UpVector).Z, 0.5);
    const double TotalThrust = FMath::Max(Settings.MassKg * (Settings.GravityMS2 + Acceleration.Z) / CosTilt, 0.0);

    // --- X-quad mixer: orthogonal sign columns invert the thrust/torque mapping exactly ---
    // motor order FR, FL, BL, BR; spin dirs +1,-1,+1,-1 (see FDroneFlightModel)
    static constexpr double RollSign[4] = {+1.0, -1.0, -1.0, +1.0};
    static constexpr double PitchSign[4] = {-1.0, -1.0, +1.0, +1.0};
    static constexpr double YawSign[4] = {-1.0, +1.0, -1.0, +1.0}; // = -SpinDirection[i]
    const double Lever = Settings.ArmLengthM * FMath::Sqrt(0.5);
    const double YawScale = Settings.TorqueCoefficient / Settings.ThrustCoefficient; // N*m of yaw per N of thrust
    const double RhoRatio = Settings.AirDensity / FDroneFlightModel::SeaLevelAirDensity;
    const double ThrustToSpeedSq = 1.0 / FMath::Max(Settings.ThrustCoefficient * RhoRatio, 1e-12);

    for (int32 Motor = 0; Motor < 4; ++Motor)
    {
        const double MotorThrust = TotalThrust / 4.0 + RollSign[Motor] * Torque.X / (4.0 * Lever) +
                                   PitchSign[Motor] * Torque.Y / (4.0 * Lever) +
                                   YawSign[Motor] * Torque.Z / (4.0 * YawScale);
        OutMotorCommands[Motor] =
            FMath::Clamp(FMath::Sqrt(FMath::Max(MotorThrust, 0.0) * ThrustToSpeedSq), 0.0, Settings.MotorMaxRadS);
    }
}
