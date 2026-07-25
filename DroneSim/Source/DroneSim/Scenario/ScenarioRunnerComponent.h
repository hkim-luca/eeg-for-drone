#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "DronePhysics.h"
#include "ScenarioTypes.h"

#include "ScenarioRunnerComponent.generated.h"

class APawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnScenarioFinished);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScenarioActionChanged, const FString &, ActionLabel);

/** A single position sample recorded during scenario playback */
struct FPositionSample
{
    /** Wall-clock time in KST (UTC+9) at the moment of sampling; written to CSV as ISO 8601 with +09:00 offset */
    FDateTime Timestamp;
    float Time = 0.0f;
    /** Action label active at the moment of sampling */
    FString Action;
    FVector Location = FVector::ZeroVector;
};

/**
 *  Plays back scenario steps on the pawn possessed by the owning player controller
 *  and records the pawn position at a fixed sampling resolution.
 *  Steps run in order; after each step the runner waits until the drone has
 *  physically settled (stopped and level) before starting the next one.
 *  The CSV at Saved/Recordings/<FileName>.csv is created when Start() is called and
 *  appended to every time the action changes (action-to-action transitions and the
 *  action/WAIT boundary), so most of the recording is on disk before playback ends.
 */
UCLASS(ClassGroup = (Scenario), meta = (BlueprintSpawnableComponent))
class UScenarioRunnerComponent : public UActorComponent
{
    GENERATED_BODY()

  public:
    UScenarioRunnerComponent();

    /** Starts playback of the given steps, sampling positions every InSampleInterval seconds and
     *  writing them incrementally to Saved/Recordings/<InFileName>.csv.
     *  InMoveSpeed > 0 overrides the pawn's max walk/fly speed (cm/s) for the run. */
    void Start(const TArray<FScenarioStep> &InSteps, float InSampleInterval, float InMoveSpeed,
               const FString &InFileName);

    /** Returns the absolute path of the CSV file being written by the current/last run,
     *  or an empty string if writing it failed. */
    auto GetRecordingPath() const -> const FString &;

    /** Re-reads UDronePhysicsConfig into the active flight physics (settings UI live edit) */
    void NotifySettingsChanged();

    /** Fired once when playback has passed the end of the last step */
    UPROPERTY(BlueprintAssignable, Category = "Scenario")
    FOnScenarioFinished OnScenarioFinished;

    /** Fired when the currently applied action changes; label is LEFT, RIGHT, FRONTWARD, BACKWARD, STOP or WAIT */
    UPROPERTY(BlueprintAssignable, Category = "Scenario")
    FOnScenarioActionChanged OnActionChanged;

    /** WGS84 latitude of the UE world origin (defaults to Daejeon) */
    UPROPERTY(EditAnywhere, Category = "Scenario|Geo")
    double OriginLatitude = 36.3504;

    /** WGS84 longitude of the UE world origin (defaults to Daejeon) */
    UPROPERTY(EditAnywhere, Category = "Scenario|Geo")
    double OriginLongitude = 127.3845;

    /** Altitude of the UE world origin in meters */
    UPROPERTY(EditAnywhere, Category = "Scenario|Geo")
    double OriginAltitude = 0.0;

    void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

  private:
    /** Playback phase: applying the current step's action, or settling before the next step */
    enum class EPhase : uint8
    {
        Action,
        Settle
    };

    /** Advances the step state machine and applies the current step's movement input */
    void UpdatePlayback(float DeltaTime);

    /** Logs and broadcasts the action label when it changes */
    void PublishActionLabel(const FString &Label);

    /** Records position samples for every sample point passed since the last tick */
    void SamplePosition();

    /** Returns the pawn currently possessed by the owning player controller */
    auto GetControlledPawn() const -> APawn *;

    /** Creates (truncates) RecordingFilePath and writes the CSV header; clears it on failure */
    void BeginRecordingFile(const FString &FileName);

    /** Appends buffered Samples to RecordingFilePath and clears the buffer; call on every action
     *  change so the file stays close to up to date, and once more when playback finishes */
    void FlushSamples();

    /** Steps to play, sorted by time */
    TArray<FScenarioStep> Steps;

    /** Drone physics driver; active between Start() and the end of playback */
    FDronePhysics Physics;

    /** Position samples not yet flushed to RecordingFilePath */
    TArray<FPositionSample> Samples;

    /** Absolute path of the CSV file being written this run; empty once writing has failed */
    FString RecordingFilePath;

    /** Label broadcast for the last applied action */
    FString LastActionLabel;

    /** Seconds between two position samples (recording resolution) */
    float SampleInterval = 0.1f;

    /** Playback time of the next pending sample */
    float NextSampleTime = 0.0f;

    /** Elapsed playback time in seconds */
    float Elapsed = 0.0f;

    /** Current playback phase */
    EPhase Phase = EPhase::Action;

    /** Index of the step currently applied or settled from; Steps.Num() when done */
    int32 CurrentStepIndex = 0;

    /** Seconds the current step's action has been applied */
    float StepElapsed = 0.0f;

    /** Seconds spent settling after the current step */
    float SettleElapsed = 0.0f;

    /** True while a scenario is playing */
    bool bRunning = false;

    /** Prevents the missing-pawn error from being logged every tick */
    bool bWarnedNoPawn = false;
};
