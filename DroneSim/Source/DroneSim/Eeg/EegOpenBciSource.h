#pragma once

#include "CoreMinimal.h"
#include "EegTypes.h"

/**
 *  Live EEG source backed by an OpenBCI board through the BrainFlow C++ SDK.
 *
 *  Replaces the former simulated device: OpenBCI (on the same PC as DroneSim) streams raw
 *  channels, which this class maps into the AI model's 22-channel layout and hands out as
 *  completed frames via Tick(). The board channel -> electrode mapping is read from
 *  mappings/openbci_to_sim.json; each electrode is then placed at its model-input index
 *  (EegConfig::ChannelNames order), and every model channel the board does not cover is left
 *  at 0 (zero-fill). The action is decided entirely by the server's AI model - never encoded
 *  here.
 *
 *  Connection is self-healing: Start() keeps a "want connection" intent and Tick() retries the
 *  board session until it succeeds, and again whenever a running session drops. The on-screen
 *  graph shows the ACTUAL acquired channels (8 Cyton / 16 Cyton+Daisy), whose count and names
 *  are fixed from the mapping at Start(), not the 22 model channels.
 */
class FEegOpenBciSource
{
  public:
    /** Begins streaming intent: opens the board now and keeps retrying on failure/drop.
     *  SerialPort is the Cyton dongle COM port (e.g. "COM3"); MappingFilePath overrides the
     *  default mappings/openbci_to_sim.json. */
    void Start(const FString &SerialPort, const FString &MappingFilePath);

    /** Stops streaming and releases the board session; cancels the reconnect intent. */
    void Stop();

    /** Pulls all samples available from the board (reconnecting if needed) and appends every
     *  completed frame to OutFrames. */
    void Tick(float DeltaTime, TArray<FEegFrame> &OutFrames);

    /** True while the board session is open and streaming (false while reconnecting). */
    auto IsStreaming() const -> bool;

    /** Actual acquired channel count, fixed at Start() (8 Cyton / 16 Cyton+Daisy). */
    auto GetChannelCount() const -> int32;

    /** Electrode name per acquired channel, in board order; same length as GetChannelCount(). */
    auto GetChannelNames() const -> const TArray<FString> &;

    /** Channel-major graph ring buffer over the ACQUIRED channels:
     *  [Channel * GraphWindowSamples + Index], microvolts. */
    auto GetGraphBuffer() const -> const TArray<float> &;

    /** Ring position of the oldest sample in the graph buffer. */
    auto GetGraphWriteIndex() const -> int32;

  private:
    /** Loads openbci_to_sim.json: resolves board channels to model indices and names. */
    void BuildChannelMap(const FString &MappingFilePath);

    /** Opens the board session; sets bStreaming on success, leaves it false on failure. */
    void TryConnect();

    /** Stops and releases the board session if one is open. */
    void ReleaseBoard();

    /** Appends one acquired-channel (length ChannelCount) sample to the graph ring buffer. */
    void AppendGraphSample(const TArray<float> &BoardSample);

    /** Appends one model-order (length EegConfig::ChannelCount) sample to the pending frame,
     *  flushing a completed frame to OutFrames every SamplesPerFrame samples. */
    void AppendFrameSample(const TArray<float> &ModelSample, TArray<FEegFrame> &OutFrames);

    /** Board channel index (0..ChannelCount-1) -> model-input index (0..EegConfig::ChannelCount-1),
     *  or INDEX_NONE when that board channel's electrode is not one the model uses. */
    TArray<int32> OpenBciToModelIndex;

    /** Electrode name per board channel (graph labels), fixed at Start(). */
    TArray<FString> ChannelNames;

    /** BrainFlow EEG row indices within a get_board_data() result (one per board channel). */
    TArray<int32> EegChannelRows;

    /** Acquired channel count from the mapping: 8 = Cyton, 16 = Cyton+Daisy. */
    int32 BoardChannelCount = 8;

    /** Graph ring buffer, channel-major over the acquired channels. */
    TArray<float> GraphBuffer;

    /** Next write position within one channel's ring window. */
    int32 GraphWriteIndex = 0;

    /** Frame being filled (model-order, EegConfig::ChannelCount wide). */
    FEegFrame PendingFrame;

    /** Sequence number for the next completed frame. */
    int64 NextSeq = 0;

    /** True while Start() intent is active (drives automatic reconnect). */
    bool bWantConnection = false;

    /** True while a board session is open and streaming. */
    bool bStreaming = false;

    /** Serial port kept for reconnect attempts. */
    FString SerialPort;

    /** Seconds accumulated since the last connect attempt; gates the retry interval. */
    float SecondsSinceRetry = 0.0f;

    /** Opaque BrainFlow BoardShim* (kept void* so BrainFlow headers stay in the .cpp). */
    void *BoardHandle = nullptr;
};
