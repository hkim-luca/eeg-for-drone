#include "ScenarioConfig.h"
#include "Misc/Paths.h"
#include "ScenarioLoader.h"
#include "ScenarioLog.h"

namespace
{
/** Fixed scenario file location; staged as a loose file when packaged so users can replace it
 *  post-deployment without repackaging */
auto ScenarioFilePath() -> FString
{
    return FPaths::ProjectContentDir() / TEXT("Scenarios") / TEXT("Scenario.json");
}
} // namespace

auto FScenarioConfig::LoadConfiguredScenario(FLoadedScenario &OutScenario) -> bool
{
    OutScenario.FilePath = ScenarioFilePath();
    OutScenario.Steps.Reset();

    if (!FScenarioLoader::LoadFromFile(OutScenario.FilePath, OutScenario.Steps))
    {
        return false;
    }
    if (OutScenario.Steps.IsEmpty())
    {
        FScenarioLog::Error(FString::Printf(TEXT("Scenario file has no steps: %s"), *OutScenario.FilePath));
        return false;
    }
    return true;
}
