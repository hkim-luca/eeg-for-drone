#include "DronePhysicsPresets.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ScenarioLog.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
/** JSON key -> settings member; keys follow the proto field names for consistency */
struct FPresetField
{
    const TCHAR *Key;
    double FDronePhysicsSettings::*DoubleField;
    float FDronePhysicsSettings::*FloatField;
    int32 FDronePhysicsSettings::*IntField;
};

// clang-format off
const FPresetField PresetFields[] = {
    {TEXT("mass_kg"),                &FDronePhysicsSettings::MassKg, nullptr, nullptr},
    {TEXT("arm_length_m"),           &FDronePhysicsSettings::ArmLengthM, nullptr, nullptr},
    {TEXT("inertia_xx"),             &FDronePhysicsSettings::InertiaXX, nullptr, nullptr},
    {TEXT("inertia_yy"),             &FDronePhysicsSettings::InertiaYY, nullptr, nullptr},
    {TEXT("inertia_zz"),             &FDronePhysicsSettings::InertiaZZ, nullptr, nullptr},
    {TEXT("motor_time_constant_s"),  &FDronePhysicsSettings::MotorTimeConstantS, nullptr, nullptr},
    {TEXT("thrust_coefficient"),     &FDronePhysicsSettings::ThrustCoefficient, nullptr, nullptr},
    {TEXT("torque_coefficient"),     &FDronePhysicsSettings::TorqueCoefficient, nullptr, nullptr},
    {TEXT("motor_max_rad_s"),        &FDronePhysicsSettings::MotorMaxRadS, nullptr, nullptr},
    {TEXT("rotor_inertia"),          &FDronePhysicsSettings::RotorInertiaKgM2, nullptr, nullptr},
    {TEXT("rotor_radius_m"),         &FDronePhysicsSettings::RotorRadiusM, nullptr, nullptr},
    {TEXT("gravity"),                &FDronePhysicsSettings::GravityMS2, nullptr, nullptr},
    {TEXT("air_density"),            &FDronePhysicsSettings::AirDensity, nullptr, nullptr},
    {TEXT("drag_linear"),            &FDronePhysicsSettings::DragLinear, nullptr, nullptr},
    {TEXT("drag_quadratic"),         &FDronePhysicsSettings::DragQuadratic, nullptr, nullptr},
    {TEXT("ground_effect_strength"), &FDronePhysicsSettings::GroundEffectStrength, nullptr, nullptr},
    {TEXT("wind_x"),                 &FDronePhysicsSettings::WindXMS, nullptr, nullptr},
    {TEXT("wind_y"),                 &FDronePhysicsSettings::WindYMS, nullptr, nullptr},
    {TEXT("gust_intensity"),         &FDronePhysicsSettings::GustIntensityMS, nullptr, nullptr},
    {TEXT("max_speed_ms"),           &FDronePhysicsSettings::MaxSpeedMS, nullptr, nullptr},
    {TEXT("max_climb_rate_ms"),      &FDronePhysicsSettings::MaxClimbRateMS, nullptr, nullptr},
    {TEXT("max_tilt_deg"),           &FDronePhysicsSettings::MaxTiltDeg, nullptr, nullptr},
    {TEXT("takeoff_altitude_m"),     &FDronePhysicsSettings::TakeoffAltitudeM, nullptr, nullptr},
    {TEXT("vel_p_gain"),             &FDronePhysicsSettings::VelPGain, nullptr, nullptr},
    {TEXT("vel_i_gain"),             &FDronePhysicsSettings::VelIGain, nullptr, nullptr},
    {TEXT("att_p_gain"),             &FDronePhysicsSettings::AttPGain, nullptr, nullptr},
    {TEXT("rate_p_gain"),            &FDronePhysicsSettings::RatePGain, nullptr, nullptr},
    {TEXT("rate_d_gain"),            &FDronePhysicsSettings::RateDGain, nullptr, nullptr},
    {TEXT("substep_hz"),             nullptr, nullptr, &FDronePhysicsSettings::SubstepHz},
    {TEXT("settle_speed_threshold"), nullptr, &FDronePhysicsSettings::SettleSpeedThreshold, nullptr},
    {TEXT("settle_tilt_threshold"),  nullptr, &FDronePhysicsSettings::SettleTiltThreshold, nullptr},
};
// clang-format on

auto PresetFilePath() -> FString
{
    return FPaths::ProjectContentDir() / TEXT("Drones") / TEXT("DronePresets.json");
}

/** Fills one settings struct from a JSON object; unknown keys are ignored and
 *  missing keys keep the STANDARD 1.2 kg defaults */
