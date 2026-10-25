#include "EegOpenBciSource.h"
#include "DroneSimPaths.h"
#include "ScenarioLog.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"
#include "Tasks/Task.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#if WITH_BRAINFLOW
// BrainFlow's C API only (plain C types across the DLL boundary); the C++ BoardShim wrapper
// passes std::string by value, which arrived corrupted inside the UE process and made
// start_stream fail with UNABLE_TO_OPEN_PORT (code 2) while the same calls worked standalone.
#include "board_controller.h"
#include "board_info_getter.h"
#include "brainflow_constants.h"
#endif

namespace
{
/** BrainFlow board ids: Cyton = 8ch, Cyton+Daisy = 16ch */
constexpr int32 CytonBoardId = 0;       // BoardIds::CYTON_BOARD
constexpr int32 CytonDaisyBoardId = 2;  // BoardIds::CYTON_DAISY_BOARD

/** Seconds between (re)connect attempts */
constexpr float RetryInterval = 2.0f;

/** Channel maps as loaded on the first Start(); the JSON files are read once per run */
struct FChannelMapCache
{
    bool bLoaded = false;
    bool bValid = false;
    TArray<FString> ModelChannelNames;
    TArray<FString> ChannelNames;
    TArray<int32> OpenBciToModelIndex;
    int32 BoardChannelCount = 0;
};
FChannelMapCache ChannelMapCache;

/** Picks the board id for the mapped channel count (16 -> Cyton+Daisy, otherwise Cyton) */
auto BoardIdForChannels(int32 ChannelCount) -> int32
{
    return ChannelCount >= 16 ? CytonDaisyBoardId : CytonBoardId;
}

/** Loads a JSON object file; null (with an error logged) if missing or malformed */
auto LoadJsonFile(const FString &Path) -> TSharedPtr<FJsonObject>
{
    FString Json;
    TSharedPtr<FJsonObject> Root;
    if (!FFileHelper::LoadFileToString(Json, *Path) ||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<TCHAR>::Create(Json), Root) || !Root.IsValid())
    {
        FScenarioLog::Error(FString::Printf(TEXT("Channel map not found or invalid: %s"), *Path));
        return nullptr;
    }
    return Root;
}
} // namespace

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
namespace
{
/** Returns the COM port of the first FTDI serial device (OpenBCI Cyton dongle), or empty.
 *  FTDI's VCP driver registers under HKLM\HARDWARE\DEVICEMAP\SERIALCOMM with a value name
 *  like "\Device\VCP0" whose data is the "COMx" string. */
auto DetectFtdiSerialPort() -> FString
{
    HKEY Key = nullptr;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, &Key) != ERROR_SUCCESS)
    {
        return FString();
    }

    FString Port;
    for (DWORD Index = 0;; ++Index)
    {
        wchar_t Name[256];
        DWORD NameLen = UE_ARRAY_COUNT(Name);
        BYTE Data[256];
        DWORD DataLen = sizeof(Data);
        DWORD Type = 0;
        const LONG Result = RegEnumValueW(Key, Index, Name, &NameLen, nullptr, &Type, Data, &DataLen);
        if (Result != ERROR_SUCCESS)
        {
            break; // ERROR_NO_MORE_ITEMS or an error
        }
        if (Type != REG_SZ)
        {
            continue;
        }
        const FString ValueName(Name);
        if (ValueName.Contains(TEXT("VCP"))) // FTDI virtual COM port
        {
            Port = FString(reinterpret_cast<const wchar_t *>(Data));
            break;
        }
    }

    RegCloseKey(Key);
    return Port;
}
} // namespace
#include "Windows/HideWindowsPlatformTypes.h"
#else
namespace
{
auto DetectFtdiSerialPort() -> FString
{
    return FString();
}
} // namespace
#endif

