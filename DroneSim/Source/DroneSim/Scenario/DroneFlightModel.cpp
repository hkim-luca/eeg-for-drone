#include "DroneFlightModel.h"
#include "HAL/PlatformTime.h"

void FDroneFlightModel::Reset(const FVector &PositionM, double YawRad)
{
    State = FDroneFlightState();
    State.Position = PositionM;
    State.Attitude = FQuat(FVector::UpVector, YawRad);
    Gust = FVector::ZeroVector;
    // gusts should differ between runs; determinism within a run comes from the fixed substep
    GustRandom.Initialize(static_cast<int32>(FPlatformTime::Cycles()));
}

void FDroneFlightModel::SetSettings(const FDronePhysicsSettings &InSettings)
{
    Settings = InSettings;
}

void FDroneFlightModel::Advance(double DeltaTimeS, const double MotorCommands[4], double GroundAltitudeM)
{
    if (DeltaTimeS <= 0.0)
    {
        return;
    }

    // the stochastic gust is held constant across the 4 RK4 stages of one step
    UpdateGust(DeltaTimeS);

    const FDerivative K1 = Derivative(State, MotorCommands, GroundAltitudeM);
    const FDerivative K2 = Derivative(AddScaled(State, K1, DeltaTimeS * 0.5), MotorCommands, GroundAltitudeM);
    const FDerivative K3 = Derivative(AddScaled(State, K2, DeltaTimeS * 0.5), MotorCommands, GroundAltitudeM);
    const FDerivative K4 = Derivative(AddScaled(State, K3, DeltaTimeS), MotorCommands, GroundAltitudeM);

    // y += h/6 * (k1 + 2*k2 + 2*k3 + k4)
    const double Sixth = DeltaTimeS / 6.0;
    State.Position += Sixth * (K1.Velocity + 2.0 * K2.Velocity + 2.0 * K3.Velocity + K4.Velocity);
    State.Velocity += Sixth * (K1.Acceleration + 2.0 * K2.Acceleration + 2.0 * K3.Acceleration + K4.Acceleration);
    State.Attitude.X +=
        Sixth * (K1.AttitudeRate.X + 2.0 * K2.AttitudeRate.X + 2.0 * K3.AttitudeRate.X + K4.AttitudeRate.X);
    State.Attitude.Y +=
        Sixth * (K1.AttitudeRate.Y + 2.0 * K2.AttitudeRate.Y + 2.0 * K3.AttitudeRate.Y + K4.AttitudeRate.Y);
    State.Attitude.Z +=
        Sixth * (K1.AttitudeRate.Z + 2.0 * K2.AttitudeRate.Z + 2.0 * K3.AttitudeRate.Z + K4.AttitudeRate.Z);
    State.Attitude.W +=
        Sixth * (K1.AttitudeRate.W + 2.0 * K2.AttitudeRate.W + 2.0 * K3.AttitudeRate.W + K4.AttitudeRate.W);
    State.Attitude.Normalize();
    State.AngularVelocity += Sixth * (K1.AngularAcceleration + 2.0 * K2.AngularAcceleration +
                                      2.0 * K3.AngularAcceleration + K4.AngularAcceleration);
    for (int32 Motor = 0; Motor < 4; ++Motor)
    {
        State.MotorSpeed[Motor] += Sixth * (K1.MotorAcceleration[Motor] + 2.0 * K2.MotorAcceleration[Motor] +
                                            2.0 * K3.MotorAcceleration[Motor] + K4.MotorAcceleration[Motor]);
        State.MotorSpeed[Motor] = FMath::Clamp(State.MotorSpeed[Motor], 0.0, Settings.MotorMaxRadS);
    }
}

auto FDroneFlightModel::GetState() const -> const FDroneFlightState &
{
    return State;
}

auto FDroneFlightModel::GetMutableState() -> FDroneFlightState &
{
    return State;
}

auto FDroneFlightModel::HoverMotorSpeed() const -> double
{
    const double RhoRatio = Settings.AirDensity / SeaLevelAirDensity;
    if (RhoRatio <= 0.0 || Settings.ThrustCoefficient <= 0.0)
    {
        return 0.0;
    }
    return FMath::Sqrt(Settings.MassKg * Settings.GravityMS2 / (4.0 * Settings.ThrustCoefficient * RhoRatio));
}

auto FDroneFlightModel::GetWind() const -> FVector
{
    return FVector(Settings.WindXMS, Settings.WindYMS, 0.0) + Gust;
}

