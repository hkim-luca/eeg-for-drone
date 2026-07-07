#pragma once

#include "CoreMinimal.h"
#include "ScenarioTypes.h"

/** Returns the world-space movement direction of one scenario action relative to the
 *  given yaw rotation; zero for Stop. When bTurnWithLeftRight is set, Left/Right are
 *  turn commands (see ScenarioActionYawRate) and translate nothing. Shared by
 *  scenario playback and EEG running mode so both feed FDronePhysics identically. */
inline auto ScenarioActionDirection(EScenarioAction Action, const FRotator &YawRotation,
                                    bool bTurnWithLeftRight = false) -> FVector
{
    const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    switch (Action)
    {
    case EScenarioAction::Left:
        return bTurnWithLeftRight ? FVector::ZeroVector : -Right;
    case EScenarioAction::Right:
        return bTurnWithLeftRight ? FVector::ZeroVector : Right;
    case EScenarioAction::Forward:
        return Forward;
    case EScenarioAction::Backward:
        return -Forward;
    case EScenarioAction::Stop:
    default:
        return FVector::ZeroVector;
    }
}

/** Returns the yaw rate (rad/s, positive turns right) of one scenario action for
 *  FDronePhysics::SetYawRate: Left/Right yaw at TurnRateDegS while
 *  bTurnWithLeftRight is set, every other action and the strafe mode return zero. */
inline auto ScenarioActionYawRate(EScenarioAction Action, bool bTurnWithLeftRight, double TurnRateDegS) -> double
{
    if (!bTurnWithLeftRight)
    {
        return 0.0;
    }

    switch (Action)
    {
    case EScenarioAction::Left:
        return -FMath::DegreesToRadians(TurnRateDegS);
    case EScenarioAction::Right:
        return FMath::DegreesToRadians(TurnRateDegS);
    default:
        return 0.0;
    }
}
