#pragma once

#include "CoreMinimal.h"
#include "EegTypes.h"

/**
 *  Live EEG source backed by an OpenBCI board through the BrainFlow C++ SDK.
 *
 *  Replaces the former simulated device: OpenBCI (on the same PC as DroneSim) streams raw
 *  channels, which this class maps into the AI model's 22-channel layout and hands out as
 *  completed frames via Tick(). The board channel -> electrode mapping is read from
 *  mappings/openbci_to_channel.json; each electrode is then placed at its model-input index
 *  (EegConfig::ChannelNames order), and every model channel the board does not cover is left
 *  at 0 (zero-fill). The action is decided entirely by the server's AI model - never encoded
 *  here. A ring buffer of recent samples feeds the on-screen electrode graphs.
 *
 *  If the BrainFlow SDK is not linked (WITH_BRAINFLOW=0) or the board fails to open, the
 *  source produces no frames and logs the reason; the drone simply holds position.
 */
class FEegOpenBciSource
{
  public:
    /** Opens the OpenBCI board and begins streaming. SerialPort is the Cyton dongle COM port
     *  (e.g. "COM3"); MappingFilePath overrides the default mappings/openbci_to_channel.json. */
    void Start(const FString &SerialPort, const FString &MappingFilePath);

    /** Stops streaming and releases the board session. */
    void Stop();

    /** Pulls all samples available from the board and appends every completed frame to OutFrames. */
    void Tick(float DeltaTime, TArray<FEegFrame> &OutFrames);

    /** Channel-major graph ring buffer: [Channel * GraphWindowSamples + Index], microvolts */
    auto GetGraphBuffer() const -> const TArray<float> &;

    /** Ring position of the oldest sample in the graph buffer */
    auto GetGraphWriteIndex() const -> int32;

    /** True while the board session is open and streaming */
    auto IsStreaming() const -> bool;

  private:
    /** Loads openbci_to_channel.json and resolves each board channel to a model-input index */
    void BuildChannelMap(const FString &MappingFilePath);

    /** Appends one model-order (length ChannelCount) sample to the pending frame and graph buffer,
     *  flushing a completed frame to OutFrames every SamplesPerFrame samples */
    void AppendSample(const TArray<float> &ModelSample, TArray<FEegFrame> &OutFrames);

    /** Board channel index (0..N-1) -> model-input index (0..ChannelCount-1), or INDEX_NONE
     *  when that board channel's electrode is not one the model uses */
    TArray<int32> OpenBciToModelIndex;

    /** BrainFlow EEG row indices within a get_board_data() result (one per board channel) */
    TArray<int32> EegChannelRows;

    /** Board channel count from the mapping file: 8 = Cyton, 16 = Cyton+Daisy */
    int32 BoardChannelCount = 8;

    /** Graph ring buffer, channel-major */
    TArray<float> GraphBuffer;

    /** Next write position within one channel's ring window */
    int32 GraphWriteIndex = 0;

    /** Frame being filled; flushed every SamplesPerFrame samples */
    FEegFrame PendingFrame;

    /** Sequence number for the next completed frame */
    int64 NextSeq = 0;

    /** True between a successful Start() and Stop() */
    bool bStreaming = false;

    /** Opaque BrainFlow BoardShim* (kept void* so BrainFlow headers stay in the .cpp) */
    void *BoardHandle = nullptr;
};
