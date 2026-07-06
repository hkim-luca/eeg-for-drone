#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtrTemplates.h"

class AActor;
class APawn;
class UWorld;

/**
 *  Hides every scenery actor in the level so the viewport shows only the drone
 *  and the UMG overlays on a black background. Customer-facing option: enabled
 *  from launch with -blackbg or toggled with the B key at runtime.
 *
 *  Kept visible: the player pawn (with anything attached to it) and all light
 *  actors, so the drone stays lit - hiding a light actor switches the light
 *  itself off, not just its editor sprite. Everything else (landscape, static
 *  meshes, sky atmosphere, clouds, fog, Blueprint sky spheres) is hidden with
 *  SetActorHiddenInGame, so collision and physics are unchanged. With the sky
 *  hidden the viewport clears to black.
 *
 *  Restore() unhides only the actors Apply() hid, so actors that were already
 *  hidden in the level stay hidden after a toggle round-trip.
 */
class FEnvironmentBlackout
{
  public:
    /** Hides all scenery actors except KeepVisiblePawn (and its attachments) and
     *  lights; no-op while already active. Returns the number of actors hidden. */
    auto Apply(UWorld &World, const APawn *KeepVisiblePawn) -> int32;

    /** Unhides exactly the actors the last Apply call hid */
    void Restore();

    auto IsActive() const -> bool
    {
        return bActive;
    }

  private:
    /** Actors hidden by Apply; weak so actors destroyed in the meantime are skipped */
    TArray<TWeakObjectPtr<AActor>> HiddenActors;

    bool bActive = false;
};
