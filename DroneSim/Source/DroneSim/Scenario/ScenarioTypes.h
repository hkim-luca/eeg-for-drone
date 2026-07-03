#pragma once

#include "CoreMinimal.h"

#include "ScenarioTypes.generated.h"

/** Movement actions a scenario step can apply to the controlled character */
UENUM(BlueprintType)
enum class EScenarioAction : uint8
{
    Left,
    Right,
    Forward,
    Backward,
    Stop
};

/** A single scenario step: apply Action for Duration seconds. Steps run in file order;
 *  the next step starts only after the drone has physically settled (stopped and level). */
USTRUCT(BlueprintType)
struct FScenarioStep
{
    GENERATED_BODY()

    /** How long the action is applied */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scenario", meta = (ForceUnits = "s"))
    float Duration = 0.0f;

    /** Movement action to apply */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scenario")
    EScenarioAction Action = EScenarioAction::Stop;
};

/** Action label taken from the enum entry name (e.g. FRONTWARD); the single source used by
 *  the HUD, logs and the recording CSV, so renaming or adding actions never needs code here. */
inline auto ScenarioActionName(EScenarioAction Action) -> FString
{
    return StaticEnum<EScenarioAction>()->GetNameStringByValue(static_cast<int64>(Action)).ToUpper();
}

/** Parses the uppercase wire label (LEFT, RIGHT, FORWARD, BACKWARD, STOP) written by
 *  ScenarioActionName() back to the enum; returns false if the label is unknown */
inline auto ParseScenarioActionName(const FString &Label, EScenarioAction &OutAction) -> bool
{
    const UEnum *Enum = StaticEnum<EScenarioAction>();
    for (int32 Index = 0; Index < Enum->NumEnums() - 1; ++Index)
    {
        if (Enum->GetNameStringByIndex(Index).ToUpper() == Label)
        {
            OutAction = static_cast<EScenarioAction>(Enum->GetValueByIndex(Index));
            return true;
        }
    }
    return false;
}
