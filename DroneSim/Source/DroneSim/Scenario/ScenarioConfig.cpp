#include "ScenarioConfig.h"
#include "DroneSimPaths.h"
#include "ScenarioLoader.h"
#include "ScenarioLog.h"

namespace
{
/** Scenario file in the user-editable config folder; edited without repackaging */
auto ScenarioFilePath() -> FString
{
    return DroneSimConfigDir() / TEXT("Scenario.json");
}
} // namespace

auto FScenarioConfig::LoadConfiguredScenario(FLoadedScenario &OutScenario) -> bool
{
    // read once per run; later recordings reuse the first result
    static bool bLoaded = false;
    static bool bValid = false;
    static FLoadedScenario Cached;
    if (!bLoaded)
    {
        bLoaded = true;
        Cached.FilePath = ScenarioFilePath();
        bValid = FScenarioLoader::LoadFromFile(Cached.FilePath, Cached.Steps);
        if (bValid && Cached.Steps.IsEmpty())
        {
            FScenarioLog::Error(FString::Printf(TEXT("Scenario file has no steps: %s"), *Cached.FilePath));
            bValid = false;
        }
    }
    OutScenario = Cached;
    return bValid;
}
