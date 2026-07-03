#pragma once

#include "CoreMinimal.h"
#include "EegTypes.h"
#include "Math/RandomStream.h"

/**
 *  Stand-in for the not-yet-available EEG device: generates a 32-electrode signal at
 *  250 Hz made of alpha/beta rhythms plus Gaussian noise and occasional artifact bursts.
 *  A fixed demo script cycles through the movement actions; the active action boosts the
 *  amplitude of its channel group (see EegConfig::GroupStartChannel) so the dummy server
 *  can classify the frames back. Completed 100 ms frames are handed out via Tick(), and a
 *  ring buffer of recent samples feeds the on-screen electrode graphs.
 */
class FEegSignalSimulator
{
  public:
    /** Resets time, the demo script and the graph buffer; Seed fixes the noise sequence */
    void Start(int32 Seed);

    /** Advances the signal by DeltaTime and appends every completed frame to OutFrames */
    void Tick(float DeltaTime, TArray<FEegFrame> &OutFrames);

    /** Channel-major graph ring buffer: [Channel * GraphWindowSamples + Index], microvolts */
    auto GetGraphBuffer() const -> const TArray<float> &;

    /** Ring position of the oldest sample in the graph buffer */
    auto GetGraphWriteIndex() const -> int32;

  private:
    /** Action the demo script currently drives; used to pick which channel group's
     *  amplitude is boosted while generating samples */
    auto GetActiveAction() const -> EScenarioAction;

    /** Appends one multi-channel sample to the pending frame and the graph buffer */
    void GenerateSample();

    /** Advances the demo script and re-rolls the per-frame artifact when a frame closes */
    void OnFrameBoundary();

    /** Normally distributed noise via Box-Muller on the seeded stream */
    auto Gaussian(float Sigma) -> float;

    /** Deterministic noise/phase source so runs are reproducible */
    FRandomStream Random;

    /** Per-channel phase offsets of the alpha and beta oscillations */
    float AlphaPhase[EegConfig::ChannelCount] = {};
    float BetaPhase[EegConfig::ChannelCount] = {};

    /** Simulated signal time in seconds; advances by exactly 1/SampleRateHz per sample */
    double SignalTime = 0.0;

    /** Time not yet turned into samples */
    double PendingTime = 0.0;

    /** Frame being filled; flushed to Tick()'s output every SamplesPerFrame samples */
    FEegFrame PendingFrame;

    /** Sequence number for the next completed frame */
    int64 NextSeq = 0;

    /** Index of the current demo script entry */
    int32 ScriptIndex = 0;

    /** Seconds the current script entry has been active */
    double ScriptElapsed = 0.0;

    /** Channel group hit by the current artifact burst; INDEX_NONE while clean */
    int32 ArtifactGroupStart = INDEX_NONE;

    /** Graph ring buffer, channel-major */
    TArray<float> GraphBuffer;

    /** Next write position within one channel's ring window */
    int32 GraphWriteIndex = 0;
};
