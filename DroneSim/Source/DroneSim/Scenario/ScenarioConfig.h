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
 *  Loads the scenario JSON file that deployed builds always play:
 *  Content/Scenarios/Scenario.json. Users reconfigure the run by replacing
 *  the contents of that single file; no repackaging is needed since the
 *  Scenarios folder is staged as loose files.
 */
class FScenarioConfig
{
  public:
    /** Loads Content/Scenarios/Scenario.json. Returns false if the file fails to load. */
    static auto LoadConfiguredScenario(FLoadedScenario &OutScenario) -> bool;
};
