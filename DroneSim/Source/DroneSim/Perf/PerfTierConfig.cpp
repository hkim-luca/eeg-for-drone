#include "PerfTierConfig.h"
#include "Dom/JsonObject.h"
#include "DroneSimPaths.h"
#include "Misc/FileHelper.h"
#include "Scenario/ScenarioLog.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

auto PerfTierName(EPerfTier Tier) -> const TCHAR *
{
    switch (Tier)
    {
    case EPerfTier::High:
        return TEXT("High");
    case EPerfTier::Low:
        return TEXT("Low");
    default:
        return TEXT("");
    }
}

auto FPerfTierConfig::FilePath() -> FString
{
    return DroneSimConfigDir() / TEXT("perf_tier.json");
}

auto FPerfTierConfig::Load() -> FPerfTierConfig
{
    FPerfTierConfig Config;
    const FString Path = FilePath();

    FString JsonText;
    if (!FFileHelper::LoadFileToString(JsonText, *Path))
    {
        FScenarioLog::Info(FString::Printf(TEXT("Perf tier config not found, using defaults: %s"), *Path));
        return Config;
    }

    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        FScenarioLog::Error(FString::Printf(TEXT("Perf tier config is not valid JSON, using defaults: %s (%s)"), *Path,
                                            *Reader->GetErrorMessage()));
        return Config;
    }

    Root->TryGetBoolField(TEXT("enabled"), Config.bEnabled);
    Root->TryGetBoolField(TEXT("remeasure"), Config.bRemeasure);

    FString TierName;
    if (Root->TryGetStringField(TEXT("tier"), TierName) && !TierName.IsEmpty())
    {
        if (TierName.Equals(TEXT("High"), ESearchCase::IgnoreCase))
        {
            Config.Tier = EPerfTier::High;
        }
        else if (TierName.Equals(TEXT("Low"), ESearchCase::IgnoreCase))
        {
            Config.Tier = EPerfTier::Low;
        }
        else
        {
            FScenarioLog::Error(
                FString::Printf(TEXT("Unknown tier '%s' in %s (valid: High, Low); measuring again"), *TierName, *Path));
        }
    }
    return Config;
}

auto FPerfTierConfig::Save() const -> bool
{
    FString JsonText;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonText);
    Writer->WriteObjectStart();
    Writer->WriteValue(TEXT("enabled"), bEnabled);
    Writer->WriteValue(TEXT("remeasure"), bRemeasure);
    Writer->WriteValue(TEXT("tier"), FString(PerfTierName(Tier)));
    Writer->WriteObjectEnd();
    Writer->Close();

    const FString Path = FilePath();
    if (!FFileHelper::SaveStringToFile(JsonText, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        FScenarioLog::Error(
            FString::Printf(TEXT("Cannot save perf tier config: %s (%s)"), *Path, *FScenarioLog::SystemError()));
        return false;
    }
    return true;
}
