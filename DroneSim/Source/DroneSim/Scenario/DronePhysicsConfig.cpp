#include "DronePhysicsConfig.h"
#include "DronePhysicsPresets.h"

auto UDronePhysicsConfig::Get() -> UDronePhysicsConfig *
{
    return GetMutableDefault<UDronePhysicsConfig>();
}

void UDronePhysicsConfig::Save()
{
    SaveConfig();
}

void UDronePhysicsConfig::ResetToDefaults()
{
    Settings = FDronePhysicsSettings();
    PresetName = TEXT("STANDARD 1.2 kg"); // the C++ defaults are exactly this airframe
}
