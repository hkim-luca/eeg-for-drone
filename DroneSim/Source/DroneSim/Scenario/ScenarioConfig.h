#pragma once

#include "CoreMinimal.h"
#include "ScenarioTypes.h"

/** One scenario file loaded and ready to play */
struct FLoadedScenario
{
    /** Absolute path of the source JSON file */
    FString FilePath;

    /** Steps parsed from the file, in file order */
    TArray<FScenarioStep> Steps;
};

/**
 *  Loads config/Scenario.json (see DroneSimConfigDir) on the first call of a run; later
 *  calls return that result. Users reconfigure by editing the file and restarting; no
 *  repackaging is needed.
 */
class FScenarioConfig
{
  public:
    /** Returns the scenario file's steps, read on first use. False if the file fails to load. */
    static auto LoadConfiguredScenario(FLoadedScenario &OutScenario) -> bool;
};
