#include "EegRunnerComponent.h"
#include "DronePhysicsConfig.h"
#include "EegProto.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "ScenarioActionInput.h"
#include "ScenarioLog.h"

namespace
{
/** Label published while the server connection is not up; not an action name */
const TCHAR *ConnectingLabel = TEXT("CONNECTING");
} // namespace

UEegRunnerComponent::UEegRunnerComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UEegRunnerComponent::Start(float InMoveSpeed)
{
    CurrentAction = EScenarioAction::Stop;
    FMemory::Memzero(LastActionProbs);
    LastMetrics = FEegMetrics();
    MetricsHistory.Reset();
    LastActionLabel.Reset();
    PendingFrames.Reset();
    PendingAcks.Reset();
    bWasConnected = false;
    bWarnedNoPawn = false;
    MoveSpeed = InMoveSpeed;

    // seed from the clock: the noise should differ between runs, unlike unit-style tests
    Simulator.Start(static_cast<int32>(FPlatformTime::Cycles()));
    Client.Connect(ServerHost, ServerPort);

    if (ACharacter *Character = Cast<ACharacter>(GetControlledPawn()))
    {
        Physics.Begin(*Character, MoveSpeed, UDronePhysicsConfig::Get()->Settings);
    }

    FScenarioLog::Info(FString::Printf(TEXT("EEG running mode start: server=%s:%d channels=%d rate=%dHz speed=%.0f"),
                                       *ServerHost, ServerPort, EegConfig::ChannelCount, EegConfig::SampleRateHz,
                                       InMoveSpeed));

    PublishActionLabel(ConnectingLabel);
    bRunning = true;
    SetComponentTickEnabled(true);
}

void UEegRunnerComponent::Stop()
{
    if (!bRunning)
    {
        return;
    }

    bRunning = false;
    SetComponentTickEnabled(false);
    Physics.End();
    Client.Disconnect();
    FScenarioLog::Info(FString::Printf(TEXT("EEG running mode stopped; messages dropped while disconnected: %lld"),
                                       Client.GetDroppedMessageCount()));
}

void UEegRunnerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Stop();
    Super::EndPlay(EndPlayReason);
}

auto UEegRunnerComponent::IsRunning() const -> bool
{
    return bRunning;
}

auto UEegRunnerComponent::IsServerConnected() const -> bool
{
    return Client.IsConnected();
}

auto UEegRunnerComponent::GetSimulator() const -> const FEegSignalSimulator &
{
    return Simulator;
}

auto UEegRunnerComponent::GetLastActionProbs() const -> TConstArrayView<float>
{
    return TConstArrayView<float>(LastActionProbs, EegConfig::ProbCount);
}

auto UEegRunnerComponent::GetLastMetrics() const -> const FEegMetrics &
{
    return LastMetrics;
}

auto UEegRunnerComponent::GetMetricsHistory() const -> TConstArrayView<FEegMetrics>
{
    return MetricsHistory;
}

void UEegRunnerComponent::NotifySettingsChanged()
{
    if (Physics.IsActive())
    {
        Physics.UpdateSettings(UDronePhysicsConfig::Get()->Settings);
    }
    if (Client.IsConnected())
    {
        Client.SendPayload(EegProto::EncodePhysicsSettings(UDronePhysicsConfig::Get()->Settings,
                                                           UDronePhysicsConfig::Get()->PresetName));
    }
}

auto UEegRunnerComponent::GetCurrentTilt() const -> FRotator
{
    return Physics.GetCurrentTilt();
}

void UEegRunnerComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                        FActorComponentTickFunction *ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bRunning)
    {
        return;
    }

    // 1) pump the network first so this tick applies the freshest inferred action
    Client.Tick(FPlatformTime::Seconds(), [this](TConstArrayView<uint8> Payload) { HandleServerMessage(Payload); });

    if (Client.IsConnected() != bWasConnected)
    {
        bWasConnected = Client.IsConnected();
        if (!bWasConnected)
        {
            CurrentAction = EScenarioAction::Stop; // stale commands must not keep the drone moving
            PublishActionLabel(ConnectingLabel);
        }
        else
        {
            // every (re)connect: show the active physics setup on the dashboard
            Client.SendPayload(EegProto::EncodePhysicsSettings(UDronePhysicsConfig::Get()->Settings,
                                                               UDronePhysicsConfig::Get()->PresetName));
        }
    }

    // 2) advance the simulated device and stream completed frames to the server
    Simulator.Tick(DeltaTime, PendingFrames);
    for (const FEegFrame &Frame : PendingFrames)
    {
        Client.SendPayload(EegProto::EncodeEegFrame(Frame, EegProto::NowUnixMs()));
    }
    PendingFrames.Reset();

    // 3) apply the inferred action to the pawn with the shared scenario movement rules
    APawn *Pawn = GetControlledPawn();
    if (Pawn != nullptr)
    {
        // the pawn may be possessed only after Start() already ran (BeginPlay ordering),
        // in which case the physics never began; begin it as soon as the pawn shows up
        if (!Physics.IsActive())
        {
            if (ACharacter *Character = Cast<ACharacter>(Pawn))
            {
                Physics.Begin(*Character, MoveSpeed, UDronePhysicsConfig::Get()->Settings);
            }
        }

        const APlayerController *Controller = GetWorld()->GetFirstPlayerController();
        const FRotator YawRotation(0.0f, Controller != nullptr ? Controller->GetControlRotation().Yaw : 0.0f, 0.0f);
        Physics.SetMoveDirection(ScenarioActionDirection(CurrentAction, YawRotation));
        if (bWasConnected)
        {
            PublishActionLabel(ScenarioActionName(CurrentAction));
        }
    }
    else if (!bWarnedNoPawn)
    {
        bWarnedNoPawn = true;
        FScenarioLog::Error(TEXT("EEG running mode: no pawn possessed; movement skipped"));
    }

    Physics.Tick(DeltaTime);

    // 4) confirm every action applied this tick so the server can measure latency
    for (const FEegActionResult &Result : PendingAcks)
    {
        Client.SendPayload(EegProto::EncodeControlAck(Result.ActionSeq, EegProto::NowUnixMs()));
    }
    PendingAcks.Reset();
}

void UEegRunnerComponent::HandleServerMessage(TConstArrayView<uint8> Payload)
{
    FEegActionResult Result;
    if (!EegProto::DecodeServerMessage(Payload, Result))
    {
        FScenarioLog::Error(FString::Printf(TEXT("EEG server sent an undecodable message (%d bytes)"), Payload.Num()));
        return;
    }

    CurrentAction = Result.Action;
    FMemory::Memcpy(LastActionProbs, Result.ActionProbs, sizeof(LastActionProbs));

    FEegMetrics MetricsWithInferDuration = Result.Metrics;
    MetricsWithInferDuration.InferDurationMs = static_cast<float>(Result.InferDurationMs);
    LastMetrics = MetricsWithInferDuration;

    MetricsHistory.Add(MetricsWithInferDuration);
    if (MetricsHistory.Num() > MetricsHistoryCapacity)
    {
        MetricsHistory.RemoveAt(0, MetricsHistory.Num() - MetricsHistoryCapacity, EAllowShrinking::No);
    }

    PendingAcks.Add(Result);
}

void UEegRunnerComponent::PublishActionLabel(const FString &Label)
{
    if (Label == LastActionLabel)
    {
        return;
    }

    LastActionLabel = Label;
    FScenarioLog::Info(FString::Printf(TEXT("EEG action=%s"), *Label));
    OnActionChanged.Broadcast(Label);
}

auto UEegRunnerComponent::GetControlledPawn() const -> APawn *
{
    // hosted on the systems actor now, not the controller: drive whichever pawn the
    // local player currently possesses
    const UWorld *World = GetWorld();
    const APlayerController *Controller = World != nullptr ? World->GetFirstPlayerController() : nullptr;
    return Controller != nullptr ? Controller->GetPawn() : nullptr;
}
