#include "DronePhysicsSettingsViewModel.h"
#include "Scenario/DronePhysicsConfig.h"

namespace
{
/** One editable parameter: the descriptor shown to the view plus the config field
 *  behind it (exactly one of the three member pointers is set) */
struct FParameterBinding
{
    UDronePhysicsSettingsViewModel::FParameterDesc Desc;
    double FDronePhysicsSettings::*DoubleField;
    float FDronePhysicsSettings::*FloatField;
    int32 FDronePhysicsSettings::*IntField;
};

// clang-format off
const FParameterBinding ParameterBindings[] = {
    {{TEXT("AIRFRAME"), TEXT("MASS [kg]"),             0.1, 30.0, 3},      &FDronePhysicsSettings::MassKg,               nullptr, nullptr},
    {{nullptr,          TEXT("ARM LENGTH [m]"),        0.05, 1.0, 3},      &FDronePhysicsSettings::ArmLengthM,           nullptr, nullptr},
    {{nullptr,          TEXT("INERTIA XX [kg m2]"),    0.0001, 10.0, 5},   &FDronePhysicsSettings::InertiaXX,            nullptr, nullptr},
    {{nullptr,          TEXT("INERTIA YY [kg m2]"),    0.0001, 10.0, 5},   &FDronePhysicsSettings::InertiaYY,            nullptr, nullptr},
    {{nullptr,          TEXT("INERTIA ZZ [kg m2]"),    0.0001, 10.0, 5},   &FDronePhysicsSettings::InertiaZZ,            nullptr, nullptr},
    {{TEXT("MOTOR"),    TEXT("TIME CONST [s]"),        0.005, 0.5, 3},     &FDronePhysicsSettings::MotorTimeConstantS,   nullptr, nullptr},
    {{nullptr,          TEXT("THRUST COEF kT"),        1e-7, 1e-3, 9},     &FDronePhysicsSettings::ThrustCoefficient,    nullptr, nullptr},
    {{nullptr,          TEXT("TORQUE COEF kQ"),        1e-9, 1e-4, 10},    &FDronePhysicsSettings::TorqueCoefficient,    nullptr, nullptr},
    {{nullptr,          TEXT("MAX SPEED [rad/s]"),     100.0, 10000.0, 0}, &FDronePhysicsSettings::MotorMaxRadS,         nullptr, nullptr},
    {{nullptr,          TEXT("ROTOR INERTIA [kg m2]"), 0.0, 0.01, 7},      &FDronePhysicsSettings::RotorInertiaKgM2,     nullptr, nullptr},
    {{nullptr,          TEXT("ROTOR RADIUS [m]"),      0.01, 1.0, 3},      &FDronePhysicsSettings::RotorRadiusM,         nullptr, nullptr},
    {{TEXT("ENVIRONMENT"), TEXT("GRAVITY [m/s2]"),     0.0, 30.0, 3},      &FDronePhysicsSettings::GravityMS2,           nullptr, nullptr},
    {{nullptr,          TEXT("AIR DENSITY [kg/m3]"),   0.0, 2.0, 3},       &FDronePhysicsSettings::AirDensity,           nullptr, nullptr},
    {{nullptr,          TEXT("DRAG LINEAR"),           0.0, 10.0, 3},      &FDronePhysicsSettings::DragLinear,           nullptr, nullptr},
    {{nullptr,          TEXT("DRAG QUADRATIC"),        0.0, 10.0, 3},      &FDronePhysicsSettings::DragQuadratic,        nullptr, nullptr},
    {{nullptr,          TEXT("GROUND EFFECT"),         0.0, 1.0, 2},       &FDronePhysicsSettings::GroundEffectStrength, nullptr, nullptr},
    {{nullptr,          TEXT("WIND X [m/s]"),          -30.0, 30.0, 1},    &FDronePhysicsSettings::WindXMS,              nullptr, nullptr},
    {{nullptr,          TEXT("WIND Y [m/s]"),          -30.0, 30.0, 1},    &FDronePhysicsSettings::WindYMS,              nullptr, nullptr},
    {{nullptr,          TEXT("GUST [m/s]"),            0.0, 15.0, 1},      &FDronePhysicsSettings::GustIntensityMS,      nullptr, nullptr},
    {{TEXT("CONTROL"),  TEXT("MAX SPEED [m/s]"),       0.5, 40.0, 1},      &FDronePhysicsSettings::MaxSpeedMS,           nullptr, nullptr},
    {{nullptr,          TEXT("MAX CLIMB [m/s]"),       0.1, 10.0, 1},      &FDronePhysicsSettings::MaxClimbRateMS,       nullptr, nullptr},
    {{nullptr,          TEXT("MAX TILT [deg]"),        1.0, 45.0, 1},      &FDronePhysicsSettings::MaxTiltDeg,           nullptr, nullptr},
    {{nullptr,          TEXT("TAKEOFF ALT [m]"),       0.0, 50.0, 1},      &FDronePhysicsSettings::TakeoffAltitudeM,     nullptr, nullptr},
    {{nullptr,          TEXT("VEL P GAIN"),            0.1, 20.0, 2},      &FDronePhysicsSettings::VelPGain,             nullptr, nullptr},
    {{nullptr,          TEXT("VEL I GAIN"),            0.0, 10.0, 2},      &FDronePhysicsSettings::VelIGain,             nullptr, nullptr},
    {{nullptr,          TEXT("ATT P GAIN"),            0.5, 50.0, 1},      &FDronePhysicsSettings::AttPGain,             nullptr, nullptr},
    {{nullptr,          TEXT("RATE P GAIN"),           1.0, 200.0, 1},     &FDronePhysicsSettings::RatePGain,            nullptr, nullptr},
    {{nullptr,          TEXT("RATE D GAIN"),           0.0, 5.0, 2},       &FDronePhysicsSettings::RateDGain,            nullptr, nullptr},
    {{nullptr,          TEXT("TURN RATE [deg/s]"),     5.0, 360.0, 0},     &FDronePhysicsSettings::TurnRateDegS,         nullptr, nullptr},
    {{nullptr,          TEXT("SUBSTEP [Hz]"),          250.0, 4000.0, 0},  nullptr, nullptr, &FDronePhysicsSettings::SubstepHz},
    {{TEXT("SETTLE"),   TEXT("SPEED THRESH [cm/s]"),   0.1, 100.0, 1},     nullptr, &FDronePhysicsSettings::SettleSpeedThreshold, nullptr},
    {{nullptr,          TEXT("TILT THRESH [deg]"),     0.01, 5.0, 2},      nullptr, &FDronePhysicsSettings::SettleTiltThreshold, nullptr},
};
// clang-format on

constexpr int32 ParameterBindingCount = UE_ARRAY_COUNT(ParameterBindings);

auto BindingValue(const FDronePhysicsSettings &Settings, const FParameterBinding &Binding) -> double
{
    if (Binding.DoubleField != nullptr)
    {
        return Settings.*Binding.DoubleField;
    }
    if (Binding.FloatField != nullptr)
    {
        return Settings.*Binding.FloatField;
    }
    return Settings.*Binding.IntField;
}

void SetBindingValue(FDronePhysicsSettings &Settings, const FParameterBinding &Binding, double Value)
{
    if (Binding.DoubleField != nullptr)
    {
        Settings.*Binding.DoubleField = Value;
    }
    else if (Binding.FloatField != nullptr)
    {
        Settings.*Binding.FloatField = static_cast<float>(Value);
    }
    else
    {
        Settings.*Binding.IntField = FMath::RoundToInt32(Value);
    }
}
} // namespace

