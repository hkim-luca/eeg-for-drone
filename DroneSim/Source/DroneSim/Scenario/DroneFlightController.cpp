#include "DroneFlightController.h"

namespace
{
/** Proportional gain of the altitude hold: climb rate per meter of altitude error [1/s] */
constexpr double AltitudeGain = 1.0;

/** Anti-windup bound of the velocity integrator, per axis [m/s^2] */
constexpr double IntegralLimit = 2.0;

/** Time constant of the rate-loop D-term low-pass (~8 Hz cutoff) [s]; sized so the
 *  loop stays quiet at the largest admissible RateDGain (10) */
constexpr double DFilterTimeConstantS = 0.02;

/** Body-up Z below which the thrust fades to zero: past ~72 deg of tilt collective
 *  thrust no longer supports the vehicle and once inverted it accelerates the fall,
 *  so attitude recovery gets priority over the altitude hold */
constexpr double UprightGateCos = 0.3;
} // namespace

void FDroneFlightController::Reset(const FDronePhysicsSettings &InSettings, double HoldAltitudeM_, double HoldYawRad_)
{
    Settings = InSettings;
    HoldAltitudeM = HoldAltitudeM_;
    HoldYawRad = HoldYawRad_;
    VelocityIntegral = FVector::ZeroVector;
    PreviousRate = FVector::ZeroVector;
    FilteredAngularAccel = FVector::ZeroVector;
}

void FDroneFlightController::SetSettings(const FDronePhysicsSettings &InSettings)
{
    Settings = InSettings;
}

void FDroneFlightController::Compute(const FDroneFlightState &State, const FVector &MoveDirection, double DeltaTimeS,
                                     double OutMotorCommands[DroneMaxMotorCount])
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

    // --- rate PD (D on the low-passed measured angular acceleration) -> torque with
    // Euler feedforward; see FilteredAngularAccel for why the D term must be filtered ---
    const FVector MeasuredAngularAccel =
        DeltaTimeS > 0.0 ? (State.AngularVelocity - PreviousRate) / DeltaTimeS : FVector::ZeroVector;
    PreviousRate = State.AngularVelocity;
    const double FilterBlend = DeltaTimeS / (DeltaTimeS + DFilterTimeConstantS);
    FilteredAngularAccel += FilterBlend * (MeasuredAngularAccel - FilteredAngularAccel);
    const FVector DesiredAngularAccel =
        Settings.RatePGain * (DesiredRate - State.AngularVelocity) - Settings.RateDGain * FilteredAngularAccel;
    const FVector Inertia(Settings.InertiaXX, Settings.InertiaYY, Settings.InertiaZZ);
    const FVector Torque =
        Inertia * DesiredAngularAccel + FVector::CrossProduct(State.AngularVelocity, Inertia * State.AngularVelocity);

    // --- total thrust: keep the vertical force target despite the current tilt; the
    // upright gate fades it out toward inverted attitudes so recovery torque wins ---
    const double BodyUpZ = State.Attitude.RotateVector(FVector::UpVector).Z;
    const double CosTilt = FMath::Max(BodyUpZ, 0.5);
    const double UprightGate = FMath::Clamp(BodyUpZ / UprightGateCos, 0.0, 1.0);
    const double TotalThrust =
        UprightGate * FMath::Max(Settings.MassKg * (Settings.GravityMS2 + Acceleration.Z) / CosTilt, 0.0);

    // --- layout mixer with saturation priority: roll/pitch first, collective second,
    // yaw last - a saturated mixer gives up heading authority, never altitude ---
    // the layout's roll/pitch/spin columns are mutually orthogonal for every supported
    // rotor count, so the least-squares inversion is exact per column
    const int32 MotorCount = Settings.MotorCount;
    const FDroneFlightModel::FMotorGeometry *Layout = FDroneFlightModel::MotorLayout(MotorCount);
    const double YawScale = Settings.TorqueCoefficient / Settings.ThrustCoefficient; // N*m of yaw per N of thrust
    const double RhoRatio = Settings.AirDensity / FDroneFlightModel::SeaLevelAirDensity;
    const double ThrustToSpeedSq = 1.0 / FMath::Max(Settings.ThrustCoefficient * RhoRatio, 1e-12);
    const double MaxMotorThrust = Settings.ThrustCoefficient * RhoRatio * Settings.MotorMaxRadS * Settings.MotorMaxRadS;

    double RollNormSq = 0.0;
    double PitchNormSq = 0.0;
    for (int32 Motor = 0; Motor < MotorCount; ++Motor)
    {
        RollNormSq += Layout[Motor].RollArm * Layout[Motor].RollArm;
        PitchNormSq += Layout[Motor].PitchArm * Layout[Motor].PitchArm;
    }

    double AttThrust[DroneMaxMotorCount];
    double YawThrust[DroneMaxMotorCount];
    double MinAtt = 0.0;
    double MaxAtt = 0.0;
    for (int32 Motor = 0; Motor < MotorCount; ++Motor)
    {
        AttThrust[Motor] = Layout[Motor].RollArm * Torque.X / (Settings.ArmLengthM * RollNormSq) +
                           Layout[Motor].PitchArm * Torque.Y / (Settings.ArmLengthM * PitchNormSq);
        YawThrust[Motor] = -Layout[Motor].SpinDir * Torque.Z / (YawScale * MotorCount);
        MinAtt = FMath::Min(MinAtt, AttThrust[Motor]);
        MaxAtt = FMath::Max(MaxAtt, AttThrust[Motor]);
    }

    // roll/pitch: shrink until the demanded span fits the motor range, then slide the
    // collective inside the remaining window (the lever columns sum to zero, so the
    // span straddles zero and the window is never empty)
    const double AttSpan = MaxAtt - MinAtt;
    const double AttScale = AttSpan > MaxMotorThrust ? MaxMotorThrust / AttSpan : 1.0;
    const double BaseThrust =
        FMath::Clamp(TotalThrust / MotorCount, -AttScale * MinAtt, MaxMotorThrust - AttScale * MaxAtt);

    // yaw: only the per-motor headroom left after roll/pitch and collective
    double YawFit = 1.0;
    for (int32 Motor = 0; Motor < MotorCount; ++Motor)
    {
        const double Committed = BaseThrust + AttScale * AttThrust[Motor];
        if (YawThrust[Motor] > UE_DOUBLE_SMALL_NUMBER)
        {
            YawFit = FMath::Min(YawFit, (MaxMotorThrust - Committed) / YawThrust[Motor]);
        }
        else if (YawThrust[Motor] < -UE_DOUBLE_SMALL_NUMBER)
        {
            YawFit = FMath::Min(YawFit, Committed / -YawThrust[Motor]);
        }
    }
    YawFit = FMath::Clamp(YawFit, 0.0, 1.0);

    for (int32 Motor = 0; Motor < DroneMaxMotorCount; ++Motor)
    {
        if (Motor >= MotorCount)
        {
            OutMotorCommands[Motor] = 0.0;
            continue;
        }
        const double MotorThrust = BaseThrust + AttScale * AttThrust[Motor] + YawFit * YawThrust[Motor];
        OutMotorCommands[Motor] =
            FMath::Clamp(FMath::Sqrt(FMath::Max(MotorThrust, 0.0) * ThrustToSpeedSq), 0.0, Settings.MotorMaxRadS);
    }
}
