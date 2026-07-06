#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"

#include "DroneSystemsActor.generated.h"

class UEegRunnerComponent;
class UScenarioRunnerComponent;
class UDroneTelemetryComponent;

/**
 *  Level-scoped host for the drone session systems that do not belong on the
 *  player controller: the EEG link + inference-driven pawn driver, scenario
 *  playback/recording, and the flight telemetry that feeds the HUD.
 *
 *  These are game systems, not per-player input concerns, so they live on this
 *  actor in the level instead of the controller. The systems drive whichever
 *  pawn the local player currently possesses (looked up through the world's
 *  first player controller), so they survive a controller swap; they are
 *  recreated with the level, which is fine - the TCP link simply reconnects.
 *
 *  AInfo base: a non-physical manager actor with no transform or rendering.
 *  Keeping the three components together preserves the telemetry -> EEG runner
 *  sibling lookup used for the attitude indicator.
 *
 *  Placement: drop one instance into each gameplay level in the editor (Place
 *  Actors -> "Drone Systems Actor"). EEG running and scenario playback are
 *  disabled in any level that has none.
 */
UCLASS(placeable)
class ADroneSystemsActor : public AInfo
{
    GENERATED_BODY()

  public:
    ADroneSystemsActor();

    /** Returns the systems actor placed in the level, or nullptr if the level has
     *  none (place one in the editor). */
    static auto Get(UWorld *World) -> ADroneSystemsActor *;

    auto GetEegRunner() const -> UEegRunnerComponent *;
    auto GetScenarioRunner() const -> UScenarioRunnerComponent *;
    auto GetTelemetry() const -> UDroneTelemetryComponent *;

  private:
    /** Drives the pawn from EEG server inferences in running mode, owns the TCP link */
    UPROPERTY(VisibleAnywhere, Category = "Drone Systems")
    TObjectPtr<UEegRunnerComponent> EegRunner;

    /** Plays back scenarios on the pawn and records positions */
    UPROPERTY(VisibleAnywhere, Category = "Drone Systems")
    TObjectPtr<UScenarioRunnerComponent> ScenarioRunner;

    /** Tracks the controlled pawn's flight state for the status overlay */
    UPROPERTY(VisibleAnywhere, Category = "Drone Systems")
    TObjectPtr<UDroneTelemetryComponent> DroneTelemetry;
};
