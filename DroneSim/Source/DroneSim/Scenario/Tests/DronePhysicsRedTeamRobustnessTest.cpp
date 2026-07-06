#include "Misc/AutomationTest.h"
#include "Scenario/DroneFlightController.h"
#include "Scenario/DroneFlightModel.h"

#if WITH_DEV_AUTOMATION_TESTS

// Red-team robustness contract C1: any parameter set that passes Sanitize() must never
// drive the model into a non-finite state. These tests probe the admissible-parameter
// space adversarially; a failure here is a genuine finding, not a broken test.
// note: names carry the RtRob prefix - the unity build merges anonymous namespaces
// of every test file in the module into one translation unit
namespace
{
constexpr double RtRobStepS = 0.001; // 1 kHz, the default SubstepHz

/** Settings with every disturbance disabled so the runs are deterministic */
auto RtRobQuietSettings() -> FDronePhysicsSettings
{
    FDronePhysicsSettings Settings;
    Settings.GroundEffectStrength = 0.0;
    Settings.WindXMS = 0.0;
    Settings.WindYMS = 0.0;
    Settings.GustIntensityMS = 0.0;
    return Settings;
}

/** True when every component of the state is finite, motor speeds included
 *  (the facade divergence guard skips MotorSpeed, so the tests must not) */
auto RtRobIsFinite(const FDroneFlightState &State) -> bool
{
    const bool bMotorsFinite = FMath::IsFinite(State.MotorSpeed[0]) && FMath::IsFinite(State.MotorSpeed[1]) &&
                               FMath::IsFinite(State.MotorSpeed[2]) && FMath::IsFinite(State.MotorSpeed[3]);
    return bMotorsFinite && !State.Position.ContainsNaN() && !State.Velocity.ContainsNaN() &&
           !State.Attitude.ContainsNaN() && !State.AngularVelocity.ContainsNaN();
}

/** Runs the closed loop and returns the first step with a non-finite state, or -1 */
auto RtRobRunClosedLoop(FDroneFlightController &Controller, FDroneFlightModel &Model, const FVector &MoveDirection,
                        int32 StepCount) -> int32
{
    for (int32 Step = 0; Step < StepCount; ++Step)
    {
        double Commands[DroneMaxMotorCount] = {};
        Controller.Compute(Model.GetState(), MoveDirection, RtRobStepS, Commands);
        Model.Advance(RtRobStepS, Commands, -1000.0);
        if (!RtRobIsFinite(Model.GetState()))
        {
            return Step;
        }
    }
    return -1;
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRedTeamCornerFuzzTest, "DroneSim.Physics.RedTeam.ParameterCornerFuzz",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
auto FRedTeamCornerFuzzTest::RunTest(const FString &Parameters) -> bool
{
    // all 2^6 corners of the six parameters that set the stiffness of the dynamics;
    // every value below sits exactly on a Sanitize() clamp bound, so each combo is
    // reachable through the JSON preset file
    constexpr double MassKg[2] = {0.1, 30.0};
    constexpr double ArmM[2] = {0.05, 1.0};
    constexpr double Inertia[2] = {0.0001, 10.0};
    constexpr double ThrustK[2] = {1e-7, 1e-3};
    constexpr double MotorMax[2] = {100.0, 10000.0};
    constexpr double MotorTau[2] = {0.005, 0.5};

    int32 DivergedCount = 0;
    for (int32 Combo = 0; Combo < 64; ++Combo)
    {
        FDronePhysicsSettings Settings = RtRobQuietSettings();
        Settings.MassKg = MassKg[Combo & 1];
        Settings.ArmLengthM = ArmM[(Combo >> 1) & 1];
        Settings.InertiaXX = Inertia[(Combo >> 2) & 1];
        Settings.InertiaYY = Settings.InertiaXX;
        Settings.InertiaZZ = FMath::Min(Settings.InertiaXX * 2.0, 10.0);
        Settings.ThrustCoefficient = ThrustK[(Combo >> 3) & 1];
        Settings.MotorMaxRadS = MotorMax[(Combo >> 4) & 1];
        Settings.MotorTimeConstantS = MotorTau[(Combo >> 5) & 1];
        Settings.Sanitize();

        FDroneFlightModel Model;
        Model.SetSettings(Settings);
        Model.Reset(FVector(0.0, 0.0, 5.0), 0.0);
        FDroneFlightController Controller;
        Controller.Reset(Settings, /*HoldAltitudeM=*/5.0, /*HoldYawRad=*/0.0);

        const int32 DivergedStep = RtRobRunClosedLoop(Controller, Model, FVector::ZeroVector, 2000); // 2 s
        if (DivergedStep >= 0)
        {
            ++DivergedCount;
            AddError(FString::Printf(TEXT("non-finite state at step %d: m=%.1f arm=%.2f Ixx=%.4f kT=%.0e ")
                                         TEXT("wmax=%.0f tau=%.3f"),
                                     DivergedStep, Settings.MassKg, Settings.ArmLengthM, Settings.InertiaXX,
                                     Settings.ThrustCoefficient, Settings.MotorMaxRadS, Settings.MotorTimeConstantS));
        }
    }

    UE_LOG(LogTemp, Display, TEXT("ParameterCornerFuzz: %d of 64 sanitized corner combos diverged"), DivergedCount);
    TestTrue(TEXT("no sanitized parameter corner drives the model non-finite"), DivergedCount == 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRedTeamRateDGainTest, "DroneSim.Physics.RedTeam.RateDGainStability",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
auto FRedTeamRateDGainTest::RunTest(const FString &Parameters) -> bool
{
    // the rate-loop D term feeds back the finite-differenced measured angular
    // acceleration; the one-step-delayed signal self-amplifies at high gains even
    // through the low-pass, which is why Sanitize caps the gain at 5. Every
    // admissible value must stay stable and settle quietly.
    constexpr double Gains[] = {0.0, 0.4, 1.0, 2.0, 3.5, 5.0};

    for (const double Gain : Gains)
    {
        FDronePhysicsSettings Settings = RtRobQuietSettings();
        Settings.RateDGain = Gain;

        FDroneFlightModel Model;
        Model.SetSettings(Settings);
        Model.Reset(FVector(0.0, 0.0, 5.0), 0.0);
        // 10 deg initial roll and hover-speed rotors: a clean attitude-recovery transient
        Model.GetMutableState().Attitude = FQuat(FVector::ForwardVector, FMath::DegreesToRadians(10.0));
        const double Hover = Model.HoverMotorSpeed();
        for (int32 Motor = 0; Motor < 4; ++Motor)
        {
            Model.GetMutableState().MotorSpeed[Motor] = Hover;
        }

        FDroneFlightController Controller;
        Controller.Reset(Settings, 5.0, 0.0);

        // 2 s to recover, then 1 s in which the body must stay quiet
        const int32 DivergedStep = RtRobRunClosedLoop(Controller, Model, FVector::ZeroVector, 2000);
        double MaxRate = 0.0;
        if (DivergedStep < 0)
        {
            for (int32 Step = 0; Step < 1000; ++Step)
            {
                double Commands[DroneMaxMotorCount] = {};
                Controller.Compute(Model.GetState(), FVector::ZeroVector, RtRobStepS, Commands);
                Model.Advance(RtRobStepS, Commands, -1000.0);
                MaxRate = FMath::Max(MaxRate, Model.GetState().AngularVelocity.Size());
            }
        }

        UE_LOG(LogTemp, Display, TEXT("RateDGainStability: D=%.1f diverged_step=%d settled_max_rate=%.4f rad/s"), Gain,
               DivergedStep, MaxRate);
        TestTrue(FString::Printf(TEXT("D=%.1f keeps the state finite"), Gain), DivergedStep < 0);
        TestTrue(FString::Printf(TEXT("D=%.1f settles below 0.5 rad/s"), Gain), DivergedStep < 0 && MaxRate < 0.5);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRedTeamMixerSaturationTest, "DroneSim.Physics.RedTeam.MixerYawSaturation",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
auto FRedTeamMixerSaturationTest::RunTest(const FString &Parameters) -> bool
{
    // adversarial kQ/kT ratio (both values admissible) multiplies the mixer's yaw term
    // by ~250000x; a 180 deg yaw error then saturates all four motors. The mixer has no
    // thrust-priority reallocation, so this measures how far the altitude hold degrades.
    FDronePhysicsSettings Settings = RtRobQuietSettings();
    Settings.ThrustCoefficient = 1e-3;
    Settings.TorqueCoefficient = 1e-9;
    Settings.Sanitize();

    FDroneFlightModel Model;
    Model.SetSettings(Settings);
    Model.Reset(FVector(0.0, 0.0, 50.0), 0.0);
    const double Hover = Model.HoverMotorSpeed();
    for (int32 Motor = 0; Motor < 4; ++Motor)
    {
        Model.GetMutableState().MotorSpeed[Motor] = Hover;
    }

    FDroneFlightController Controller;
    Controller.Reset(Settings, /*HoldAltitudeM=*/50.0, /*HoldYawRad=*/UE_DOUBLE_PI);

    double MinZ = 50.0;
    double MaxZ = 50.0;
    int32 DivergedStep = -1;
    for (int32 Step = 0; Step < 3000 && DivergedStep < 0; ++Step) // 3 s
    {
        double Commands[DroneMaxMotorCount] = {};
        Controller.Compute(Model.GetState(), FVector::ZeroVector, RtRobStepS, Commands);
        Model.Advance(RtRobStepS, Commands, -1000.0);
        if (!RtRobIsFinite(Model.GetState()))
        {
            DivergedStep = Step;
            break;
        }
        MinZ = FMath::Min(MinZ, Model.GetState().Position.Z);
        MaxZ = FMath::Max(MaxZ, Model.GetState().Position.Z);
    }

    UE_LOG(LogTemp, Display, TEXT("MixerYawSaturation: diverged_step=%d altitude range [%.2f, %.2f] m"), DivergedStep,
           MinZ, MaxZ);
    TestTrue(TEXT("state stays finite through full yaw saturation"), DivergedStep < 0);
    TestTrue(TEXT("altitude held within 5 m despite the saturated mixer"), MinZ > 45.0 && MaxZ < 55.0);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