auto FEegOpenBciSource::BuildModelLayout() -> bool
{
    // sim_to_model.json: electrode name -> model-input index, in the order the model was trained on
    ModelChannelNames.Reset();
    const FString Path = DroneSimConfigDir() / TEXT("sim_to_model.json");
    const TSharedPtr<FJsonObject> Root = LoadJsonFile(Path);
    const TSharedPtr<FJsonObject> *Index = nullptr;
    if (!Root.IsValid() || !Root->TryGetObjectField(TEXT("index"), Index) || (*Index)->Values.IsEmpty())
    {
        FScenarioLog::Error(FString::Printf(TEXT("%s needs an index"), *Path));
        return false;
    }

    // the model-input count is the number of entries; their indices must cover 0..Count-1 once each
    const int32 Count = (*Index)->Values.Num();
    ModelChannelNames.SetNum(Count);
    for (const TPair<FString, TSharedPtr<FJsonValue>> &Pair : (*Index)->Values)
    {
        int32 ModelIndex = INDEX_NONE;
        if (!Pair.Value.IsValid() || !Pair.Value->TryGetNumber(ModelIndex) || !ModelChannelNames.IsValidIndex(ModelIndex) ||
            !ModelChannelNames[ModelIndex].IsEmpty())
        {
            FScenarioLog::Error(FString::Printf(TEXT("%s: '%s' has an invalid or duplicate index"), *Path, *Pair.Key));
            ModelChannelNames.Reset();
            return false;
        }
        ModelChannelNames[ModelIndex] = Pair.Key;
    }
    for (int32 ModelIndex = 0; ModelIndex < Count; ++ModelIndex)
    {
        if (ModelChannelNames[ModelIndex].IsEmpty())
        {
            FScenarioLog::Error(FString::Printf(TEXT("%s: no electrode for model index %d"), *Path, ModelIndex));
            ModelChannelNames.Reset();
            return false;
        }
    }
    return true;
}

auto FEegOpenBciSource::BuildChannelMap(const FString &MappingFilePath) -> bool
{
    // openbci_to_sim.json: board channel -> electrode name; the name then picks the model index
    BoardChannelCount = 0;
    OpenBciToModelIndex.Reset();
    ChannelNames.Reset();

    const FString Path = MappingFilePath.IsEmpty() ? DroneSimConfigDir() / TEXT("openbci_to_sim.json") : MappingFilePath;
    const TSharedPtr<FJsonObject> Root = LoadJsonFile(Path);
    const TSharedPtr<FJsonObject> *Mapping = nullptr;
    if (!Root.IsValid() || !Root->TryGetObjectField(TEXT("mapping"), Mapping) || (*Mapping)->Values.IsEmpty())
    {
        FScenarioLog::Error(FString::Printf(TEXT("%s needs a mapping"), *Path));
        return false;
    }

    // the board channel count is the number of entries; their keys must cover 0..Count-1 once each
    BoardChannelCount = (*Mapping)->Values.Num();
    OpenBciToModelIndex.Init(INDEX_NONE, BoardChannelCount);
    ChannelNames.SetNum(BoardChannelCount);
    TArray<bool> Seen;
    Seen.Init(false, BoardChannelCount);
    for (const TPair<FString, TSharedPtr<FJsonValue>> &Pair : (*Mapping)->Values)
    {
        const int32 BoardIndex = FCString::IsNumeric(*Pair.Key) ? FCString::Atoi(*Pair.Key) : INDEX_NONE;
        if (!ChannelNames.IsValidIndex(BoardIndex) || Seen[BoardIndex] || !Pair.Value.IsValid())
        {
            FScenarioLog::Error(FString::Printf(TEXT("%s: board channel '%s' must be one of 0..%d, once each"), *Path,
                                                *Pair.Key, BoardChannelCount - 1));
            BoardChannelCount = 0;
            OpenBciToModelIndex.Reset();
            ChannelNames.Reset();
            return false;
        }
        Seen[BoardIndex] = true;
        ChannelNames[BoardIndex] = Pair.Value->AsString();
    }

    int32 Mapped = 0;
    for (int32 BoardIndex = 0; BoardIndex < BoardChannelCount; ++BoardIndex)
    {
        const FString &Name = ChannelNames[BoardIndex];
        const int32 ModelIndex =
            Name.IsEmpty() ? INDEX_NONE
                           : ModelChannelNames.IndexOfByPredicate(
                                 [&Name](const FString &Model) { return Model.Equals(Name, ESearchCase::IgnoreCase); });
        OpenBciToModelIndex[BoardIndex] = ModelIndex;
        if (ModelIndex != INDEX_NONE)
        {
            ++Mapped;
        }
        else
        {
            FScenarioLog::Info(FString::Printf(TEXT("Board channel %d ('%s') is not a model input; unused"),
                                               BoardIndex, *Name));
        }
    }

    FScenarioLog::Info(FString::Printf(TEXT("OpenBCI channel map: %d channels, %d placed into the %d-channel model"),
                                       BoardChannelCount, Mapped, ModelChannelNames.Num()));
    return true;
}

/** Written by the connect worker under Lock; read by the game thread once bDone is set */
struct FEegOpenBciSource::FConnectAttempt
{
    FCriticalSection Lock;
    bool bDone = false;
    bool bCanceled = false;
    int Result = 0;
};

