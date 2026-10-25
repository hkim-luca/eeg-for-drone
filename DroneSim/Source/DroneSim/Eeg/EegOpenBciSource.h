#pragma once

#include "CoreMinimal.h"
#include "EegTypes.h"

/**
 *  Live EEG source backed by an OpenBCI board through the BrainFlow C++ SDK.
 *
 *  OpenBCI (on the same PC as DroneSim) streams raw channels, which this class routes by
 *  electrode name into the AI model's input layout and hands out as completed frames via
 *  Tick(): config/openbci_to_sim.json names each board channel, config/sim_to_model.json gives
 *  each name its model-input index. Model inputs the board does not cover are left at 0
 *  (zero-fill); without both files nothing is acquired. The action is decided entirely by the
 *  server's AI model - never encoded here.
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
     *  default config/openbci_to_sim.json. */
    void Start(const FString &SerialPort, const FString &MappingFilePath);

    /** Stops streaming and releases the board session; cancels the reconnect intent. */
    void Stop();

    /** Pulls all samples available from the board (reconnecting if needed) and appends every
     *  completed frame to OutFrames. If OutRaw is given it also receives every acquired sample
     *  as [board time (unix s), ch0 .. ch(ChannelCount-1)] in board order, sample-major. */
    void Tick(float DeltaTime, TArray<FEegFrame> &OutFrames, TArray<double> *OutRaw = nullptr);

    /** True while the board session is open and streaming (false while reconnecting). */
    auto IsStreaming() const -> bool;

    /** Board sampling rate reported by BrainFlow (250 Cyton, 125 Cyton+Daisy); 0 until streaming. */
    auto GetSampleRate() const -> int32;

    /** Actual acquired channel count, fixed at Start() (8 Cyton / 16 Cyton+Daisy). */
    auto GetChannelCount() const -> int32;

    /** Electrode name per acquired channel, in board order; same length as GetChannelCount(). */
    auto GetChannelNames() const -> const TArray<FString> &;

    /** Model-input index per acquired channel, in board order; INDEX_NONE if the model does not use it. */
    auto GetModelIndices() const -> const TArray<int32> &;

    /** Electrode name per model input, in model-input order (from sim_to_model.json). */
    auto GetModelChannelNames() const -> const TArray<FString> &;

    /** Channel-major graph ring buffer over the ACQUIRED channels:
     *  [Channel * GraphWindowSamples + Index], microvolts. */
    auto GetGraphBuffer() const -> const TArray<float> &;

    /** Ring position of the oldest sample in the graph buffer. */
    auto GetGraphWriteIndex() const -> int32;

  private:
    /** Loads sim_to_model.json into ModelChannelNames; false (logged) if missing or inconsistent. */
    auto BuildModelLayout() -> bool;

    /** Loads openbci_to_sim.json: board channel -> name -> model index; false (logged) if invalid. */
    auto BuildChannelMap(const FString &MappingFilePath) -> bool;

    /** Starts opening the board session on a worker thread (Cyton's prepare_session blocks ~2 s);
     *  Tick() applies the outcome via FinishConnect(). */
    void TryConnect();

    /** Applies a finished connect attempt on the game thread: streaming on success, retry on failure. */
    void FinishConnect();

    /** Abandons an in-flight connect; the worker releases a session that opens after this. */
    void CancelPendingConnect();

    /** Stops and releases the board session if one is open. */
    void ReleaseBoard();

    /** Appends one acquired-channel (length ChannelCount) sample to the graph ring buffer. */
    void AppendGraphSample(const TArray<float> &BoardSample);

    /** Appends one model-order (length ModelChannelNames.Num()) sample to the pending frame,
     *  flushing a completed frame to OutFrames every SamplesPerFrame samples. */
    void AppendFrameSample(const TArray<float> &ModelSample, TArray<FEegFrame> &OutFrames);

    /** Board channel index (0..ChannelCount-1) -> model-input index (0..ModelChannelNames.Num()-1),
     *  or INDEX_NONE when that board channel's electrode is not one the model uses. */
    TArray<int32> OpenBciToModelIndex;

    /** Electrode name per board channel (graph labels), fixed at Start(). */
    TArray<FString> ChannelNames;

    /** BrainFlow EEG row indices within a get_board_data() result (one per board channel). */
    TArray<int32> EegChannelRows;

    /** BrainFlow timestamp row (unix seconds) within a get_board_data() result. */
    int32 TimestampRow = INDEX_NONE;

    /** Acquired channel count from the mapping: 8 = Cyton, 16 = Cyton+Daisy. */
    int32 BoardChannelCount = 0;

    /** Graph ring buffer, channel-major over the acquired channels. */
    TArray<float> GraphBuffer;

    /** Next write position within one channel's ring window. */
    int32 GraphWriteIndex = 0;

    /** Electrode name per model input, in model-input order (sim_to_model.json). */
    TArray<FString> ModelChannelNames;

    /** Frame being filled (model-order, ModelChannelNames.Num() wide). */
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

    /** BrainFlow board id of the open session (Cyton / Cyton+Daisy). */
    int32 BoardId = 0;

    /** BrainFlow input params JSON identifying the session; passed to every C API call. */
    FString ParamsJson;

    /** True after prepare_session succeeded, until release_session. */
    bool bSessionPrepared = false;

    /** Outcome of one background connect, shared with its worker. */
    struct FConnectAttempt;

    /** Connect attempt in flight on a worker thread; null when none. */
    TSharedPtr<FConnectAttempt, ESPMode::ThreadSafe> PendingConnect;

    /** Serial port of the pending or open session, for log lines. */
    FString ConnectPort;

    /** Rows of a get_board_data() result for this board. */
    int32 NumRows = 0;

    /** Board sampling rate from BrainFlow, set when streaming starts. */
    int32 SampleRateHz = 0;

    /** Reused buffer for get_board_data() (row-major [NumRows][samples]). */
    TArray<double> DataScratch;

    /** BrainFlow ring buffer size in samples (~3 min at 250 Hz). */
    static constexpr int32 StreamBufferSamples = 45000;
};
