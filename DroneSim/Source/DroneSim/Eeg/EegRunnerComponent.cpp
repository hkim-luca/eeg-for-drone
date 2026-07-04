#include "EegRunnerComponent.h"
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
        Physics.Begin(*Character, MoveSpeed, PhysicsSettings);
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
                Physics.Begin(*Character, MoveSpeed, PhysicsSettings);
            }
        }

        const APlayerController *Controller = Cast<APlayerController>(GetOwner());
        const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
        ApplyScenarioActionInput(CurrentAction, *Pawn, YawRotation);
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
    const APlayerController *Controller = Cast<APlayerController>(GetOwner());
    return Controller != nullptr ? Controller->GetPawn() : nullptr;
}