auto FDroneFlightModel::Derivative(const FDroneFlightState &At, const double MotorCommands[4],
                                   double GroundAltitudeM) const -> FDerivative
{
    FDerivative Out;
    const double RhoRatio = Settings.AirDensity / SeaLevelAirDensity;

    // ground effect: thrust gain grows as the rotor plane nears the terrain
    const double Height = FMath::Max(At.Position.Z - GroundAltitudeM, 0.0);
    const double HeightRatio = Settings.RotorRadiusM / (4.0 * FMath::Max(Height, Settings.RotorRadiusM));
    const double GroundGain = 1.0 / (1.0 - Settings.GroundEffectStrength * FMath::Min(HeightRatio * HeightRatio, 0.25));

    double Thrust[4];
    double TotalThrust = 0.0;
    double YawTorque = 0.0;
    double GyroMomentum = 0.0;
    for (int32 Motor = 0; Motor < 4; ++Motor)
    {
        const double Command = FMath::Clamp(MotorCommands[Motor], 0.0, Settings.MotorMaxRadS);
        Out.MotorAcceleration[Motor] = (Command - At.MotorSpeed[Motor]) / Settings.MotorTimeConstantS;

        const double SpeedSq = At.MotorSpeed[Motor] * At.MotorSpeed[Motor];
        Thrust[Motor] = Settings.ThrustCoefficient * RhoRatio * GroundGain * SpeedSq;
        TotalThrust += Thrust[Motor];
        // rotor reaction torque acts on the body against the spin direction
        YawTorque -= SpinDirection[Motor] * Settings.TorqueCoefficient * RhoRatio * SpeedSq;
        GyroMomentum += SpinDirection[Motor] * At.MotorSpeed[Motor];
    }

    // thrust torque from the X-quad geometry, motor order FR, FL, BL, BR at (+-L/sqrt2, +-L/sqrt2)
    const double Lever = Settings.ArmLengthM * FMath::Sqrt(0.5);
    FVector Torque(Lever * (Thrust[0] - Thrust[1] - Thrust[2] + Thrust[3]),
                   Lever * (-Thrust[0] - Thrust[1] + Thrust[2] + Thrust[3]), YawTorque);

    // rotor gyroscopic torque: -J_r * sum(dir_i * w_i) * (W x z)
    Torque -=
        Settings.RotorInertiaKgM2 * GyroMomentum * FVector::CrossProduct(At.AngularVelocity, FVector(0.0, 0.0, 1.0));

    // rigid-body rotation (Newton-Euler): I*dW = tau - W x (I*W), diagonal inertia
    const FVector Inertia(Settings.InertiaXX, Settings.InertiaYY, Settings.InertiaZZ);
    const FVector NetTorque = Torque - FVector::CrossProduct(At.AngularVelocity, Inertia * At.AngularVelocity);
    Out.AngularAcceleration = NetTorque / Inertia;

    // translation: body-z thrust, gravity, and drag against the wind-relative airspeed
    const FVector ThrustWorld = At.Attitude.RotateVector(FVector(0.0, 0.0, TotalThrust));
    const FVector Airspeed = At.Velocity - GetWind();
    const FVector Drag = -(Settings.DragLinear * Airspeed + Settings.DragQuadratic * Airspeed.Size() * Airspeed);
    Out.Acceleration = (ThrustWorld + Drag) / Settings.MassKg + FVector(0.0, 0.0, -Settings.GravityMS2);
    Out.Velocity = At.Velocity;

    // attitude kinematics: dQ = 0.5 * Q * (W, 0)
    const FQuat Spin = At.Attitude * FQuat(At.AngularVelocity.X, At.AngularVelocity.Y, At.AngularVelocity.Z, 0.0);
    Out.AttitudeRate = FQuat(Spin.X * 0.5, Spin.Y * 0.5, Spin.Z * 0.5, Spin.W * 0.5);

    return Out;
}

auto FDroneFlightModel::AddScaled(const FDroneFlightState &Base, const FDerivative &Rate, double Scale)
    -> FDroneFlightState
{
    FDroneFlightState Out = Base;
    Out.Position += Rate.Velocity * Scale;
    Out.Velocity += Rate.Acceleration * Scale;
    Out.Attitude = FQuat(Base.Attitude.X + Rate.AttitudeRate.X * Scale, Base.Attitude.Y + Rate.AttitudeRate.Y * Scale,
                         Base.Attitude.Z + Rate.AttitudeRate.Z * Scale, Base.Attitude.W + Rate.AttitudeRate.W * Scale);
    // intermediate RK4 stages rotate vectors with this quaternion, so keep it unit length
    Out.Attitude.Normalize();
    Out.AngularVelocity += Rate.AngularAcceleration * Scale;
    for (int32 Motor = 0; Motor < 4; ++Motor)
    {
        Out.MotorSpeed[Motor] = Base.MotorSpeed[Motor] + Rate.MotorAcceleration[Motor] * Scale;
    }
    return Out;
}

void FDroneFlightModel::UpdateGust(double DeltaTimeS)
{
    if (Settings.GustIntensityMS <= 0.0)
    {
        Gust = FVector::ZeroVector;
        return;
    }

    // first-order Markov (Ornstein-Uhlenbeck) process with a 2 s correlation time;
    // vertical gusts are weaker than horizontal ones near the ground
    constexpr double CorrelationTimeS = 2.0;
    const double Decay = FMath::Min(DeltaTimeS / CorrelationTimeS, 1.0);
    const double Sigma = Settings.GustIntensityMS * FMath::Sqrt(2.0 * Decay);
    auto Gaussian = [this]() -> double {
        const double U1 = FMath::Max(static_cast<double>(GustRandom.FRand()), 1e-12);
        const double U2 = GustRandom.FRand();
        return FMath::Sqrt(-2.0 * FMath::Loge(U1)) * FMath::Cos(2.0 * UE_DOUBLE_PI * U2);
    };
    Gust.X += -Gust.X * Decay + Sigma * Gaussian();
    Gust.Y += -Gust.Y * Decay + Sigma * Gaussian();
    Gust.Z += -Gust.Z * Decay + 0.5 * Sigma * Gaussian();
}
