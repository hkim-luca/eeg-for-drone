#pragma once

#include "CoreMinimal.h"
#include "DronePhysicsSettings.h"
#include "UObject/Object.h"

#include "DronePhysicsConfig.generated.h"

/**
 *  Single source of truth for the drone physics parameters. Both runner components
 *  read from here when their physics begins, and the in-game settings UI edits this
 *  object directly. Values persist across runs in Saved/Config (Game ini section)
 *  once Save() is called.
 */
UCLASS(Config = Game)
class UDronePhysicsConfig : public UObject
{
    GENERATED_BODY()

  public:
    /** The mutable class default object holding the live settings */
    static auto Get() -> UDronePhysicsConfig *;

    /** Writes the current settings to the Game ini so they survive a restart */
    void Save();

    /** Restores every parameter to its C++ default (does not save) */
    void ResetToDefaults();

    /** Live physics parameters; edited by the settings UI */
    UPROPERTY(Config, EditAnywhere, Category = "Drone Physics")
    FDronePhysicsSettings Settings;
};
