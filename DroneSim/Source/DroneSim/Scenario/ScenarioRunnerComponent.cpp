#include "ScenarioRunnerComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ScenarioLog.h"
#include "ScenarioTime.h"

namespace
{
/** Label reported while settling between actions; not an action name */
const TCHAR *SettlingLabel = TEXT("WAIT");

/** Settling is abandoned after this many seconds so a stuck pawn cannot hang the run */
constexpr float MaxSettleSeconds = 20.0f;
} // namespace

UScenarioRunnerComponent::UScenarioRunnerComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UScenarioRunnerComponent::Start(const TArray<FScenarioStep> &InSteps, float InSampleInterval, float InMoveSpeed)
{
    Steps = InSteps;
    SampleInterval = FMath::Max(InSampleInterval, 0.01f);
    Samples.Reset();
    LastActionLabel.Reset();
    Elapsed = 0.0f;
    NextSampleTime = 0.0f;
    Phase = EPhase::Action;
    CurrentStepIndex = 0;
    StepElapsed = 0.0f;
    SettleElapsed = 0.0f;

    // apply drone flight physics (inertia, drift, body tilt) for the run
    if (ACharacter *Character = Cast<ACharacter>(GetControlledPawn()))
    {
        Physics.Begin(*Character, InMoveSpeed, PhysicsSettings);
    }

    const APawn *Pawn = GetControlledPawn();
    FScenarioLog::Info(FString::Printf(
        TEXT("Playback start: steps=%d sampleInterval=%.3fs speedOverride=%.0f accel=%.0f braking=%.0f "
             "pawn=%s at %s"),
        Steps.Num(), SampleInterval, InMoveSpeed, PhysicsSettings.Acceleration, PhysicsSettings.BrakingDeceleration,
        Pawn != nullptr ? *Pawn->GetClass()->GetName() : TEXT("NULL"),
        Pawn != nullptr ? *Pawn->GetActorLocation().ToString() : TEXT("-")));

    bWarnedNoPawn = false;
    bRunning = true;
    SetComponentTickEnabled(true);
}

void UScenarioRunnerComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                             FActorComponentTickFunction *ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bRunning)
    {
        return;
    }

    Elapsed += DeltaTime;

    UpdatePlayback(DeltaTime);
    Physics.Tick(DeltaTime);
    SamplePosition();

    if (CurrentStepIndex >= Steps.Num())
    {
        bRunning = false;
        SetComponentTickEnabled(false);
        Physics.End();
        FScenarioLog::Info(
            FString::Printf(TEXT("Playback finished: elapsed=%.2fs samples=%d"), Elapsed, Samples.Num()));
        OnScenarioFinished.Broadcast();
    }
}

void UScenarioRunnerComponent::UpdatePlayback(float DeltaTime)
{
    APawn *Pawn = GetControlledPawn();
    if (Pawn == nullptr)
    {
        if (!bWarnedNoPawn)
        {
            bWarnedNoPawn = true;
            FScenarioLog::Error(
                FString::Printf(TEXT("No pawn possessed at t=%.2fs; movement and sampling skipped"), Elapsed));
        }
        return;
    }

    if (CurrentStepIndex >= Steps.Num())
    {
        return;
    }

    const FScenarioStep &Step = Steps[CurrentStepIndex];

    if (Phase == EPhase::Action)
    {
        const APlayerController *Controller = Cast<APlayerController>(GetOwner());
        const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
        ApplyAction(Step.Action, *Pawn, YawRotation);
        PublishActionLabel(ScenarioActionName(Step.Action));

        StepElapsed += DeltaTime;
        if (StepElapsed >= Step.Duration)
        {
            Phase = EPhase::Settle;
            SettleElapsed = 0.0f;
        }
        return;
    }

    // settle phase: no input; wait until the drone has stopped and is level again
    PublishActionLabel(SettlingLabel);
    SettleElapsed += DeltaTime;

    if (SettleElapsed >= MaxSettleSeconds)
    {
        FScenarioLog::Error(
            FString::Printf(TEXT("Drone did not settle within %.0fs after step[%d]; forcing the next step"),
                            MaxSettleSeconds, CurrentStepIndex));
    }
    else if (!Physics.IsSettled())
    {
        return;
    }

    ++CurrentStepIndex;
    StepElapsed = 0.0f;
    Phase = EPhase::Action;
}