void FEegOpenBciSource::Start(const FString &InSerialPort, const FString &MappingFilePath)
{
    CancelPendingConnect();
    ReleaseBoard();

    if (!ChannelMapCache.bLoaded)
    {
        ChannelMapCache.bLoaded = true;
        ChannelMapCache.bValid = BuildModelLayout() && BuildChannelMap(MappingFilePath);
        ChannelMapCache.ModelChannelNames = ModelChannelNames;
        ChannelMapCache.ChannelNames = ChannelNames;
        ChannelMapCache.OpenBciToModelIndex = OpenBciToModelIndex;
        ChannelMapCache.BoardChannelCount = BoardChannelCount;
    }
    else
    {
        ModelChannelNames = ChannelMapCache.ModelChannelNames;
        ChannelNames = ChannelMapCache.ChannelNames;
        OpenBciToModelIndex = ChannelMapCache.OpenBciToModelIndex;
        BoardChannelCount = ChannelMapCache.BoardChannelCount;
    }

    // no guessed layout: without both maps nothing is acquired or sent
    if (!ChannelMapCache.bValid)
    {
        BoardChannelCount = 0;
        GraphBuffer.Reset();
        bWantConnection = false;
        bStreaming = false;
        FScenarioLog::Error(TEXT("OpenBCI not started: fix the channel maps in the config folder"));
        return;
    }

    GraphBuffer.Init(0.0f, BoardChannelCount * EegConfig::GraphWindowSamples);
    GraphWriteIndex = 0;
    NextSeq = 0;
    PendingFrame = FEegFrame();
    PendingFrame.Seq = NextSeq;
    PendingFrame.Channels = ModelChannelNames.Num();
    PendingFrame.Samples.Reserve(EegConfig::SamplesPerFrame * ModelChannelNames.Num());
    EegChannelRows.Reset();

    SerialPort = InSerialPort;
    bWantConnection = true;
    bStreaming = false;
    SecondsSinceRetry = 0.0f;
    TryConnect();
}

void FEegOpenBciSource::TryConnect()
{
#if WITH_BRAINFLOW
    // one-time: route BrainFlow's internal diagnostics to Saved/Logs/brainflow.log so a
    // failing prepare/start_stream reveals the underlying reason
    static bool bBrainFlowLoggerReady = false;
    if (!bBrainFlowLoggerReady)
    {
        bBrainFlowLoggerReady = true;
        set_log_file_board_controller(TCHAR_TO_UTF8(*(FPaths::ProjectLogDir() / TEXT("brainflow.log"))));
        set_log_level_board_controller(0); // trace
    }

    FString Port = SerialPort;
    if (Port.IsEmpty() || Port.Equals(TEXT("auto"), ESearchCase::IgnoreCase))
    {
        Port = DetectFtdiSerialPort();
        if (Port.IsEmpty())
        {
            FScenarioLog::Error(FString::Printf(
                TEXT("OpenBCI serial port auto-detect found no FTDI device; set OpenBciSerialPort. Retrying in %.0fs"),
                RetryInterval));
            bStreaming = false;
            return;
        }
        FScenarioLog::Info(FString::Printf(TEXT("OpenBCI serial port auto-detected: %s"), *Port));
    }

    BoardId = BoardIdForChannels(BoardChannelCount);
    // BrainFlow identifies a session by these parsed values, so prepare/start/read/release
    // must all pass the same JSON
    ParamsJson = FString::Printf(
        TEXT("{\"serial_port\":\"%s\",\"ip_protocol\":0,\"ip_port\":0,\"ip_port_aux\":0,\"ip_port_anc\":0,"
             "\"other_info\":\"\",\"mac_address\":\"\",\"ip_address\":\"\",\"ip_address_aux\":\"\","
             "\"ip_address_anc\":\"\",\"timeout\":0,\"serial_number\":\"\",\"file\":\"\",\"file_aux\":\"\","
             "\"file_anc\":\"\",\"master_board\":-100}"),
        *Port);
    ConnectPort = Port;

    // prepare_session/start_stream wait on the serial board for seconds; keep the game thread free
    PendingConnect = MakeShared<FConnectAttempt, ESPMode::ThreadSafe>();
    UE::Tasks::Launch(UE_SOURCE_LOCATION, [Attempt = PendingConnect, Board = BoardId, Params = ParamsJson]() {
        const FTCHARToUTF8 Json(*Params);
        constexpr int StatusOk = static_cast<int>(BrainFlowExitCodes::STATUS_OK);
        int Result = prepare_session(Board, Json.Get());
        const bool bPrepared = Result == StatusOk;
        if (bPrepared)
        {
            Result = start_stream(StreamBufferSamples, "", Board, Json.Get());
        }

        FScopeLock ScopeLock(&Attempt->Lock);
        if (bPrepared && (Result != StatusOk || Attempt->bCanceled))
        {
            // a failed start, or Stop() came first: no one else will release this session, and a
            // leaked one makes the next attempt fail with ANOTHER_BOARD_IS_CREATED (code 16)
            release_session(Board, Json.Get());
        }
        Attempt->Result = Result;
        Attempt->bDone = true;
    });
#else
    bWantConnection = false; // nothing to retry without the SDK
    FScenarioLog::Error(TEXT("BrainFlow SDK not linked (WITH_BRAINFLOW=0): no EEG will be streamed. "
                             "Place the SDK under DroneSim/ThirdParty/BrainFlow and rebuild."));
#endif
}