void ApplyJsonFields(const FJsonObject &Object, FDronePhysicsSettings &Settings)
{
    for (const FPresetField &Field : PresetFields)
    {
        double Value = 0.0;
        if (!Object.TryGetNumberField(Field.Key, Value))
        {
            continue;
        }
        if (Field.DoubleField != nullptr)
        {
            Settings.*Field.DoubleField = Value;
        }
        else if (Field.FloatField != nullptr)
        {
            Settings.*Field.FloatField = static_cast<float>(Value);
        }
        else
        {
            Settings.*Field.IntField = FMath::RoundToInt32(Value);
        }
    }
}
} // namespace

namespace DronePhysicsPresets
{

auto LoadPresets() -> TArray<FDroneAirframePreset>
{
    const FString FilePath = PresetFilePath();

    FString JsonText;
    if (!FFileHelper::LoadFileToString(JsonText, *FilePath))
    {
        FScenarioLog::Error(FString::Printf(TEXT("Drone preset file not found, using built-ins: %s"), *FilePath));
        return BuiltInPresets();
    }

    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
    const TArray<TSharedPtr<FJsonValue>> *PresetValues = nullptr;
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid() ||
        !Root->TryGetArrayField(TEXT("presets"), PresetValues))
    {
        FScenarioLog::Error(
            FString::Printf(TEXT("Drone preset file needs a 'presets' array, using built-ins: %s"), *FilePath));
        return BuiltInPresets();
    }

    TArray<FDroneAirframePreset> Presets;
    for (const TSharedPtr<FJsonValue> &Value : *PresetValues)
    {
        const TSharedPtr<FJsonObject> *Object = nullptr;
        FDroneAirframePreset Preset;
        if (!Value->TryGetObject(Object) || !(*Object)->TryGetStringField(TEXT("name"), Preset.Name))
        {
            FScenarioLog::Error(
                FString::Printf(TEXT("Drone preset[%d] needs a 'name', skipped: %s"), Presets.Num(), *FilePath));
            continue;
        }
        ApplyJsonFields(**Object, Preset.Settings);
        Presets.Add(MoveTemp(Preset));
    }

    if (Presets.IsEmpty())
    {
        FScenarioLog::Error(FString::Printf(TEXT("Drone preset file has no usable presets: %s"), *FilePath));
        return BuiltInPresets();
    }

    FScenarioLog::Info(FString::Printf(TEXT("Loaded %d drone presets from %s"), Presets.Num(), *FilePath));
    return Presets;
}

