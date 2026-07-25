#include "EegSignalSimulator.h"

namespace
{
/** One demo script entry: hold Action for Seconds, then move on (loops forever) */
struct FDemoScriptEntry
{
    EScenarioAction Action;
    double Seconds;
};

/** Fixed demo script cycled while running mode is active; the rests between movements
 *  give the classifier unambiguous STOP frames and let the drone settle */
constexpr FDemoScriptEntry DemoScript[] = {
    {EScenarioAction::Forward, 4.0},  {EScenarioAction::Stop, 2.0},  {EScenarioAction::Left, 3.0},
    {EScenarioAction::Stop, 2.0},     {EScenarioAction::Right, 3.0}, {EScenarioAction::Stop, 2.0},
    {EScenarioAction::Backward, 3.0}, {EScenarioAction::Stop, 2.0},
};

/** Base amplitude of the alpha/beta rhythms in microvolts */
constexpr float BaseAmplitude = 10.0f;

/** Amplitude multiplier applied to the active action's channel group */
constexpr float ActiveBoost = 2.5f;

/** Amplitude multiplier of an artifact burst; strong enough to confuse the classifier */
constexpr float ArtifactBoost = 2.0f;

/** Chance per 100 ms frame that an artifact burst hits one random channel group */
constexpr float ArtifactChancePerFrame = 0.08f;

/** Standard deviation of the per-sample Gaussian noise in microvolts */
constexpr float NoiseSigma = 4.0f;

/** Alpha (10 Hz) and beta (22 Hz) rhythm frequencies */
constexpr float AlphaHz = 10.0f;
constexpr float BetaHz = 22.0f;

constexpr double SampleStep = 1.0 / static_cast<double>(EegConfig::SampleRateHz);
} // namespace

void FEegSignalSimulator::Start(int32 Seed)
{
    Random.Initialize(Seed);
    SignalTime = 0.0;
    PendingTime = 0.0;
    NextSeq = 0;
    ScriptIndex = 0;
    ScriptElapsed = 0.0;
    ArtifactGroupStart = INDEX_NONE;

    for (int32 Channel = 0; Channel < EegConfig::ChannelCount; ++Channel)
    {
        AlphaPhase[Channel] = Random.FRandRange(0.0f, 2.0f * PI);
        BetaPhase[Channel] = Random.FRandRange(0.0f, 2.0f * PI);
    }

    GraphBuffer.Init(0.0f, EegConfig::ChannelCount * EegConfig::GraphWindowSamples);
    GraphWriteIndex = 0;

    PendingFrame = FEegFrame();
    PendingFrame.Seq = NextSeq;
    PendingFrame.Samples.Reserve(EegConfig::SamplesPerFrame * EegConfig::ChannelCount);
}

void FEegSignalSimulator::Tick(float DeltaTime, TArray<FEegFrame> &OutFrames)
{
    PendingTime += DeltaTime;

    while (PendingTime >= SampleStep)
    {
        PendingTime -= SampleStep;
        GenerateSample();

        if (PendingFrame.Samples.Num() >= EegConfig::SamplesPerFrame * EegConfig::ChannelCount)
        {
            OutFrames.Add(MoveTemp(PendingFrame));
            ++NextSeq;
            OnFrameBoundary();
        }
    }
}

auto FEegSignalSimulator::GetActiveAction() const -> EScenarioAction
{
    return DemoScript[ScriptIndex].Action;
}

auto FEegSignalSimulator::GetGraphBuffer() const -> const TArray<float> &
{
    return GraphBuffer;
}

auto FEegSignalSimulator::GetGraphWriteIndex() const -> int32
{
    return GraphWriteIndex;
}

void FEegSignalSimulator::GenerateSample()
{
    const int32 ActiveGroupStart = EegConfig::GroupStartChannel(GetActiveAction());
    const auto Time = static_cast<float>(SignalTime);

    for (int32 Channel = 0; Channel < EegConfig::ChannelCount; ++Channel)
    {
        float Amplitude = BaseAmplitude;
        if (ActiveGroupStart != INDEX_NONE && Channel >= ActiveGroupStart &&
            Channel < ActiveGroupStart + EegConfig::GroupChannelCount)
        {
            Amplitude *= ActiveBoost;
        }
        if (ArtifactGroupStart != INDEX_NONE && Channel >= ArtifactGroupStart &&
            Channel < ArtifactGroupStart + EegConfig::GroupChannelCount)
        {
            Amplitude *= ArtifactBoost;
        }

        const float Rhythm = 0.8f * FMath::Sin(2.0f * PI * AlphaHz * Time + AlphaPhase[Channel]) +
                             0.4f * FMath::Sin(2.0f * PI * BetaHz * Time + BetaPhase[Channel]);
        const float Value = Amplitude * Rhythm + Gaussian(NoiseSigma);

        PendingFrame.Samples.Add(Value);
        GraphBuffer[Channel * EegConfig::GraphWindowSamples + GraphWriteIndex] = Value;
    }

    GraphWriteIndex = (GraphWriteIndex + 1) % EegConfig::GraphWindowSamples;
    SignalTime += SampleStep;
    ScriptElapsed += SampleStep;
}

void FEegSignalSimulator::OnFrameBoundary()
{
    // move the demo script forward; frames never straddle an action change by more
    // than one frame
    if (ScriptElapsed >= DemoScript[ScriptIndex].Seconds)
    {
        ScriptElapsed = 0.0;
        ScriptIndex = (ScriptIndex + 1) % static_cast<int32>(UE_ARRAY_COUNT(DemoScript));
    }

    // re-roll the artifact burst for the next frame
    ArtifactGroupStart = INDEX_NONE;
    if (Random.FRand() < ArtifactChancePerFrame)
    {
        constexpr EScenarioAction Groups[] = {EScenarioAction::Forward, EScenarioAction::Left, EScenarioAction::Right,
                                              EScenarioAction::Backward};
        ArtifactGroupStart =
            EegConfig::GroupStartChannel(Groups[Random.RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(Groups)) - 1)]);
    }

    PendingFrame = FEegFrame();
    PendingFrame.Seq = NextSeq;
    PendingFrame.Samples.Reserve(EegConfig::SamplesPerFrame * EegConfig::ChannelCount);
}

auto FEegSignalSimulator::Gaussian(float Sigma) -> float
{
    // Box-Muller transform; FRand() is clamped away from 0 to keep the log finite
    const float U1 = FMath::Max(Random.FRand(), 1.0e-6f);
    const float U2 = Random.FRand();
    return Sigma * FMath::Sqrt(-2.0f * FMath::Loge(U1)) * FMath::Cos(2.0f * PI * U2);
}
