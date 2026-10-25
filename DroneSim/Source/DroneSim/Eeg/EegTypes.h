#pragma once

#include "CoreMinimal.h"
#include "ScenarioTypes.h"

/** Timing of the frames sent to the EEG server. The model-input channel layout is not fixed
 *  here: it is read from config/sim_to_model.json (see FEegOpenBciSource). */
namespace EegConfig
{
/** Samples generated per second per channel */
inline constexpr int32 SampleRateHz = 250;

/** Samples per channel bundled into one network frame (100 ms of signal) */
inline constexpr int32 SamplesPerFrame = 25;

/** Samples per channel kept for the on-screen electrode graphs (2 s window) */
inline constexpr int32 GraphWindowSamples = SampleRateHz * 2;

/** Wire order of ActionResult.action_probs; must match config.ACTION_PROB_ORDER on the server */
inline constexpr EScenarioAction ProbOrder[] = {EScenarioAction::Forward, EScenarioAction::Backward,
                                                EScenarioAction::Left, EScenarioAction::Right};

inline constexpr int32 ProbCount = UE_ARRAY_COUNT(ProbOrder);

/** Index of one action inside ActionProbs (the ProbOrder position); INDEX_NONE if absent */
inline constexpr auto ProbIndex(EScenarioAction Action) -> int32
{
    for (int32 Index = 0; Index < ProbCount; ++Index)
    {
        if (ProbOrder[Index] == Action)
        {
            return Index;
        }
    }
    return INDEX_NONE;
}
} // namespace EegConfig

/** One 100 ms block of simulated EEG ready to be sent to the EEG server */
struct FEegFrame
{
    /** Monotonic frame number, used by the server to detect dropped frames */
    int64 Seq = 0;

    /** Model-input channel count of Samples (from config/sim_to_model.json) */
    int32 Channels = 0;

    /** Sample-major data in microvolts: Samples[SampleIndex * ChannelCount + Channel] */
    TArray<float> Samples;
};

/** Rolling evaluation metrics, mirrored from the dashboard KPI tiles
 *  (eeg_server MetricsStore.snapshot) so the in-game EEG HUD shows the same numbers */
struct FEegMetrics
{
    float LatencyDeviceToInferMeanMs = 0.0f;
    float LatencyDeviceToInferLastMs = 0.0f;
    float LatencyDeviceToInferP95Ms = 0.0f;
    float LatencyInferToControlMeanMs = 0.0f;
    float LatencyInferToControlLastMs = 0.0f;
    float LatencyInferToControlP95Ms = 0.0f;
    float LatencyDeviceToControlMeanMs = 0.0f;
    float LatencyDeviceToControlLastMs = 0.0f;
    float LatencyDeviceToControlP95Ms = 0.0f;
    float ReliabilityOverallPercent = 0.0f;
    float ReliabilityFramePercent = 0.0f;
    int32 ReliabilityFramesLost = 0;
    float ReliabilityAckPercent = 0.0f;

    /** Server-only duration (ms) spent classifying this frame; copied in from the enclosing
     *  FEegActionResult.InferDurationMs so the HUD's pipeline chart has it alongside the rest
     *  of the metrics history */
    float InferDurationMs = 0.0f;
};

/** One classification result received from the EEG server */
struct FEegActionResult
{
    /** Monotonic number assigned by the server; echoed back in the control ack */
    int64 ActionSeq = 0;

    /** Action the server inferred from the EEG frames */
    EScenarioAction Action = EScenarioAction::Forward;

    /** Classifier confidence in [0, 1]; informational only */
    float Confidence = 0.0f;

    /** Server-local duration (ms) spent classifying the frame; NOT an absolute timestamp
     *  (the server may run on a different, unsynchronized-clock machine), informational only
     *  on this side - the server uses it to derive Metrics.LatencyInferToControl* */
    double InferDurationMs = 0.0;

    /** Per-action probabilities in [0, 1], ordered by EegConfig::ProbOrder */
    float ActionProbs[EegConfig::ProbCount] = {};

    /** Rolling latency/reliability metrics sent alongside this result */
    FEegMetrics Metrics;
};
