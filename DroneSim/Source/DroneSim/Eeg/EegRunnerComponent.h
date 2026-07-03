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

    /** True between Start() and Stop() */
    auto IsRunning() const -> bool;

    /** True while the EEG server connection is established */
    auto IsServerConnected() const -> bool;

    /** Simulated device, exposed read-only for the electrode graph widget */
    auto GetSimulator() const -> const FEegSignalSimulator &;

    /** Rolling classification accuracy last reported by the server, in percent;
     *  negative until the first action result arrives */
    auto GetLastAccuracyPercent() const -> float;

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

    /** Drone flight physics applied while running mode is active */
    UPROPERTY(EditAnywhere, Category = "EEG|Physics")
    FDronePhysicsSettings PhysicsSettings;

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

    /** Rolling classification accuracy last reported by the server; negative = none yet */
    float LastAccuracyPercent = -1.0f;

    /** Label broadcast for the last applied action */
    FString LastActionLabel;

    /** True between Start() and Stop() */
    bool bRunning = false;

    /** True once the server connection was seen up, to publish reconnect transitions */
    bool bWasConnected = false;

    /** Prevents the missing-pawn error from being logged every tick */
    bool bWarnedNoPawn = false;
};
