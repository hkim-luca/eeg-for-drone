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
 *  Resolves which scenario JSON file to play from an external ini, so deployed
 *  builds can be reconfigured without repackaging. Users keep any number of
 *  scenario files in Content/Scenarios and select one in Scenarios.ini:
 *    [Scenarios]
 *    File=Mission1.json
 *  The entry is relative to the Scenarios folder (absolute paths also work).
 *  Without the ini (or with an empty entry) DefaultScenario.json plays.
 */
class FScenarioConfig
{
  public:
    /** Loads the scenario selected in Scenarios.ini. Returns false if the file fails to load. */
    static auto LoadConfiguredScenario(FLoadedScenario &OutScenario) -> bool;

  private:
    /** Returns the absolute path of the selected scenario file */
    static auto ResolveScenarioFile() -> FString;
};
