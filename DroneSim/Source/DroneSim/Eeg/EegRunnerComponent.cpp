#include "EegRunnerComponent.h"
#include "DronePhysicsConfig.h"
#include "EegProto.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "ScenarioLog.h"

namespace
{
/** Label published while the server connection is not up; not an action name */
const TCHAR *ConnectingLabel = TEXT("CONNECTING");

/** Stick travel below which an axis reads zero: a real classifier never balances its
 *  opposing probabilities exactly, and without a deadzone that residual would drift
 *  the drone while the inference says STOP */
constexpr float AxisDeadzone = 0.1f;

auto ApplyAxisDeadzone(float Axis) -> float
{
    return FMath::Abs(Axis) < AxisDeadzone ? 0.0f : Axis;
}
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

void UEegRunnerComponent::NotifySettingsSaved()
{
    if (Physics.IsActive())
    {
        Physics.UpdateSettings(UDronePhysicsConfig::Get()->Settings);
    }
    if (Client.IsConnected())
    {
        SendPhysicsSettings();
    }
}

void UEegRunnerComponent::SendPhysicsSettings()
{
    Client.SendPayload(
        EegProto::EncodePhysicsSettings(UDronePhysicsConfig::Get()->Settings, UDronePhysicsConfig::Get()->PresetName));
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
            // stale commands must not keep the drone moving
            CurrentAction = EScenarioAction::Stop;
            FMemory::Memzero(LastActionProbs);
            PublishActionLabel(ConnectingLabel);
        }
        else
        {
            // every (re)connect: show the active physics setup on the dashboard
            SendPhysicsSettings();
        }
    }

    // 2) advance the simulated device and stream completed frames to the server
    Simulator.Tick(DeltaTime, PendingFrames);
    for (const FEegFrame &Frame : PendingFrames)
    {
        Client.SendPayload(EegProto::EncodeEegFrame(Frame, EegProto::NowUnixMs()));
    }
    PendingFrames.Reset();

    // 3) apply the inference as two-axis transmitter input: each axis is the net
    // probability of its opposing actions, a throttle in [-1, 1], so the drone can fly
    // diagonals and partial speeds instead of snapping to the single winning action
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

        const float PitchAxis = ApplyAxisDeadzone(LastActionProbs[EegConfig::ProbIndex(EScenarioAction::Forward)] -
                                                  LastActionProbs[EegConfig::ProbIndex(EScenarioAction::Backward)]);
        const float RollAxis = ApplyAxisDeadzone(LastActionProbs[EegConfig::ProbIndex(EScenarioAction::Right)] -
                                                 LastActionProbs[EegConfig::ProbIndex(EScenarioAction::Left)]);

        // the stick frame follows the drone's own heading: in mouse-yaw mode the pawn
        // yaw tracks the control rotation anyway, and in camera-follow mode the
        // decoupled control rotation must not steer the movement
        const FDronePhysicsSettings &PhysicsSettings = Physics.GetSettings();
        const FRotator YawRotation(0.0f, Pawn->GetActorRotation().Yaw, 0.0f);
        const FVector ForwardDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        if (PhysicsSettings.bTurnWithLeftRight)
        {
            // mode-2 style: the roll axis becomes the rudder and throttles the turn rate
            Physics.SetMoveInput(ForwardDir * PitchAxis);
            Physics.SetYawRate(RollAxis * FMath::DegreesToRadians(PhysicsSettings.TurnRateDegS));
        }
        else
        {
            Physics.SetMoveInput(ForwardDir * PitchAxis + RightDir * RollAxis);
            Physics.SetYawRate(0.0);
        }

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
