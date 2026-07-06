#include "DronePhysicsPresets.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ScenarioLog.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
/** One airframe model from the JSON "models" section: what the drone IS (body mesh
 *  and rotor layout), as opposed to a preset, which is how it is parameterized */
struct FAirframeModel
{
    FString MeshPath;
    int32 MotorCount = 4;
};

/** JSON key -> settings member; keys follow the proto field names for consistency.
 *  motor_count and the body mesh are deliberately absent: they belong to the model
 *  a preset references, never to an individual preset. */
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

/** The one body mesh shipped with the project; hexa/octo models reuse it as a
 *  placeholder until dedicated assets are imported (swap the path in the JSON) */
const TCHAR *QuadMeshPath = TEXT("/Game/Characters/Drone/Flying_drone_/SkeletalMeshes/Flying_drone_.Flying_drone_");

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

/** Parses the "models" section into name -> {mesh, rotor count}; corrections are
 *  logged - a typo the user cannot see is undebuggable */
auto ParseModels(const FJsonObject &Root, const FString &FilePath) -> TMap<FString, FAirframeModel>
{
    TMap<FString, FAirframeModel> Models;
    const TArray<TSharedPtr<FJsonValue>> *ModelValues = nullptr;
    if (!Root.TryGetArrayField(TEXT("models"), ModelValues))
    {
        FScenarioLog::Error(FString::Printf(TEXT("Drone preset file has no 'models' array: %s"), *FilePath));
        return Models;
    }

    for (const TSharedPtr<FJsonValue> &Value : *ModelValues)
    {
        const TSharedPtr<FJsonObject> *Object = nullptr;
        FString Name;
        FAirframeModel Model;
        if (!Value->TryGetObject(Object) || !(*Object)->TryGetStringField(TEXT("name"), Name) ||
            !(*Object)->TryGetStringField(TEXT("mesh"), Model.MeshPath))
        {
            FScenarioLog::Error(
                FString::Printf(TEXT("Drone model[%d] needs 'name' and 'mesh', skipped: %s"), Models.Num(), *FilePath));
            continue;
        }

        double MotorCount = 4.0;
        (*Object)->TryGetNumberField(TEXT("motor_count"), MotorCount);
        Model.MotorCount = FMath::RoundToInt32(MotorCount);
        if (Model.MotorCount != 4 && Model.MotorCount != 6 && Model.MotorCount != 8)
        {
            const int32 Snapped = Model.MotorCount <= 4 ? 4 : (Model.MotorCount <= 6 ? 6 : 8);
            FScenarioLog::Error(
                FString::Printf(TEXT("Model '%s': motor_count %d is not a supported layout (4/6/8); using %d"), *Name,
                                Model.MotorCount, Snapped));
            Model.MotorCount = Snapped;
        }
        Models.Add(Name, Model);
    }
    return Models;
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

    const TMap<FString, FAirframeModel> Models = ParseModels(*Root, FilePath);

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

        // every preset must fly a declared model; the model owns rotor count and body mesh
        if (!(*Object)->TryGetStringField(TEXT("model"), Preset.ModelName))
        {
            FScenarioLog::Error(FString::Printf(TEXT("Preset '%s' has no 'model' reference, skipped"), *Preset.Name));
            continue;
        }
        const FAirframeModel *Model = Models.Find(Preset.ModelName);
        if (Model == nullptr)
        {
            FScenarioLog::Error(FString::Printf(TEXT("Preset '%s' references unknown model '%s', skipped"),
                                                *Preset.Name, *Preset.ModelName));
            continue;
        }
        Preset.Settings.MotorCount = Model->MotorCount;
        Preset.Settings.AirframeMeshPath = Model->MeshPath;

        ApplyJsonFields(**Object, Preset.Settings);
        // hand-edited preset files bypass the UI clamps; never ship a divide-by-zero
        Preset.Settings.Sanitize();
        Presets.Add(MoveTemp(Preset));
    }

    if (Presets.IsEmpty())
    {
        FScenarioLog::Error(FString::Printf(TEXT("Drone preset file has no usable presets: %s"), *FilePath));
        return BuiltInPresets();
    }

    FScenarioLog::Info(
        FString::Printf(TEXT("Loaded %d drone presets (%d models) from %s"), Presets.Num(), Models.Num(), *FilePath));
    return Presets;
}