auto UDronePhysicsSettingsViewModel::ParameterCount() -> int32
{
    return ParameterBindingCount;
}

auto UDronePhysicsSettingsViewModel::ParameterDesc(int32 Index) -> const FParameterDesc &
{
    return ParameterBindings[Index].Desc;
}

auto UDronePhysicsSettingsViewModel::GetParameterValue(int32 Index) const -> double
{
    return BindingValue(UDronePhysicsConfig::Get()->Settings, ParameterBindings[Index]);
}

auto UDronePhysicsSettingsViewModel::GetMouseYawControl() const -> bool
{
    return UDronePhysicsConfig::Get()->Settings.bMouseYawControl;
}

auto UDronePhysicsSettingsViewModel::GetTurnWithLeftRight() const -> bool
{
    return UDronePhysicsConfig::Get()->Settings.bTurnWithLeftRight;
}

auto UDronePhysicsSettingsViewModel::GetPresetName() const -> FString
{
    return UDronePhysicsConfig::Get()->PresetName;
}

auto UDronePhysicsSettingsViewModel::GetPresetNames() -> TArray<FString>
{
    EnsurePresetsLoaded();
    TArray<FString> Names;
    Names.Reserve(Presets.Num());
    for (const FDroneAirframePreset &Preset : Presets)
    {
        Names.Add(Preset.Name);
    }
    return Names;
}