auto BuiltInPresets() -> TArray<FDroneAirframePreset>
{
    TArray<FDroneAirframePreset> Presets;
    Presets.Reserve(6);

    // hover ~2020 rad/s of 3500 max: thrust-to-weight ~3.0
    FDroneAirframePreset Micro{TEXT("MICRO 250 g"), {}};
    Micro.Settings.MassKg = 0.249;
    Micro.Settings.ArmLengthM = 0.09;
    Micro.Settings.InertiaXX = 2.5e-4;
    Micro.Settings.InertiaYY = 2.5e-4;
    Micro.Settings.InertiaZZ = 4.0e-4;
    Micro.Settings.MotorTimeConstantS = 0.02;
    Micro.Settings.ThrustCoefficient = 1.5e-7;
    Micro.Settings.TorqueCoefficient = 2.4e-9;
    Micro.Settings.MotorMaxRadS = 3500.0;
    Micro.Settings.RotorInertiaKgM2 = 2e-6;
    Micro.Settings.RotorRadiusM = 0.06;
    Micro.Settings.DragLinear = 0.08;
    Micro.Settings.DragQuadratic = 0.03;
    Micro.Settings.MaxSpeedMS = 12.0;
    Micro.Settings.MaxTiltDeg = 30.0;
    Presets.Add(Micro);

    // hover ~1810 rad/s of 4000 max: thrust-to-weight ~4.9, racing agility
    FDroneAirframePreset Racer{TEXT("FPV RACER 600 g"), {}};
    Racer.Settings.MassKg = 0.6;
    Racer.Settings.ArmLengthM = 0.12;
    Racer.Settings.InertiaXX = 2.5e-3;
    Racer.Settings.InertiaYY = 2.5e-3;
    Racer.Settings.InertiaZZ = 4.5e-3;
    Racer.Settings.MotorTimeConstantS = 0.015;
    Racer.Settings.ThrustCoefficient = 4.5e-7;
    Racer.Settings.TorqueCoefficient = 7e-9;
    Racer.Settings.MotorMaxRadS = 4000.0;
    Racer.Settings.RotorInertiaKgM2 = 1e-5;
    Racer.Settings.RotorRadiusM = 0.0635;
    Racer.Settings.DragLinear = 0.15;
    Racer.Settings.DragQuadratic = 0.05;
    Racer.Settings.MaxSpeedMS = 27.0;
    Racer.Settings.MaxTiltDeg = 40.0;
    Presets.Add(Racer);

    // the struct's C++ defaults ARE this class
    Presets.Add({TEXT("STANDARD 1.2 kg"), {}});

    // hover ~898 rad/s of 1500 max: thrust-to-weight ~2.8
    FDroneAirframePreset Cinema{TEXT("CINEMA 2.5 kg"), {}};
    Cinema.Settings.MassKg = 2.5;
    Cinema.Settings.ArmLengthM = 0.25;
    Cinema.Settings.InertiaXX = 3.5e-2;
    Cinema.Settings.InertiaYY = 3.5e-2;
    Cinema.Settings.InertiaZZ = 6.0e-2;
    Cinema.Settings.MotorTimeConstantS = 0.08;
    Cinema.Settings.ThrustCoefficient = 7.6e-6;
    Cinema.Settings.TorqueCoefficient = 1.5e-7;
    Cinema.Settings.MotorMaxRadS = 1500.0;
    Cinema.Settings.RotorInertiaKgM2 = 1.5e-4;
    Cinema.Settings.RotorRadiusM = 0.17;
    Cinema.Settings.DragLinear = 0.4;
    Cinema.Settings.DragQuadratic = 0.18;
    Cinema.Settings.MaxSpeedMS = 22.0;
    Cinema.Settings.MaxTiltDeg = 30.0;
    // slower motors need a gentler inner loop to stay clear of the actuator pole
    Cinema.Settings.AttPGain = 6.0;
    Cinema.Settings.RatePGain = 12.0;
    Presets.Add(Cinema);

    // hover ~499 rad/s of 850 max: thrust-to-weight ~2.9
    FDroneAirframePreset Industrial{TEXT("INDUSTRIAL 6.5 kg"), {}};
    Industrial.Settings.MassKg = 6.5;
    Industrial.Settings.ArmLengthM = 0.45;
    Industrial.Settings.InertiaXX = 0.35;
    Industrial.Settings.InertiaYY = 0.35;
    Industrial.Settings.InertiaZZ = 0.6;
    Industrial.Settings.MotorTimeConstantS = 0.12;
    Industrial.Settings.ThrustCoefficient = 6.4e-5;
    Industrial.Settings.TorqueCoefficient = 1.6e-6;
    Industrial.Settings.MotorMaxRadS = 850.0;
    Industrial.Settings.RotorInertiaKgM2 = 8e-4;
    Industrial.Settings.RotorRadiusM = 0.27;
    Industrial.Settings.DragLinear = 0.8;
    Industrial.Settings.DragQuadratic = 0.35;
    Industrial.Settings.MaxSpeedMS = 17.0;
    Industrial.Settings.MaxTiltDeg = 30.0;
    Industrial.Settings.AttPGain = 5.0;
    Industrial.Settings.RatePGain = 10.0;
    Presets.Add(Industrial);

    // hover ~300 rad/s of 520 max: thrust-to-weight ~3.0
    FDroneAirframePreset HeavyLift{TEXT("HEAVY LIFT 25 kg"), {}};
    HeavyLift.Settings.MassKg = 25.0;
    HeavyLift.Settings.ArmLengthM = 0.9;
    HeavyLift.Settings.InertiaXX = 4.0;
    HeavyLift.Settings.InertiaYY = 4.0;
    HeavyLift.Settings.InertiaZZ = 7.0;
    HeavyLift.Settings.MotorTimeConstantS = 0.18;
    HeavyLift.Settings.ThrustCoefficient = 6.8e-4;
    HeavyLift.Settings.TorqueCoefficient = 2e-5;
    HeavyLift.Settings.MotorMaxRadS = 520.0;
    HeavyLift.Settings.RotorInertiaKgM2 = 6e-3;
    HeavyLift.Settings.RotorRadiusM = 0.5;
    HeavyLift.Settings.DragLinear = 2.0;
    HeavyLift.Settings.DragQuadratic = 1.2;
    HeavyLift.Settings.MaxSpeedMS = 13.0;
    HeavyLift.Settings.MaxTiltDeg = 25.0;
    // the slowest actuators of the range: gentlest inner loop
    HeavyLift.Settings.AttPGain = 4.0;
    HeavyLift.Settings.RatePGain = 8.0;
    Presets.Add(HeavyLift);

    return Presets;
}

} // namespace DronePhysicsPresets