auto BuiltInPresets() -> TArray<FDroneAirframePreset>
{
    TArray<FDroneAirframePreset> Presets;
    Presets.Reserve(6);

    const auto WithModel = [](FDroneAirframePreset Preset, const TCHAR *ModelName, int32 MotorCount) {
        Preset.ModelName = ModelName;
        Preset.Settings.MotorCount = MotorCount;
        Preset.Settings.AirframeMeshPath = QuadMeshPath;
        return Preset;
    };

    // kT/kQ from UIUC static data (GWS DD 5x3); hover ~931 rad/s of 1600 max: T/W ~3.0
    FDroneAirframePreset Micro{TEXT("MICRO 250 g"), {}, {}};
    Micro.Settings.MassKg = 0.249;
    Micro.Settings.ArmLengthM = 0.09;
    Micro.Settings.InertiaXX = 2.5e-4;
    Micro.Settings.InertiaYY = 2.5e-4;
    Micro.Settings.InertiaZZ = 4.0e-4;
    Micro.Settings.MotorTimeConstantS = 0.02;
    Micro.Settings.ThrustCoefficient = 7.04e-7;
    Micro.Settings.TorqueCoefficient = 5.88e-9;
    Micro.Settings.MotorMaxRadS = 1600.0;
    Micro.Settings.RotorInertiaKgM2 = 2e-6;
    Micro.Settings.RotorRadiusM = 0.06;
    Micro.Settings.DragLinear = 0.08;
    Micro.Settings.DragQuadratic = 0.03;
    Micro.Settings.MaxSpeedMS = 12.0;
    Micro.Settings.MaxTiltDeg = 30.0;
    Presets.Add(WithModel(Micro, TEXT("FLYING DRONE QUAD"), 4));

    // kT/kQ from UIUC static data (DA4002 5x3.75); hover ~1208 rad/s of 2700 max: T/W ~5.0
    FDroneAirframePreset Racer{TEXT("FPV RACER 600 g"), {}, {}};
    Racer.Settings.MassKg = 0.6;
    Racer.Settings.ArmLengthM = 0.12;
    Racer.Settings.InertiaXX = 2.5e-3;
    Racer.Settings.InertiaYY = 2.5e-3;
    Racer.Settings.InertiaZZ = 4.5e-3;
    Racer.Settings.MotorTimeConstantS = 0.015;
    Racer.Settings.ThrustCoefficient = 1.01e-6;
    Racer.Settings.TorqueCoefficient = 1.50e-8;
    Racer.Settings.MotorMaxRadS = 2700.0;
    Racer.Settings.RotorInertiaKgM2 = 1e-5;
    Racer.Settings.RotorRadiusM = 0.0635;
    Racer.Settings.DragLinear = 0.15;
    Racer.Settings.DragQuadratic = 0.05;
    Racer.Settings.MaxSpeedMS = 27.0;
    Racer.Settings.MaxTiltDeg = 40.0;
    Presets.Add(WithModel(Racer, TEXT("FLYING DRONE QUAD"), 4));

    // the struct's C++ defaults ARE this class (APC SF 9x4.7/10x4.7 UIUC data)
    Presets.Add(WithModel({TEXT("STANDARD 1.2 kg"), {}, {}}, TEXT("FLYING DRONE QUAD"), 4));

    // kT/kQ from UIUC static data (ANCF 13x6.5); hover ~401 rad/s of 670 max: T/W ~2.8
    FDroneAirframePreset Cinema{TEXT("CINEMA 2.5 kg"), {}, {}};
    Cinema.Settings.MassKg = 2.5;
    Cinema.Settings.ArmLengthM = 0.25;
    Cinema.Settings.InertiaXX = 3.5e-2;
    Cinema.Settings.InertiaYY = 3.5e-2;
    Cinema.Settings.InertiaZZ = 6.0e-2;
    Cinema.Settings.MotorTimeConstantS = 0.08;
    Cinema.Settings.ThrustCoefficient = 3.81e-5;
    Cinema.Settings.TorqueCoefficient = 7.11e-7;
    Cinema.Settings.MotorMaxRadS = 670.0;
    Cinema.Settings.RotorInertiaKgM2 = 1.5e-4;
    Cinema.Settings.RotorRadiusM = 0.17;
    Cinema.Settings.DragLinear = 0.4;
    Cinema.Settings.DragQuadratic = 0.18;
    Cinema.Settings.MaxSpeedMS = 22.0;
    Cinema.Settings.MaxTiltDeg = 30.0;
    // slower motors need a gentler inner loop to stay clear of the actuator pole
    Cinema.Settings.AttPGain = 6.0;
    Cinema.Settings.RatePGain = 12.0;
    Presets.Add(WithModel(Cinema, TEXT("FLYING DRONE QUAD"), 4));

    // hexa (Matrice 600 class); kT/kQ from UIUC static data (ANCF 16x8);
    // hover ~213 rad/s of 365 max: T/W ~2.9
    FDroneAirframePreset Industrial{TEXT("INDUSTRIAL 6.5 kg"), {}, {}};
    Industrial.Settings.MassKg = 6.5;
    Industrial.Settings.ArmLengthM = 0.45;
    Industrial.Settings.InertiaXX = 0.35;
    Industrial.Settings.InertiaYY = 0.35;
    Industrial.Settings.InertiaZZ = 0.6;
    Industrial.Settings.MotorTimeConstantS = 0.12;
    Industrial.Settings.ThrustCoefficient = 2.34e-4;
    Industrial.Settings.TorqueCoefficient = 6.99e-6;
    Industrial.Settings.MotorMaxRadS = 365.0;
    Industrial.Settings.RotorInertiaKgM2 = 8e-4;
    Industrial.Settings.RotorRadiusM = 0.27;
    Industrial.Settings.DragLinear = 0.8;
    Industrial.Settings.DragQuadratic = 0.35;
    Industrial.Settings.MaxSpeedMS = 17.0;
    Industrial.Settings.MaxTiltDeg = 30.0;
    Industrial.Settings.AttPGain = 5.0;
    Industrial.Settings.RatePGain = 10.0;
    Presets.Add(WithModel(Industrial, TEXT("FLYING DRONE HEXA"), 6));

    // flat octo (Agras class); FM-validated coefficients; hover ~212 rad/s of 370 max: T/W ~3.0
    FDroneAirframePreset HeavyLift{TEXT("HEAVY LIFT 25 kg"), {}, {}};
    HeavyLift.Settings.MassKg = 25.0;
    HeavyLift.Settings.ArmLengthM = 0.9;
    HeavyLift.Settings.InertiaXX = 4.0;
    HeavyLift.Settings.InertiaYY = 4.0;
    HeavyLift.Settings.InertiaZZ = 7.0;
    HeavyLift.Settings.MotorTimeConstantS = 0.18;
    HeavyLift.Settings.ThrustCoefficient = 6.8e-4;
    HeavyLift.Settings.TorqueCoefficient = 2e-5;
    HeavyLift.Settings.MotorMaxRadS = 370.0;
    HeavyLift.Settings.RotorInertiaKgM2 = 6e-3;
    HeavyLift.Settings.RotorRadiusM = 0.5;
    HeavyLift.Settings.DragLinear = 2.0;
    HeavyLift.Settings.DragQuadratic = 1.2;
    HeavyLift.Settings.MaxSpeedMS = 13.0;
    HeavyLift.Settings.MaxTiltDeg = 25.0;
    // the slowest actuators of the range: gentlest inner loop
    HeavyLift.Settings.AttPGain = 4.0;
    HeavyLift.Settings.RatePGain = 8.0;
    Presets.Add(WithModel(HeavyLift, TEXT("FLYING DRONE OCTO"), 8));

    return Presets;
}

} // namespace DronePhysicsPresets
