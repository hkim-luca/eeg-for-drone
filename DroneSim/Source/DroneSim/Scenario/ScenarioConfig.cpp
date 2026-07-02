#include "ScenarioConfig.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "ScenarioLoader.h"
#include "ScenarioLog.h"

namespace
{
const TCHAR *ScenariosIniName = TEXT("Scenarios.ini");
const TCHAR *ScenariosSection = TEXT("Scenarios");
const TCHAR *FileKey = TEXT("File");
const TCHAR *DefaultScenarioFile = TEXT("DefaultScenario.json");

/** Folder holding the scenario JSON files and Scenarios.ini; staged as loose files when packaged */
auto ScenariosDir() -> FString
{
    return FPaths::ProjectContentDir() / TEXT("Scenarios");
}
} // namespace

auto FScenarioConfig::ResolveScenarioFile() -> FString
{
    const FString IniPath = ScenariosDir() / ScenariosIniName;

    FString Entry;
    if (FPaths::FileExists(IniPath))
    {
        FConfigFile ConfigFile;
        ConfigFile.Read(IniPath);
        ConfigFile.GetString(ScenariosSection, FileKey, Entry);
        Entry.TrimStartAndEndInline();
    }

    if (Entry.IsEmpty())
    {
        FScenarioLog::Info(
            FString::Printf(TEXT("No scenario selected in %s; playing %s"), *IniPath, DefaultScenarioFile));
        Entry = DefaultScenarioFile;
    }

    return FPaths::IsRelative(Entry) ? ScenariosDir() / Entry : Entry;
}

auto FScenarioConfig::LoadConfiguredScenario(FLoadedScenario &OutScenario) -> bool
{
    OutScenario.FilePath = ResolveScenarioFile();
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
