#include "EnvironmentBlackout.h"

#include "Engine/Light.h"
#include "Engine/SkyLight.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"

/** True if the actor is the kept pawn or attached (directly or transitively) to it.
 *  Named statics instead of an anonymous namespace: this module unity-builds, and
 *  same-named anonymous-namespace symbols collide across merged translation units. */
static auto BlackoutIsPartOfPawn(const AActor *Actor, const APawn *KeepVisiblePawn) -> bool
{
    for (const AActor *Current = Actor; Current != nullptr; Current = Current->GetAttachParentActor())
    {
        if (Current == KeepVisiblePawn)
        {
            return true;
        }
    }
    return false;
}

auto FEnvironmentBlackout::Apply(UWorld &World, const APawn *KeepVisiblePawn) -> int32
{
    if (bActive)
    {
        return 0;
    }

    int32 NumHidden = 0;
    for (TActorIterator<AActor> It(&World); It; ++It)
    {
        AActor *Actor = *It;

        // lights stay visible so the drone remains lit; already-hidden actors are
        // skipped so Restore() does not unhide them
        if (Actor->IsHidden() || BlackoutIsPartOfPawn(Actor, KeepVisiblePawn) || Actor->IsA<ALight>() ||
            Actor->IsA<ASkyLight>())
        {
            continue;
        }

        Actor->SetActorHiddenInGame(true);
        HiddenActors.Add(Actor);
        ++NumHidden;
    }

    bActive = true;
    return NumHidden;
}

void FEnvironmentBlackout::Restore()
{
    for (const TWeakObjectPtr<AActor> &Actor : HiddenActors)
    {
        if (Actor.IsValid())
        {
            Actor->SetActorHiddenInGame(false);
        }
    }

    HiddenActors.Empty();
    bActive = false;
}