void FEegOpenBciSource::FinishConnect()
{
#if WITH_BRAINFLOW
    const int Result = PendingConnect->Result;
    PendingConnect.Reset();
    if (Result != static_cast<int>(BrainFlowExitCodes::STATUS_OK))
    {
        FScenarioLog::Error(FString::Printf(TEXT("OpenBCI connect failed on %s (BrainFlow code %d); retrying in %.0fs"),
                                            *ConnectPort, Result, RetryInterval));
        bStreaming = false;
        SecondsSinceRetry = 0.0f;
        return;
    }
    bSessionPrepared = true;

    constexpr int32 MaxRows = 64;
    int Channels[MaxRows] = {};
    int ChannelCount = 0;
    get_eeg_channels(BoardId, static_cast<int>(BrainFlowPresets::DEFAULT_PRESET), Channels, &ChannelCount);
    EegChannelRows.Reset();
    for (int32 Index = 0; Index < ChannelCount; ++Index)
    {
        EegChannelRows.Add(Channels[Index]);
    }
    int Row = INDEX_NONE;
    get_timestamp_channel(BoardId, static_cast<int>(BrainFlowPresets::DEFAULT_PRESET), &Row);
    TimestampRow = Row;
    int Rows = 0;
    get_num_rows(BoardId, static_cast<int>(BrainFlowPresets::DEFAULT_PRESET), &Rows);
    NumRows = Rows;
    int Rate = 0;
    get_sampling_rate(BoardId, static_cast<int>(BrainFlowPresets::DEFAULT_PRESET), &Rate);
    SampleRateHz = Rate;

    bStreaming = true;
    FScenarioLog::Info(FString::Printf(TEXT("OpenBCI streaming on %s (board %d): %d EEG channels @ %dHz"), *ConnectPort,
                                       BoardId, EegChannelRows.Num(), SampleRateHz));
#endif
}

void FEegOpenBciSource::CancelPendingConnect()
{
    if (!PendingConnect.IsValid())
    {
        return;
    }
    {
        FScopeLock ScopeLock(&PendingConnect->Lock);
        if (!PendingConnect->bDone)
        {
            PendingConnect->bCanceled = true; // the worker releases the session when it finishes
        }
#if WITH_BRAINFLOW
        else if (PendingConnect->Result == static_cast<int>(BrainFlowExitCodes::STATUS_OK))
        {
            bSessionPrepared = true; // opened but never applied: ReleaseBoard() closes it
        }
#endif
    }
    PendingConnect.Reset();
}

void FEegOpenBciSource::ReleaseBoard()
{
#if WITH_BRAINFLOW
    if (bSessionPrepared)
    {
        // release_session unregisters the board in BrainFlow's process-wide registry and stops a
        // running stream; skipping it after a partial start leaks the registration and the next
        // attempt fails with ANOTHER_BOARD_IS_CREATED (code 16)
        release_session(BoardId, TCHAR_TO_UTF8(*ParamsJson));
        bSessionPrepared = false;
    }
#endif
}

void FEegOpenBciSource::Stop()
{
    bWantConnection = false;
    bStreaming = false;
    CancelPendingConnect();
    ReleaseBoard();
}

