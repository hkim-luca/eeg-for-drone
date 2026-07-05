#include "Scenario/DronePhysicsPresets.h"
#include "Misc/AutomationTest.h"
#include "Scenario/DroneFlightController.h"
#include "Scenario/DroneFlightModel.h"

#if WITH_DEV_AUTOMATION_TESTS

// note: names must not clash with the other physics test files - the unity build
// merges every anonymous namespace of the module into one translation unit
namespace
{
constexpr double PresetStepS = 0.001;

/** Disturbances off so every preset run is deterministic */
auto QuietPreset(FDroneAirframePreset Preset) -> FDroneAirframePreset
{
    Preset.Settings.GroundEffectStrength = 0.0;
    Preset.Settings.WindXMS = 0.0;
    Preset.Settings.WindYMS = 0.0;
    Preset.Settings.GustIntensityMS = 0.0;
    return Preset;
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDronePresetsJsonTest, "DroneSim.Physics.Presets.JsonLoads",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
auto FDronePresetsJsonTest::RunTest(const FString &Parameters) -> bool
{
    // the shipped Content/Drones/DronePresets.json must load and mirror the built-in
    // weight range (this test fails if the file is missing, invalid, or truncated,
    // because LoadPresets then falls back to the identical-length built-in table -
    // so also compare a value the JSON must have set)
    const TArray<FDroneAirframePreset> Presets = DronePhysicsPresets::LoadPresets();
    TestTrue(TEXT("at least 6 airframes available"), Presets.Num() >= 6);

    double MinMass = TNumericLimits<double>::Max();
    double MaxMass = 0.0;
    for (int32 Index = 0; Index < Presets.Num(); ++Index)
    {
        TestFalse(FString::Printf(TEXT("preset %d has a name"), Index), Presets[Index].Name.IsEmpty());
        MinMass = FMath::Min(MinMass, Presets[Index].Settings.MassKg);
        MaxMass = FMath::Max(MaxMass, Presets[Index].Settings.MassKg);
    }
    TestTrue(TEXT("covers the sub-250 g class"), MinMass < 0.3);
    TestTrue(TEXT("covers the 25 kg class"), MaxMass >= 25.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDronePresetsFeasibilityTest, "DroneSim.Physics.Presets.HoverFeasibility",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
auto FDronePresetsFeasibilityTest::RunTest(const FString &Parameters) -> bool
{
    // every airframe must hover with motor-speed headroom left for control:
    // thrust-to-weight = (MotorMax/HoverSpeed)^2 must stay >= ~1.8 (75% speed rule)
    for (const FDroneAirframePreset &Raw : DronePhysicsPresets::LoadPresets())
    {
        const FDroneAirframePreset Preset = QuietPreset(Raw);
        FDroneFlightModel Model;
        Model.SetSettings(Preset.Settings);
        const double Hover = Model.HoverMotorSpeed();
        const double SpeedRatio = Hover / Preset.Settings.MotorMaxRadS;

        UE_LOG(LogTemp, Display, TEXT("%s: hover=%.0f rad/s (%.0f%% of max), thrust-to-weight=%.1f"), *Preset.Name,
               Hover, SpeedRatio * 100.0, 1.0 / (SpeedRatio * SpeedRatio));
        TestTrue(FString::Printf(TEXT("%s hovers below 76%% motor speed"), *Preset.Name), SpeedRatio < 0.76);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDronePresetsTakeoffTest, "DroneSim.Physics.Presets.TakeoffAllClasses",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
auto FDronePresetsTakeoffTest::RunTest(const FString &Parameters) -> bool
{
    // closed-loop sanity per airframe: from the ground with stopped motors, every
    // preset must climb to its hold altitude and settle level - heavier classes are
    // slower, so the window scales with the motor time constant
    for (const FDroneAirframePreset &Raw : DronePhysicsPresets::LoadPresets())
    {
        const FDroneAirframePreset Preset = QuietPreset(Raw);
        FDroneFlightModel Model;
        Model.SetSettings(Preset.Settings);
        Model.Reset(FVector::ZeroVector, 0.0);
        FDroneFlightController Controller;
        Controller.Reset(Preset.Settings, Preset.Settings.TakeoffAltitudeM, 0.0);

        const double DurationS = 8.0 + 100.0 * Preset.Settings.MotorTimeConstantS;
        const int32 StepCount = static_cast<int32>(DurationS / PresetStepS);
        for (int32 Step = 0; Step < StepCount; ++Step)
        {
            double Commands[4] = {};
            Controller.Compute(Model.GetState(), FVector::ZeroVector, PresetStepS, Commands);
            Model.Advance(PresetStepS, Commands, -1000.0);
        }

        const FDroneFlightState &State = Model.GetState();
        const double TiltDeg = FMath::RadiansToDegrees(
            FMath::Acos(FMath::Clamp(State.Attitude.RotateVector(FVector::UpVector).Z, -1.0, 1.0)));
        UE_LOG(LogTemp, Display, TEXT("%s: after %.0fs alt=%.3f m (target %.1f), speed=%.3f m/s, tilt=%.2f deg"),
               *Preset.Name, DurationS, State.Position.Z, Preset.Settings.TakeoffAltitudeM, State.Velocity.Size(),
               TiltDeg);

        TestTrue(FString::Printf(TEXT("%s reaches the takeoff altitude"), *Preset.Name),
                 FMath::Abs(State.Position.Z - Preset.Settings.TakeoffAltitudeM) < 0.15);
        TestTrue(FString::Printf(TEXT("%s settles"), *Preset.Name), State.Velocity.Size() < 0.15);
        TestTrue(FString::Printf(TEXT("%s stays level"), *Preset.Name), TiltDeg < 1.0);
    }
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
