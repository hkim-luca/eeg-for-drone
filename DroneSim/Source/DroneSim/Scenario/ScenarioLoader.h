#pragma once

#include "CoreMinimal.h"
#include "ScenarioTypes.h"

/**
 *  Loads scenario steps from a JSON file. Steps play in file order; the runner waits
 *  between steps until the drone has physically settled, so no start times are given.
 *  Expected format:
 *  { "steps": [ { "duration": 2.0, "action": "forward" }, ... ] }
 *  Valid action names are the EScenarioAction entries (case-insensitive).
 */
class FScenarioLoader
{
  public:
    /** Parses the JSON file at FilePath into OutSteps (in file order). Returns false on file or parse errors. */
    static auto LoadFromFile(const FString &FilePath, TArray<FScenarioStep> &OutSteps) -> bool;

  private:
    /** Maps an action name (case-insensitive) to the enum via reflection. Returns false for unknown names. */
    static auto ParseAction(const FString &ActionName, EScenarioAction &OutAction) -> bool;
};
