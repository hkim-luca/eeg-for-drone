#pragma once

#include "CoreMinimal.h"
#include "ScenarioTypes.h"

/** Compile-time layout of the simulated EEG device and of the frames sent to the EEG server.
 *  The channel-group demo rule encoded here must match eeg-server/eeg_server/config.py. */
namespace EegConfig
{
/** Number of electrodes on the simulated EEG device */
inline constexpr int32 ChannelCount = 32;

/** International 10-20/10-10 electrode names for the 32 simulated channels, in the same
 *  order as ChannelCount / GroupStartChannel. Approximates a standard actiCAP/EasyCap
 *  32-channel montage (matches MNE's easycap-M1 template channel order); the simulated
 *  device has no real montage, so this is a display label only. */
inline constexpr const TCHAR *ChannelNames[ChannelCount] = {
    TEXT("Fp1"), TEXT("Fp2"), TEXT("F7"),  TEXT("F3"),  TEXT("Fz"), TEXT("F4"),   TEXT("F8"),  TEXT("FC5"),
    TEXT("FC1"), TEXT("FC2"), TEXT("FC6"), TEXT("T7"),  TEXT("C3"), TEXT("Cz"),   TEXT("C4"),  TEXT("T8"),
    TEXT("CP5"), TEXT("CP1"), TEXT("CP2"), TEXT("CP6"), TEXT("P7"), TEXT("P3"),   TEXT("Pz"),  TEXT("P4"),
    TEXT("P8"),  TEXT("PO9"), TEXT("O1"),  TEXT("Oz"),  TEXT("O2"), TEXT("PO10"), TEXT("AF7"), TEXT("AF8"),
};
static_assert(UE_ARRAY_COUNT(ChannelNames) == ChannelCount, "ChannelNames must have one entry per channel");

/** Samples generated per second per channel */
inline constexpr int32 SampleRateHz = 250;

/** Samples per channel bundled into one network frame (100 ms of signal) */
inline constexpr int32 SamplesPerFrame = 25;

/** Samples per channel kept for the on-screen electrode graphs (2 s window) */
inline constexpr int32 GraphWindowSamples = SampleRateHz * 2;

/** First channel of the group whose amplitude is boosted for the action; INDEX_NONE for Stop.
 *  Groups are GroupChannelCount channels wide and separated so the dummy classifier can
 *  tell them apart: FORWARD=0..5, LEFT=8..13, RIGHT=18..23, BACKWARD=26..31. */
inline constexpr int32 GroupChannelCount = 6;

inline constexpr auto GroupStartChannel(EScenarioAction Action) -> int32
{
    switch (Action)
    {
    case EScenarioAction::Forward:
        return 0;
    case EScenarioAction::Left:
        return 8;
    case EScenarioAction::Right:
        return 18;
    case EScenarioAction::Backward:
        return 26;
    case EScenarioAction::Stop:
        return INDEX_NONE;
    }
    return INDEX_NONE;
}

/** Wire order of ActionResult.action_probs; must match config.ACTION_PROB_ORDER on the server */
inline constexpr EScenarioAction ProbOrder[] = {EScenarioAction::Forward, EScenarioAction::Backward,
                                                EScenarioAction::Left, EScenarioAction::Right, EScenarioAction::Stop};

inline constexpr int32 ProbCount = 5;
} // namespace EegConfig

/** One 100 ms block of simulated EEG ready to be sent to the EEG server */
struct FEegFrame
{
    /** Monotonic frame number, used by the server to detect dropped frames */
    int64 Seq = 0;

    /** Sample-major data in microvolts: Samples[SampleIndex * ChannelCount + Channel] */
    TArray<float> Samples;
};

/** One classification result received from the EEG server */
struct FEegActionResult
{
    /** Monotonic number assigned by the server; echoed back in the control ack */
    int64 ActionSeq = 0;

    /** Action the server inferred from the EEG frames */
    EScenarioAction Action = EScenarioAction::Stop;

    /** Classifier confidence in [0, 1]; informational only */
    float Confidence = 0.0f;

    /** Server wall clock (Unix epoch ms) when the inference finished; echoed back in the ack
     *  so the server can measure inference-to-control latency */
    double InferMs = 0.0;

    /** Per-action probabilities in [0, 1], ordered by EegConfig::ProbOrder */
    float ActionProbs[EegConfig::ProbCount] = {};
};

/** Parses the uppercase wire label (LEFT, RIGHT, FORWARD, BACKWARD, STOP) written by
 *  ScenarioActionName() back to the enum; returns false if the label is unknown */
inline auto ParseScenarioActionName(const FString &Label, EScenarioAction &OutAction) -> bool
{
    const UEnum *Enum = StaticEnum<EScenarioAction>();
    for (int32 Index = 0; Index < Enum->NumEnums() - 1; ++Index)
    {
        if (Enum->GetNameStringByIndex(Index).ToUpper() == Label)
        {
            OutAction = static_cast<EScenarioAction>(Enum->GetValueByIndex(Index));
            return true;
        }
    }
    return false;
}
