#include "Misc/AutomationTest.h"
#include "Scenario/DroneFlightController.h"
#include "Scenario/DroneFlightModel.h"

#if WITH_DEV_AUTOMATION_TESTS

// note: names must not clash with DroneFlightModelTest.cpp - the unity build merges
// the anonymous namespaces of both test files into one translation unit
namespace
{
/** Settings with every disturbance disabled so the tests are deterministic */
auto QuietControllerSettings() -> FDronePhysicsSettings
{
    FDronePhysicsSettings Settings;
    Settings.GroundEffectStrength = 0.0;
    Settings.WindXMS = 0.0;
    Settings.WindYMS = 0.0;
    Settings.GustIntensityMS = 0.0;
    return Settings;
}

constexpr double CtrlStepS = 0.001; // 1 kHz, the default SubstepHz

/** Runs the closed loop (controller + model) for the given duration */
void RunClosedLoop(FDroneFlightController &Controller, FDroneFlightModel &Model, const FVector &MoveDirection,
                   double DurationS)
{
    const int32 StepCount = static_cast<int32>(DurationS / CtrlStepS);
    for (int32 Step = 0; Step < StepCount; ++Step)
    {
        double Commands[4] = {};
        Controller.Compute(Model.GetState(), MoveDirection, CtrlStepS, Commands);
        Model.Advance(CtrlStepS, Commands, -1000.0);
    }
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDroneControllerHoverCommandTest, "DroneSim.Physics.Controller.HoverCommand",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
auto FDroneControllerHoverCommandTest::RunTest(const FString &Parameters) -> bool
{
    const FDronePhysicsSettings Settings = QuietControllerSettings();
    FDroneFlightModel Model;
    Model.SetSettings(Settings);
    Model.Reset(FVector(0.0, 0.0, 5.0), 0.0);

    // exactly at the hold point and at rest: the mixer must ask for pure hover speed
    FDroneFlightController Controller;
    Controller.Reset(Settings, /*HoldAltitudeM=*/5.0, /*HoldYawRad=*/0.0);

    double Commands[4] = {};
    Controller.Compute(Model.GetState(), FVector::ZeroVector, CtrlStepS, Commands);

    const double Hover = Model.HoverMotorSpeed();
    for (int32 Motor = 0; Motor < 4; ++Motor)
    {
        TestTrue(FString::Printf(TEXT("motor %d commanded near hover speed"), Motor),
                 FMath::Abs(Commands[Motor] - Hover) < Hover * 0.01);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDroneControllerTakeoffTest, "DroneSim.Physics.Controller.TakeoffHold",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
auto FDroneControllerTakeoffTest::RunTest(const FString &Parameters) -> bool
{
    const FDronePhysicsSettings Settings = QuietControllerSettings();
    FDroneFlightModel Model;
    Model.SetSettings(Settings);
    Model.Reset(FVector::ZeroVector, 0.0);

    // start on the ground with stopped motors, hold point 1.5 m up: must climb and settle
    FDroneFlightController Controller;
    Controller.Reset(Settings, Settings.TakeoffAltitudeM, 0.0);
    RunClosedLoop(Controller, Model, FVector::ZeroVector, 8.0);

    const FDroneFlightState &State = Model.GetState();
    TestTrue(TEXT("reached the takeoff altitude"), FMath::Abs(State.Position.Z - Settings.TakeoffAltitudeM) < 0.1);
    TestTrue(TEXT("settled (slow)"), State.Velocity.Size() < 0.1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDroneControllerCruiseTest, "DroneSim.Physics.Controller.ForwardCruise",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
auto FDroneControllerCruiseTest::RunTest(const FString &Parameters) -> bool
{
    FDronePhysicsSettings Settings = QuietControllerSettings();
    Settings.MaxSpeedMS = 10.0;
    FDroneFlightModel Model;
    Model.SetSettings(Settings);
    Model.Reset(FVector(0.0, 0.0, 5.0), 0.0);

    FDroneFlightController Controller;
    Controller.Reset(Settings, 5.0, 0.0);

    // command forward (+X) and watch the tilt limit the whole way
    const int32 StepCount = 6000; // 6 s
    double MaxTiltSeen = 0.0;
    for (int32 Step = 0; Step < StepCount; ++Step)
    {
        double Commands[4] = {};
        Controller.Compute(Model.GetState(), FVector::ForwardVector, CtrlStepS, Commands);
        Model.Advance(CtrlStepS, Commands, -1000.0);

        const double CosTilt = Model.GetState().Attitude.RotateVector(FVector::UpVector).Z;
        MaxTiltSeen = FMath::Max(MaxTiltSeen, FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(CosTilt, -1.0, 1.0))));
    }

    const FDroneFlightState &State = Model.GetState();
    TestTrue(TEXT("accelerated toward +X"), State.Velocity.X > 0.8 * Settings.MaxSpeedMS);
    TestTrue(TEXT("no sideways drift"), FMath::Abs(State.Velocity.Y) < 0.5);
    TestTrue(TEXT("altitude held while cruising"), FMath::Abs(State.Position.Z - 5.0) < 0.5);
    TestTrue(TEXT("tilt limit respected"), MaxTiltSeen < Settings.MaxTiltDeg + 3.0);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
