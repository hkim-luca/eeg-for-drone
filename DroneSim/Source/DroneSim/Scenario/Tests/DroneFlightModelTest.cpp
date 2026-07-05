#include "Scenario/DroneFlightModel.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
/** Settings with every disturbance disabled so the tests are deterministic */
auto QuietSettings() -> FDronePhysicsSettings
{
    FDronePhysicsSettings Settings;
    Settings.GroundEffectStrength = 0.0;
    Settings.WindXMS = 0.0;
    Settings.WindYMS = 0.0;
    Settings.GustIntensityMS = 0.0;
    return Settings;
}

constexpr double StepS = 0.001;             // 1 kHz, the default SubstepHz
constexpr double FarBelowGroundM = -1000.0; // keeps the ground-effect term inert
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDroneFlightModelHoverTest, "DroneSim.Physics.FlightModel.Hover",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
auto FDroneFlightModelHoverTest::RunTest(const FString &Parameters) -> bool
{
    FDroneFlightModel Model;
    Model.SetSettings(QuietSettings());
    Model.Reset(FVector(0.0, 0.0, 10.0), 0.0);

    // start the rotors at the exact hover speed: thrust balances gravity from t = 0
    const double Hover = Model.HoverMotorSpeed();
    const double Commands[4] = {Hover, Hover, Hover, Hover};
    for (int32 Motor = 0; Motor < 4; ++Motor)
    {
        Model.GetMutableState().MotorSpeed[Motor] = Hover;
    }

    for (int32 Step = 0; Step < 5000; ++Step) // 5 s
    {
        Model.Advance(StepS, Commands, FarBelowGroundM);
    }

    const FDroneFlightState &State = Model.GetState();
    TestTrue(TEXT("altitude held within 1 cm"), FMath::Abs(State.Position.Z - 10.0) < 0.01);
    TestTrue(TEXT("velocity stays below 1 cm/s"), State.Velocity.Size() < 0.01);
    TestTrue(TEXT("attitude stays level"), State.Attitude.Equals(FQuat::Identity, 1e-6));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDroneFlightModelAsymmetryTest, "DroneSim.Physics.FlightModel.ThrustAsymmetry",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
auto FDroneFlightModelAsymmetryTest::RunTest(const FString &Parameters) -> bool
{
    FDroneFlightModel Model;
    Model.SetSettings(QuietSettings());
    Model.Reset(FVector::ZeroVector, 0.0);

    // 5% extra speed on the left motors (FL=1, BL=2) must roll the body toward the right
    const double Hover = Model.HoverMotorSpeed();
    const double Commands[4] = {Hover, Hover * 1.05, Hover * 1.05, Hover};
    for (int32 Motor = 0; Motor < 4; ++Motor)
    {
        Model.GetMutableState().MotorSpeed[Motor] = Commands[Motor];
    }

    for (int32 Step = 0; Step < 100; ++Step) // 0.1 s
    {
        Model.Advance(StepS, Commands, FarBelowGroundM);
    }

    // higher thrust on the left lifts the left side: negative rotation around +X (forward)
    TestTrue(TEXT("rolls away from the stronger side"), Model.GetState().AngularVelocity.X < 0.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDroneFlightModelQuatNormTest, "DroneSim.Physics.FlightModel.QuaternionNorm",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
auto FDroneFlightModelQuatNormTest::RunTest(const FString &Parameters) -> bool
{
    FDroneFlightModel Model;
    Model.SetSettings(QuietSettings());
    Model.Reset(FVector::ZeroVector, 0.5);

    // deliberately unbalanced commands keep the body tumbling for the whole run
    const double Hover = Model.HoverMotorSpeed();
    const double Commands[4] = {Hover * 1.1, Hover * 0.9, Hover * 1.05, Hover * 0.95};
    for (int32 Step = 0; Step < 10000; ++Step) // 10 s
    {
        Model.Advance(StepS, Commands, FarBelowGroundM);
    }

    const double Norm = FMath::Sqrt(Model.GetState().Attitude.SizeSquared());
    TestTrue(TEXT("attitude quaternion stays unit length"), FMath::Abs(Norm - 1.0) < 1e-9);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDroneFlightModelDragDecayTest, "DroneSim.Physics.FlightModel.DragDecay",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
auto FDroneFlightModelDragDecayTest::RunTest(const FString &Parameters) -> bool
{
    FDroneFlightModel Model;
    Model.SetSettings(QuietSettings());
    Model.Reset(FVector::ZeroVector, 0.0);

    // motors off, initial horizontal speed: drag must bleed it off monotonically
    Model.GetMutableState().Velocity = FVector(5.0, 0.0, 0.0);
    const double Commands[4] = {0.0, 0.0, 0.0, 0.0};

    double PreviousSpeed = Model.GetState().Velocity.Size2D();
    for (int32 Step = 0; Step < 2000; ++Step) // 2 s
    {
        Model.Advance(StepS, Commands, FarBelowGroundM);
        const double Speed = Model.GetState().Velocity.Size2D();
        if (Speed > PreviousSpeed + 1e-12)
        {
            TestTrue(TEXT("horizontal speed decays monotonically under drag"), false);
            return true;
        }
        PreviousSpeed = Speed;
    }

    TestTrue(TEXT("drag removed a visible share of the speed"), PreviousSpeed < 4.0);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
