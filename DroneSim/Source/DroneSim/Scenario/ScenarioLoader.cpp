#include "ScenarioLoader.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "ScenarioLog.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

auto FScenarioLoader::LoadFromFile(const FString &FilePath, TArray<FScenarioStep> &OutSteps) -> bool
{
    OutSteps.Reset();

    FString JsonText;
    if (!FFileHelper::LoadFileToString(JsonText, *FilePath))
    {
        FScenarioLog::Error(
            FString::Printf(TEXT("Scenario file not found: %s (%s)"), *FilePath, *FScenarioLog::SystemError()));
        return false;
    }

    TSharedPtr<FJsonObject> RootObject;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
    if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
    {
        FScenarioLog::Error(FString::Printf(TEXT("Scenario file is not valid JSON: %s (parse error: %s)"), *FilePath,
                                            *Reader->GetErrorMessage()));
        return false;
    }

    const TArray<TSharedPtr<FJsonValue>> *StepValues = nullptr;
    if (!RootObject->TryGetArrayField(TEXT("steps"), StepValues))
    {
        FScenarioLog::Error(FString::Printf(TEXT("Scenario file has no 'steps' array: %s"), *FilePath));
        return false;
    }

    for (const TSharedPtr<FJsonValue> &StepValue : *StepValues)
    {
        const int32 StepIndex = OutSteps.Num();
        const TSharedPtr<FJsonObject> *StepObject = nullptr;
        if (!StepValue->TryGetObject(StepObject))
        {
            FScenarioLog::Error(FString::Printf(TEXT("Scenario step[%d] is not an object: %s"), StepIndex, *FilePath));
            return false;
        }

        FScenarioStep Step;
        FString ActionName;
        if (!(*StepObject)->TryGetNumberField(TEXT("duration"), Step.Duration) ||
            !(*StepObject)->TryGetStringField(TEXT("action"), ActionName))
        {
            FScenarioLog::Error(
                FString::Printf(TEXT("Scenario step[%d] needs 'duration' and 'action': %s"), StepIndex, *FilePath));
            return false;
        }

        if (!ParseAction(ActionName, Step.Action))
        {
            TArray<FString> ValidNames;
            const UEnum *Enum = StaticEnum<EScenarioAction>();
            for (int32 Index = 0; Index < Enum->NumEnums() - 1; ++Index)
            {
                ValidNames.Add(Enum->GetNameStringByIndex(Index));
            }
            FScenarioLog::Error(FString::Printf(TEXT("Unknown scenario action '%s' in step[%d] of %s (valid: %s)"),
                                                *ActionName, StepIndex, *FilePath,
                                                *FString::Join(ValidNames, TEXT(", "))));
            return false;
        }

        OutSteps.Add(Step);
    }

    FScenarioLog::Info(FString::Printf(TEXT("Loaded %d scenario steps from %s"), OutSteps.Num(), *FilePath));
    for (int32 Index = 0; Index < OutSteps.Num(); ++Index)
    {
        FScenarioLog::Info(FString::Printf(TEXT("  step[%d] duration=%.2fs action=%s"), Index, OutSteps[Index].Duration,
                                           *ScenarioActionName(OutSteps[Index].Action)));
    }
    return true;
}

auto FScenarioLoader::ParseAction(const FString &ActionName, EScenarioAction &OutAction) -> bool
{
    // match against the enum entry names so new or renamed actions need no loader changes
    const int64 Value = StaticEnum<EScenarioAction>()->GetValueByNameString(ActionName.TrimStartAndEnd());
    if (Value == INDEX_NONE)
    {
        return false;
    }

    OutAction = static_cast<EScenarioAction>(Value);
    return true;
}
