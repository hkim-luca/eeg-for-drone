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
 */
UCLASS(ClassGroup = (Scenario), meta = (BlueprintSpawnableComponent))
class UScenarioRunnerComponent : public UActorComponent
{
    GENERATED_BODY()

  public:
    UScenarioRunnerComponent();

    /** Starts playback of the given steps, sampling positions every InSampleInterval seconds.
     *  InMoveSpeed > 0 overrides the pawn's max walk/fly speed (cm/s) for the run. */
    void Start(const TArray<FScenarioStep> &InSteps, float InSampleInterval, float InMoveSpeed);

    /** Writes the recorded samples as CSV to Saved/Recordings/<FileName>.csv. Returns the saved path or an empty string
     * on failure. */
    auto SaveRecording(const FString &FileName) const -> FString;

    /** Returns the absolute CSV path Saved/Recordings/<sanitized FileName>.csv */
    static auto MakeRecordingPath(const FString &FileName) -> FString;

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

    /** Drone flight physics applied during playback: inertia, drift and body tilt */
    UPROPERTY(EditAnywhere, Category = "Scenario|Physics")
    FDronePhysicsSettings PhysicsSettings;

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

    /** Applies one action as movement input relative to the controller's yaw rotation */
    static void ApplyAction(EScenarioAction Action, APawn &Pawn, const FRotator &YawRotation);

    /** Logs and broadcasts the action label when it changes */
    void PublishActionLabel(const FString &Label);

    /** Records position samples for every sample point passed since the last tick */
    void SamplePosition();

    /** Returns the pawn currently possessed by the owning player controller */
    auto GetControlledPawn() const -> APawn *;

    /** Steps to play, sorted by time */
    TArray<FScenarioStep> Steps;

    /** Drone physics driver; active between Start() and the end of playback */
    FDronePhysics Physics;

    /** Recorded position samples */
    TArray<FPositionSample> Samples;

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
