#pragma once

#include "CoreMinimal.h"
#include "DronePhysicsSettings.h"

/** One selectable airframe preset: a display name, the airframe model it flies
 *  (resolved into Settings.MotorCount / Settings.AirframeMeshPath) and its full
 *  parameter set */
struct FDroneAirframePreset
{
    FString Name;
    FString ModelName;
    FDronePhysicsSettings Settings;
};

/**
 *  Airframe presets for the physics settings UI. The single source is
 *  Content/Drones/DronePresets.json (staged as a loose file when packaged, so
 *  airframes can be added or tuned post-deployment without repackaging):
 *  a "models" section declares the flyable airframe models (name, character mesh,
 *  rotor count), and every preset references one by name - rotor count and body
 *  mesh are properties of the model, never of an individual preset. The built-in
 *  table below is the fallback when the file is missing or invalid, and mirrors
 *  the shipped JSON. Every preset keeps a hover thrust-to-weight margin >= ~1.8
 *  so the controller can always climb; kT/kQ follow UIUC wind-tunnel data.
 */
namespace DronePhysicsPresets
{
/** Loads Content/Drones/DronePresets.json; falls back to the built-in table on
 *  any error (logged). Never returns an empty array. */
auto LoadPresets() -> TArray<FDroneAirframePreset>;

/** The compiled-in fallback table (MICRO 250 g .. HEAVY LIFT 25 kg) */
auto BuiltInPresets() -> TArray<FDroneAirframePreset>;

/** Preset name reported while the user has hand-edited individual values */
inline const TCHAR *CustomName = TEXT("CUSTOM");
} // namespace DronePhysicsPresets