void FEegOpenBciSource::Tick(float DeltaTime, TArray<FEegFrame> &OutFrames, TArray<double> *OutRaw)
{
    if (!bWantConnection)
    {
        return;
    }

    if (!bStreaming)
    {
        if (PendingConnect.IsValid())
        {
            bool bDone = false;
            {
                FScopeLock ScopeLock(&PendingConnect->Lock);
                bDone = PendingConnect->bDone;
            }
            if (bDone)
            {
                FinishConnect();
            }
            return;
        }

        SecondsSinceRetry += DeltaTime;
        if (SecondsSinceRetry >= RetryInterval)
        {
            SecondsSinceRetry = 0.0f;
            TryConnect();
        }
        return;
    }

#if WITH_BRAINFLOW
    const FTCHARToUTF8 Json(*ParamsJson);
    const int Preset = static_cast<int>(BrainFlowPresets::DEFAULT_PRESET);
    int SampleCount = 0;
    int Result = get_board_data_count(Preset, &SampleCount, BoardId, Json.Get());
    if (Result == static_cast<int>(BrainFlowExitCodes::STATUS_OK) && SampleCount > 0)
    {
        // row-major [NumRows][SampleCount], same layout BoardShim::get_board_data builds
        DataScratch.SetNumUninitialized(NumRows * SampleCount);
        Result = get_board_data(SampleCount, Preset, DataScratch.GetData(), BoardId, Json.Get());
    }
    if (Result != static_cast<int>(BrainFlowExitCodes::STATUS_OK))
    {
        FScenarioLog::Error(FString::Printf(TEXT("OpenBCI read error (BrainFlow code %d); reconnecting"), Result));
        ReleaseBoard();
        bStreaming = false;
        SecondsSinceRetry = 0.0f;
        return;
    }
    auto At = [this, SampleCount](int32 Row, int32 Sample) -> double {
        return DataScratch[Row * SampleCount + Sample];
    };
    const int32 BoardChannels = FMath::Min(EegChannelRows.Num(), BoardChannelCount);

    TArray<float> BoardSample;
    BoardSample.SetNumZeroed(BoardChannelCount);
    TArray<float> ModelSample;
    ModelSample.SetNumZeroed(ModelChannelNames.Num());

    for (int32 Sample = 0; Sample < SampleCount; ++Sample)
    {
        for (float &Value : BoardSample)
        {
            Value = 0.0f;
        }
        for (float &Value : ModelSample)
        {
            Value = 0.0f; // zero-fill model channels the board does not cover
        }
        for (int32 Channel = 0; Channel < BoardChannels; ++Channel)
        {
            const float Value = static_cast<float>(At(EegChannelRows[Channel], Sample));
            BoardSample[Channel] = Value;
            const int32 ModelIndex = OpenBciToModelIndex[Channel];
            if (ModelIndex != INDEX_NONE)
            {
                ModelSample[ModelIndex] = Value;
            }
        }
        AppendGraphSample(BoardSample);
        AppendFrameSample(ModelSample, OutFrames);

        if (OutRaw != nullptr)
        {
            OutRaw->Add(TimestampRow != INDEX_NONE ? At(TimestampRow, Sample) : 0.0);
            for (const float Value : BoardSample)
            {
                OutRaw->Add(Value);
            }
        }
    }
#else
    (void)OutFrames;
    (void)OutRaw;
#endif
}

void FEegOpenBciSource::AppendGraphSample(const TArray<float> &BoardSample)
{
    for (int32 Channel = 0; Channel < BoardChannelCount; ++Channel)
    {
        GraphBuffer[Channel * EegConfig::GraphWindowSamples + GraphWriteIndex] = BoardSample[Channel];
    }
    GraphWriteIndex = (GraphWriteIndex + 1) % EegConfig::GraphWindowSamples;
}

void FEegOpenBciSource::AppendFrameSample(const TArray<float> &ModelSample, TArray<FEegFrame> &OutFrames)
{
    PendingFrame.Samples.Append(ModelSample);
    if (PendingFrame.Samples.Num() >= EegConfig::SamplesPerFrame * ModelChannelNames.Num())
    {
        PendingFrame.Seq = NextSeq++;
        OutFrames.Add(MoveTemp(PendingFrame));
        PendingFrame = FEegFrame();
        PendingFrame.Channels = ModelChannelNames.Num();
        PendingFrame.Samples.Reserve(EegConfig::SamplesPerFrame * ModelChannelNames.Num());
    }
}

auto FEegOpenBciSource::IsStreaming() const -> bool
{
    return bStreaming;
}

auto FEegOpenBciSource::GetSampleRate() const -> int32
{
    return SampleRateHz;
}

auto FEegOpenBciSource::GetChannelCount() const -> int32
{
    return BoardChannelCount;
}

auto FEegOpenBciSource::GetChannelNames() const -> const TArray<FString> &
{
    return ChannelNames;
}

auto FEegOpenBciSource::GetModelIndices() const -> const TArray<int32> &
{
    return OpenBciToModelIndex;
}

auto FEegOpenBciSource::GetModelChannelNames() const -> const TArray<FString> &
{
    return ModelChannelNames;
}

auto FEegOpenBciSource::GetGraphBuffer() const -> const TArray<float> &
{
    return GraphBuffer;
}

auto FEegOpenBciSource::GetGraphWriteIndex() const -> int32
{
    return GraphWriteIndex;
}
