#include "DronePhysicsConfig.h"

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
}
