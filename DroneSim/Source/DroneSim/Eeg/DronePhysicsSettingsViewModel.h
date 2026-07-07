#pragma once

#include "CoreMinimal.h"
#include "Scenario/DronePhysicsPresets.h"

#include "DronePhysicsSettingsViewModel.generated.h"

/**
 *  ViewModel of the drone physics settings editor (MVVM). Owns every read, write and
 *  business rule against the model (UDronePhysicsConfig + DronePhysicsPresets):
 *  parameter edits mark the preset CUSTOM, the yaw-mode toggles enforce their mutual
 *  exclusion, and preset selection preserves the view/input flags. The view
 *  (UDronePhysicsSettingsWidget) only builds controls from the parameter descriptors,
 *  forwards user input to the commands, and repaints on OnStateChanged - it never
 *  touches the config directly.
 */
UCLASS()
class UDronePhysicsSettingsViewModel : public UObject
{
    GENERATED_BODY()

  public:
    /** Presentation-independent description of one editable parameter */
    struct FParameterDesc
    {
        /** Section caption this row opens, or nullptr to continue the previous one */
        const TCHAR *Section;
        const TCHAR *Label;
        double Min;
        double Max;
        int32 FractionalDigits;
    };

    /** Number of rows in the parameter table */
    static auto ParameterCount() -> int32;

    /** Descriptor of one parameter row (0 <= Index < ParameterCount()) */
    static auto ParameterDesc(int32 Index) -> const FParameterDesc &;

    // --- state the view reads ---

    auto GetParameterValue(int32 Index) const -> double;
    auto GetMouseYawControl() const -> bool;
    auto GetTurnWithLeftRight() const -> bool;
    auto GetPresetName() const -> FString;
    auto GetPresetNames() -> TArray<FString>;

    // --- commands the view invokes ---

    /** Writes one parameter; a real change marks the preset CUSTOM */
    void SetParameterValue(int32 Index, double Value);

    /** Yaw-mode toggles; enabling one disables the other (both own the yaw setpoint,
     *  and the mouse would silently swallow the Left/Right turn commands) */
    void SetMouseYawControl(bool bEnabled);
    void SetTurnWithLeftRight(bool bEnabled);

    /** Applies an airframe preset, keeping the view/input flags (yaw modes) */
    void SelectPreset(const FString &PresetName);

    /** Persists the config to the Game ini */
    void Save();

    /** Restores every parameter to its C++ default (does not save) */
    void RestoreDefaults();

    // --- change notification the view subscribes to ---

    /** Any state change the view must repaint for (edits, preset, defaults) */
    FSimpleMulticastDelegate OnStateChanged;

  private:
    /** Loads the airframe presets on first use */
    void EnsurePresetsLoaded();

    /** Airframes from Content/Drones/DronePresets.json (built-ins on error) */
    TArray<FDroneAirframePreset> Presets;

    /** True once EnsurePresetsLoaded read the preset file */
    bool bPresetsLoaded = false;
};
