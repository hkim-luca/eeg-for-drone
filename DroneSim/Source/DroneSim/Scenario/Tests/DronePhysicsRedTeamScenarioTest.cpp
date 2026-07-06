#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Scenario/DroneFlightController.h"
#include "Scenario/DroneFlightModel.h"

#if WITH_DEV_AUTOMATION_TESTS

// Red-team scenario contracts: C2 - the controller recovers from a severe attitude
// upset without falling out of the sky; C3 - physically degenerate environments that
// Sanitize() admits (vacuum, zero-g) stay finite and behave as specified. The last
// test dumps a reference trajectory for the independent scipy cross-validation
// (tools/physics-xval), the third oracle layer.
// note: RtScn prefix - unity build merges anonymous namespaces across test files
namespace
{
constexpr double RtScnStepS = 0.001; // 1 kHz, the default SubstepHz

/** Settings with every disturbance disabled so the runs are deterministic */
auto RtScnQuietSettings() -> FDronePhysicsSettings
{
    FDronePhysicsSettings Settings;
    Settings.GroundEffectStrength = 0.0;
    Settings.WindXMS = 0.0;
    Settings.WindYMS = 0.0;
    Settings.GustIntensityMS = 0.0;
    return Settings;
}

/** True when every component of the state is finite, motor speeds included */
auto RtScnIsFinite(const FDroneFlightState &State) -> bool
{
    const bool bMotorsFinite = FMath::IsFinite(State.MotorSpeed[0]) && FMath::IsFinite(State.MotorSpeed[1]) &&
                               FMath::IsFinite(State.MotorSpeed[2]) && FMath::IsFinite(State.MotorSpeed[3]);
    return bMotorsFinite && !State.Position.ContainsNaN() && !State.Velocity.ContainsNaN() &&
           !State.Attitude.ContainsNaN() && !State.AngularVelocity.ContainsNaN();
}

/** Body tilt from level, degrees */
auto RtScnTiltDeg(const FDroneFlightState &State) -> double
{
    const double CosTilt = State.Attitude.RotateVector(FVector::UpVector).Z;
    return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(CosTilt, -1.0, 1.0)));
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRedTeamUpsetRecoveryTest, "DroneSim.Physics.RedTeam.UpsetRecovery",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
auto FRedTeamUpsetRecoveryTest::RunTest(const FString &Parameters) -> bool
{
    // 150 deg roll upset at 50 m: past 90 deg the thrust points below the horizon and
    // the controller's CosTilt floor (0.5) keeps commanding it - recovery must win the
    // race against the fall. Contract: level within 5 s, losing less than 10 m.
    const FDronePhysicsSettings Settings = RtScnQuietSettings();
    FDroneFlightModel Model;
    Model.SetSettings(Settings);
    Model.Reset(FVector(0.0, 0.0, 50.0), 0.0);
    Model.GetMutableState().Attitude = FQuat(FVector::ForwardVector, FMath::DegreesToRadians(150.0));
    const double Hover = Model.HoverMotorSpeed();
    for (int32 Motor = 0; Motor < 4; ++Motor)
    {
        Model.GetMutableState().MotorSpeed[Motor] = Hover;
    }

    FDroneFlightController Controller;
    Controller.Reset(Settings, /*HoldAltitudeM=*/50.0, /*HoldYawRad=*/0.0);

    double MinZ = 50.0;
    bool bFinite = true;
    for (int32 Step = 0; Step < 5000 && bFinite; ++Step) // 5 s
    {
        double Commands[DroneMaxMotorCount] = {};
        Controller.Compute(Model.GetState(), FVector::ZeroVector, RtScnStepS, Commands);
        Model.Advance(RtScnStepS, Commands, -1000.0);
        bFinite = RtScnIsFinite(Model.GetState());
        MinZ = FMath::Min(MinZ, Model.GetState().Position.Z);
    }

    const double FinalTilt = RtScnTiltDeg(Model.GetState());
    UE_LOG(LogTemp, Display, TEXT("UpsetRecovery: final tilt %.2f deg, lowest altitude %.2f m"), FinalTilt, MinZ);
    TestTrue(TEXT("state stays finite through the upset"), bFinite);
    TestTrue(TEXT("recovered level within 5 s"), FinalTilt < 5.0);
    TestTrue(TEXT("altitude loss under 10 m"), MinZ > 40.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRedTeamDegenerateEnvTest, "DroneSim.Physics.RedTeam.DegenerateEnvironment",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
auto FRedTeamDegenerateEnvTest::RunTest(const FString &Parameters) -> bool
{
    // vacuum (AirDensity=0 passes Sanitize): zero thrust at full rotor speed - the
    // drone must fall, silently but finitely; no NaN and no fake lift
    {
        FDronePhysicsSettings Settings = RtScnQuietSettings();
        Settings.AirDensity = 0.0;
        Settings.Sanitize();
        FDroneFlightModel Model;
        Model.SetSettings(Settings);
        Model.Reset(FVector(0.0, 0.0, 50.0), 0.0);
        FDroneFlightController Controller;
        Controller.Reset(Settings, 50.0, 0.0);

        bool bFinite = true;
        for (int32 Step = 0; Step < 2000 && bFinite; ++Step) // 2 s
        {
            double Commands[DroneMaxMotorCount] = {};
            Controller.Compute(Model.GetState(), FVector::ZeroVector, RtScnStepS, Commands);
            Model.Advance(RtScnStepS, Commands, -1000.0);
            bFinite = RtScnIsFinite(Model.GetState());
        }
        UE_LOG(LogTemp, Display, TEXT("DegenerateEnvironment: vacuum altitude after 2 s = %.2f m"),
               Model.GetState().Position.Z);
        TestTrue(TEXT("vacuum: state stays finite"), bFinite);
        TestTrue(TEXT("vacuum: no thrust means the drone falls"), Model.GetState().Position.Z < 45.0);
    }

    // zero gravity (GravityMS2=0 passes Sanitize): the vertical acceleration clamp
    // collapses to [0, 0], so the altitude hold is inert - the drone must neither
    // climb nor produce a non-finite state
    {
        FDronePhysicsSettings Settings = RtScnQuietSettings();
        Settings.GravityMS2 = 0.0;
        Settings.Sanitize();
        FDroneFlightModel Model;
        Model.SetSettings(Settings);
        Model.Reset(FVector(0.0, 0.0, 5.0), 0.0);
        FDroneFlightController Controller;
        Controller.Reset(Settings, 10.0, 0.0); // hold point 5 m above: unreachable

        bool bFinite = true;
        for (int32 Step = 0; Step < 2000 && bFinite; ++Step) // 2 s
        {
            double Commands[DroneMaxMotorCount] = {};
            Controller.Compute(Model.GetState(), FVector::ZeroVector, RtScnStepS, Commands);
            Model.Advance(RtScnStepS, Commands, -1000.0);
            bFinite = RtScnIsFinite(Model.GetState());
        }
        UE_LOG(LogTemp, Display, TEXT("DegenerateEnvironment: zero-g altitude after 2 s = %.4f m"),
               Model.GetState().Position.Z);
        TestTrue(TEXT("zero-g: state stays finite"), bFinite);
        TestTrue(TEXT("zero-g: no spurious climb from the collapsed clamp"), Model.GetState().Position.Z < 5.1);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRedTeamXvalDumpTest, "DroneSim.Physics.RedTeam.CrossValidationDump",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
auto FRedTeamXvalDumpTest::RunTest(const FString &Parameters) -> bool
{
    // open-loop reference run for the scipy cross-validation: fixed asymmetric motor
    // commands (same scenario family as Rk4Convergence), sampled every 0.1 s. The
    // Python side re-integrates the identical ODE with DOP853 at 1e-12 tolerance and
    // compares each sample; params are embedded so both sides can never drift apart.
    const FDronePhysicsSettings S = RtScnQuietSettings();
    FDroneFlightModel Model;
    Model.SetSettings(S);
    Model.Reset(FVector(0.0, 0.0, 50.0), 0.2);
    const double Hover = Model.HoverMotorSpeed();
    const double Commands[4] = {Hover * 1.02, Hover * 0.99, Hover * 1.01, Hover * 0.98};
    for (int32 Motor = 0; Motor < 4; ++Motor)
    {
        Model.GetMutableState().MotorSpeed[Motor] = Hover;
    }

    FString Csv;
    Csv += FString::Printf(
        TEXT("#param,mass_kg,%.17g\n#param,arm_length_m,%.17g\n#param,inertia_xx,%.17g\n")
            TEXT("#param,inertia_yy,%.17g\n#param,inertia_zz,%.17g\n#param,motor_tau_s,%.17g\n")
                TEXT("#param,thrust_coeff,%.17g\n#param,torque_coeff,%.17g\n")
                    TEXT("#param,motor_max_rad_s,%.17g\n#param,rotor_inertia,%.17g\n")
                        TEXT("#param,gravity,%.17g\n#param,air_density,%.17g\n#param,drag_linear,%.17g\n")
                            TEXT("#param,drag_quadratic,%.17g\n#param,yaw_rad,%.17g\n#param,hover_rad_s,%.17g\n"),
        S.MassKg, S.ArmLengthM, S.InertiaXX, S.InertiaYY, S.InertiaZZ, S.MotorTimeConstantS, S.ThrustCoefficient,
        S.TorqueCoefficient, S.MotorMaxRadS, S.RotorInertiaKgM2, S.GravityMS2, S.AirDensity, S.DragLinear,
        S.DragQuadratic, 0.2, Hover);
    Csv += TEXT("t,px,py,pz,vx,vy,vz,qx,qy,qz,qw,wx,wy,wz,m0,m1,m2,m3\n");

    const auto AppendSample = [&Csv, &Model](double TimeS) {
        const FDroneFlightState &St = Model.GetState();
        Csv += FString::Printf(TEXT("%.3f,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,")
                                   TEXT("%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g\n"),
                               TimeS, St.Position.X, St.Position.Y, St.Position.Z, St.Velocity.X, St.Velocity.Y,
                               St.Velocity.Z, St.Attitude.X, St.Attitude.Y, St.Attitude.Z, St.Attitude.W,
                               St.AngularVelocity.X, St.AngularVelocity.Y, St.AngularVelocity.Z, St.MotorSpeed[0],
                               St.MotorSpeed[1], St.MotorSpeed[2], St.MotorSpeed[3]);
    };

    AppendSample(0.0);
    for (int32 Step = 1; Step <= 1000; ++Step) // 1 s at 1 kHz
    {
        Model.Advance(RtScnStepS, Commands, -1000.0);
        if (Step % 100 == 0)
        {
            AppendSample(Step * RtScnStepS);
        }
    }

    const FString OutPath = FPaths::ProjectSavedDir() / TEXT("RedTeam") / TEXT("xval_cpp.csv");
    const bool bSaved = FFileHelper::SaveStringToFile(Csv, *OutPath);
    UE_LOG(LogTemp, Display, TEXT("CrossValidationDump: %s -> %s"), bSaved ? TEXT("wrote") : TEXT("FAILED to write"),
           *OutPath);
    TestTrue(TEXT("reference trajectory written for the scipy cross-validation"), bSaved);
    TestTrue(TEXT("reference run stayed finite"), RtScnIsFinite(Model.GetState()));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
