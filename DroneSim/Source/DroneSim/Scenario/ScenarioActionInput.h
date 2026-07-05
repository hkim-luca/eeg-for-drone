#pragma once

#include "CoreMinimal.h"
#include "ScenarioTypes.h"

/** Returns the world-space movement direction of one scenario action relative to the
 *  given yaw rotation; zero for Stop. Shared by scenario playback and EEG running mode
 *  so both feed FDronePhysics::SetMoveDirection identically. */
inline auto ScenarioActionDirection(EScenarioAction Action, const FRotator &YawRotation) -> FVector
{
    const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    switch (Action)
    {
    case EScenarioAction::Left:
        return -Right;
    case EScenarioAction::Right:
        return Right;
    case EScenarioAction::Forward:
        return Forward;
    case EScenarioAction::Backward:
        return -Forward;
    case EScenarioAction::Stop:
    default:
        return FVector::ZeroVector;
    }
}
