#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "DronePhysics.h"
#include "EegClient.h"
#include "EegSignalSimulator.h"
#include "EegTypes.h"

#include "EegRunnerComponent.generated.h"

class APawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEegActionChanged, const FString &, ActionLabel);

/**
 *  Drives the EEG running mode on the owning player controller's pawn:
 *  every tick it pumps the simulated 32-electrode EEG device, streams the completed
 *  100 ms frames to the EEG server, applies the last EScenarioAction the server inferred
 *  as movement input (with the same drone physics as scenario playback), and confirms
 *  each applied action back to the server so it can measure inference-to-control latency.
 *  Runs until Stop(); the connection reconnects automatically if the server restarts.
 */
UCLASS(ClassGroup = (Eeg), meta = (BlueprintSpawnableComponent))
class UEegRunnerComponent : public UActorComponent
{
    GENERATED_BODY()

  public:
    UEegRunnerComponent();

    /** Starts running mode; InMoveSpeed > 0 overrides the pawn's max speed (cm/s) */
    void Start(float InMoveSpeed);

    /** Stops running mode, restores the pawn physics and closes the server connection */
    void Stop();

    /** Closes the server connection deterministically when play ends (PIE stop, level
     *  transition, actor destruction); FEegClient's destructor alone is not enough because
     *  UObject teardown is deferred to garbage collection, which can leave the socket open
     *  long after the dashboard should have seen a disconnect */
    void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** True between Start() and Stop() */
    auto IsRunning() const -> bool;

    /** True while the EEG server connection is established */
    auto IsServerConnected() const -> bool;

    /** Simulated device, exposed read-only for the electrode graph widget */
    auto GetSimulator() const -> const FEegSignalSimulator &;

    /** Per-action probabilities of the last result in [0, 1], ordered by EegConfig::ProbOrder;
     *  all zero until the first action result arrives */
    auto GetLastActionProbs() const -> TConstArrayView<float>;

    /** Rolling latency/reliability metrics of the last result, mirrored from the dashboard
     *  KPI tiles; all zero until the first action result arrives */
    auto GetLastMetrics() const -> const FEegMetrics &;

    /** Metrics history for the EEG HUD's chart, oldest first, capped at MetricsHistoryCapacity
     *  samples (one per received action result, ~10 Hz) */
    auto GetMetricsHistory() const -> TConstArrayView<FEegMetrics>;

    /** Number of samples kept in the metrics history (~30 s at the ~10 Hz action-result rate) */
    static constexpr int32 MetricsHistoryCapacity = 300;

    /** Re-reads UDronePhysicsConfig into the active flight physics (settings UI live edit) */
    void NotifySettingsChanged();

    /** Current visual body tilt (banking roll, nose pitch) applied by the drone physics;
     *  the pawn's own actor rotation never rolls or pitches, only this cosmetic mesh tilt */
    auto GetCurrentTilt() const -> FRotator;

    void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

    /** Fired when the applied action label changes; labels match scenario playback */
    UPROPERTY(BlueprintAssignable, Category = "EEG")
    FOnEegActionChanged OnActionChanged;

    /** Host of the Python EEG server */
    UPROPERTY(EditAnywhere, Category = "EEG")
    FString ServerHost = TEXT("127.0.0.1");

    /** TCP port of the Python EEG server */
    UPROPERTY(EditAnywhere, Category = "EEG")
    int32 ServerPort = 9800;

  private:
    /** Handles one server payload: adopts the inferred action and queues its ack */
    void HandleServerMessage(TConstArrayView<uint8> Payload);

    /** Broadcasts the action label when it changes */
    void PublishActionLabel(const FString &Label);

    /** Returns the pawn currently possessed by the owning player controller */
    auto GetControlledPawn() const -> APawn *;

    /** Simulated EEG device (stands in for real hardware) */
    FEegSignalSimulator Simulator;

    /** TCP link to the Python EEG server */
    FEegClient Client;

    /** Drone flight physics driver; active between Start() and Stop() */
    FDronePhysics Physics;

    /** Frames completed by the simulator this tick; kept as a member to reuse the allocation */
    TArray<FEegFrame> PendingFrames;

    /** Action results received this tick, acked after the movement input is applied */
    TArray<FEegActionResult> PendingAcks;

    /** Action currently applied to the pawn; updated by server messages */
    EScenarioAction CurrentAction = EScenarioAction::Stop;

    /** Per-action probabilities of the last result, ordered by EegConfig::ProbOrder */
    float LastActionProbs[EegConfig::ProbCount] = {};

    /** Rolling latency/reliability metrics of the last result */
    FEegMetrics LastMetrics;

    /** Metrics history for the chart, oldest first; capped at MetricsHistoryCapacity */
    TArray<FEegMetrics> MetricsHistory;

    /** Label broadcast for the last applied action */
    FString LastActionLabel;

    /** Speed override passed to Start(), kept so the physics can begin late when the pawn
     *  is possessed only after running mode already started */
    float MoveSpeed = 0.0f;

    /** True between Start() and Stop() */
    bool bRunning = false;

    /** True once the server connection was seen up, to publish reconnect transitions */
    bool bWasConnected = false;

    /** Prevents the missing-pawn error from being logged every tick */
    bool bWarnedNoPawn = false;
};