void UScenarioRunnerComponent::PublishActionLabel(const FString &Label)
{
    if (Label == LastActionLabel)
    {
        return;
    }

    LastActionLabel = Label;
    const APawn *Pawn = GetControlledPawn();
    FScenarioLog::Info(FString::Printf(TEXT("t=%.2fs action=%s pawn at %s"), Elapsed, *Label,
                                       Pawn != nullptr ? *Pawn->GetActorLocation().ToString() : TEXT("-")));
    OnActionChanged.Broadcast(Label);
}

void UScenarioRunnerComponent::ApplyAction(EScenarioAction Action, APawn &Pawn, const FRotator &YawRotation)
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
    case EScenarioAction::Frontward:
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

void UScenarioRunnerComponent::SamplePosition()
{
    const APawn *Pawn = GetControlledPawn();
    if (Pawn == nullptr)
    {
        return;
    }

    const FDateTime NowKst = ScenarioTime::NowKst();

    while (NextSampleTime <= Elapsed)
    {
        FPositionSample Sample;
        Sample.Timestamp = NowKst;
        Sample.Time = NextSampleTime;
        Sample.Action = LastActionLabel;
        Sample.Location = Pawn->GetActorLocation();
        Samples.Add(Sample);

        NextSampleTime += SampleInterval;
    }
}

auto UScenarioRunnerComponent::MakeRecordingPath(const FString &FileName) -> FString
{
    FString CleanName = FPaths::MakeValidFileName(FileName.TrimStartAndEnd());
    if (CleanName.IsEmpty())
    {
        CleanName = TEXT("recording");
    }

    return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Recordings") / CleanName + TEXT(".csv"));
}

auto UScenarioRunnerComponent::SaveRecording(const FString &FileName) const -> FString
{
    // flat-earth conversion around the origin; UE units are cm with +X = east, +Y = south (left-handed)
    constexpr double MetersPerDegreeLatitude = 111320.0;
    const double MetersPerDegreeLongitude =
        MetersPerDegreeLatitude * FMath::Cos(FMath::DegreesToRadians(OriginLatitude));

    FString CsvText = TEXT("time,elapsed,action,lat,lon,alt_m\n");
    for (const FPositionSample &Sample : Samples)
    {
        const double EastMeters = Sample.Location.X / 100.0;
        const double NorthMeters = -Sample.Location.Y / 100.0;
        const double Latitude = OriginLatitude + NorthMeters / MetersPerDegreeLatitude;
        const double Longitude = OriginLongitude + EastMeters / MetersPerDegreeLongitude;
        const double AltitudeMeters = OriginAltitude + Sample.Location.Z / 100.0;

        CsvText += FString::Printf(TEXT("%s,%.3f,%s,%.8f,%.8f,%.3f\n"), *ScenarioTime::ToIso8601Kst(Sample.Timestamp),
                                   Sample.Time, *Sample.Action, Latitude, Longitude, AltitudeMeters);
    }

    const FString FilePath = MakeRecordingPath(FileName);
    if (!IFileManager::Get().MakeDirectory(*FPaths::GetPath(FilePath), true))
    {
        FScenarioLog::Error(FString::Printf(TEXT("Failed to create recording directory: %s (%s)"),
                                            *FPaths::GetPath(FilePath), *FScenarioLog::SystemError()));
        return FString();
    }

    if (!FFileHelper::SaveStringToFile(CsvText, *FilePath))
    {
        FScenarioLog::Error(
            FString::Printf(TEXT("Failed to save recording: %s (%s)"), *FilePath, *FScenarioLog::SystemError()));
        return FString();
    }

    FScenarioLog::Info(
        FString::Printf(TEXT("Saved recording: %s (samples=%d, %d bytes)"), *FilePath, Samples.Num(), CsvText.Len()));
    return FilePath;
}

auto UScenarioRunnerComponent::GetControlledPawn() const -> APawn *
{
    const APlayerController *Controller = Cast<APlayerController>(GetOwner());
    return Controller != nullptr ? Controller->GetPawn() : nullptr;
}