void UDronePhysicsSettingsViewModel::SetParameterValue(int32 Index, double Value)
{
    UDronePhysicsConfig *Config = UDronePhysicsConfig::Get();
    if (BindingValue(Config->Settings, ParameterBindings[Index]) == Value)
    {
        return;
    }

    SetBindingValue(Config->Settings, ParameterBindings[Index], Value);

    // hand-edited values no longer match any airframe preset
    Config->PresetName = DronePhysicsPresets::CustomName;
    OnStateChanged.Broadcast();
}

void UDronePhysicsSettingsViewModel::SetMouseYawControl(bool bEnabled)
{
    UDronePhysicsConfig *Config = UDronePhysicsConfig::Get();
    if (Config->Settings.bMouseYawControl == bEnabled)
    {
        return;
    }

    // the mouse and the turn actions cannot both own the yaw setpoint (the mouse
    // would silently swallow Left/Right); enabling one disables the other
    Config->Settings.bMouseYawControl = bEnabled;
    if (bEnabled)
    {
        Config->Settings.bTurnWithLeftRight = false;
    }
    OnStateChanged.Broadcast();
}

void UDronePhysicsSettingsViewModel::SetTurnWithLeftRight(bool bEnabled)
{
    UDronePhysicsConfig *Config = UDronePhysicsConfig::Get();
    if (Config->Settings.bTurnWithLeftRight == bEnabled)
    {
        return;
    }

    // mutually exclusive with the mouse yaw mode; see SetMouseYawControl
    Config->Settings.bTurnWithLeftRight = bEnabled;
    if (bEnabled)
    {
        Config->Settings.bMouseYawControl = false;
    }
    OnStateChanged.Broadcast();
}

void UDronePhysicsSettingsViewModel::SelectPreset(const FString &PresetName)
{
    EnsurePresetsLoaded();
    for (const FDroneAirframePreset &Preset : Presets)
    {
        if (PresetName != Preset.Name)
        {
            continue;
        }

        UDronePhysicsConfig *Config = UDronePhysicsConfig::Get();
        // the yaw/action modes are view/input choices, not part of the airframe
        const bool bKeepMouseYaw = Config->Settings.bMouseYawControl;
        const bool bKeepTurnMode = Config->Settings.bTurnWithLeftRight;
        Config->Settings = Preset.Settings;
        Config->Settings.bMouseYawControl = bKeepMouseYaw;
        Config->Settings.bTurnWithLeftRight = bKeepTurnMode;
        Config->PresetName = Preset.Name;
        OnStateChanged.Broadcast();
        return;
    }
}

void UDronePhysicsSettingsViewModel::Save()
{
    UDronePhysicsConfig::Get()->Save();
}

void UDronePhysicsSettingsViewModel::RestoreDefaults()
{
    UDronePhysicsConfig::Get()->ResetToDefaults();
    OnStateChanged.Broadcast();
}

void UDronePhysicsSettingsViewModel::EnsurePresetsLoaded()
{
    if (!bPresetsLoaded)
    {
        bPresetsLoaded = true;
        Presets = DronePhysicsPresets::LoadPresets();
    }
}
