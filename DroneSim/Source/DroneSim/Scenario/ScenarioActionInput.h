#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ScenarioTypes.h"

/** Applies one scenario action as movement input relative to the given yaw rotation.
 *  Shared by scenario playback (ScenarioRunnerComponent) and EEG running mode
 *  (EegRunnerComponent) so both modes steer the pawn identically. */
inline void ApplyScenarioActionInput(EScenarioAction Action, APawn &Pawn, const FRotator &YawRotation)
{
    const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    switch (Action)
    {
    case EScenarioAction::Left:
        Pawn.AddMovementInput(Right, -1.0f);
        break;
    case EScenarioAction::Right:
        Pawn.AddMovementInput(Right, 1.0f);
        break;
    case EScenarioAction::Forward:
        Pawn.AddMovementInput(Forward, 1.0f);
        break;
    case EScenarioAction::Backward:
        Pawn.AddMovementInput(Forward, -1.0f);
        break;
    case EScenarioAction::Stop:
        // no movement input; braking stops the pawn
        break;
    }
}
