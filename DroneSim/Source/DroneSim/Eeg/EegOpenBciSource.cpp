#include "EegOpenBciSource.h"
#include "ScenarioLog.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
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

/** Default board channel count when the mapping file does not state one (Cyton = 8) */
constexpr int32 DefaultBoardChannels = 8;

/** Seconds between (re)connect attempts */
constexpr float RetryInterval = 2.0f;

/** Picks the board id for a given channel count (16 -> Cyton+Daisy, otherwise Cyton) */
auto BoardIdForChannels(int32 ChannelCount) -> int32
{
    return ChannelCount >= 16 ? CytonDaisyBoardId : CytonBoardId;
}

/** Resolves an electrode name to its model-input index via EegConfig::ChannelNames */
auto ModelIndexForName(const FString &Name) -> int32
{
    for (int32 Index = 0; Index < EegConfig::ChannelCount; ++Index)
    {
        if (Name.Equals(EegConfig::ChannelNames[Index], ESearchCase::IgnoreCase))
        {
            return Index;
        }
    }
    return INDEX_NONE;
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

void FEegOpenBciSource::BuildChannelMap(const FString &MappingFilePath)
{
    // packaged builds ship the map at <ProjectDir>/mappings; the editor reads the repo root copy
    FString Path = MappingFilePath;
    if (Path.IsEmpty())
    {
        Path = FPaths::Combine(FPaths::ProjectDir(), TEXT("mappings/openbci_to_sim.json"));
        if (!FPaths::FileExists(Path))
        {
            Path = FPaths::Combine(FPaths::ProjectDir(), TEXT("../mappings/openbci_to_sim.json"));
        }
    }

    // defaults if the file is missing/malformed: Cyton 8ch, generic names, no model mapping
    BoardChannelCount = DefaultBoardChannels;
    OpenBciToModelIndex.Init(INDEX_NONE, BoardChannelCount);
    ChannelNames.Reset();
    for (int32 Index = 0; Index < BoardChannelCount; ++Index)
    {
        ChannelNames.Add(FString::Printf(TEXT("CH%d"), Index));
    }

    FString Json;
    TSharedPtr<FJsonObject> Root;
    if (!FFileHelper::LoadFileToString(Json, *Path) ||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<TCHAR>::Create(Json), Root) || !Root.IsValid())
    {
        FScenarioLog::Error(FString::Printf(TEXT("OpenBCI channel map not found/invalid: %s (defaults used)"), *Path));
        return;
    }

    int32 ChannelCount = DefaultBoardChannels;
    Root->TryGetNumberField(TEXT("channel_count"), ChannelCount);
    BoardChannelCount = FMath::Max(ChannelCount, 1);

    OpenBciToModelIndex.Init(INDEX_NONE, BoardChannelCount);
    ChannelNames.Reset();
    for (int32 Index = 0; Index < BoardChannelCount; ++Index)
    {
        ChannelNames.Add(FString::Printf(TEXT("CH%d"), Index));
    }

    const TSharedPtr<FJsonObject> *Mapping = nullptr;
    if (!Root->TryGetObjectField(TEXT("mapping"), Mapping))
    {
        FScenarioLog::Error(FString::Printf(TEXT("OpenBCI channel map has no 'mapping': %s"), *Path));
        return;
    }

    int32 Mapped = 0;
    for (const TPair<FString, TSharedPtr<FJsonValue>> &Pair : (*Mapping)->Values)
    {
        const int32 BoardIndex = FCString::Atoi(*Pair.Key);
        if (BoardIndex < 0 || BoardIndex >= BoardChannelCount || !Pair.Value.IsValid())
        {
            continue;
        }
        const FString Name = Pair.Value->AsString();
        ChannelNames[BoardIndex] = Name;
        const int32 ModelIndex = ModelIndexForName(Name);
        OpenBciToModelIndex[BoardIndex] = ModelIndex;
        if (ModelIndex != INDEX_NONE)
        {
            ++Mapped;
        }
    }

    FScenarioLog::Info(FString::Printf(TEXT("OpenBCI channel map: %d channels, %d placed into the %d-channel model"),
                                       BoardChannelCount, Mapped, EegConfig::ChannelCount));
}

void FEegOpenBciSource::Start(const FString &InSerialPort, const FString &MappingFilePath)
{
    BuildChannelMap(MappingFilePath);

    GraphBuffer.Init(0.0f, BoardChannelCount * EegConfig::GraphWindowSamples);
    GraphWriteIndex = 0;
    NextSeq = 0;
    PendingFrame = FEegFrame();
    PendingFrame.Seq = NextSeq;
    PendingFrame.Samples.Reserve(EegConfig::SamplesPerFrame * EegConfig::ChannelCount);
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
    const FTCHARToUTF8 Json(*ParamsJson);

    int Result = prepare_session(BoardId, Json.Get());
    if (Result == static_cast<int>(BrainFlowExitCodes::STATUS_OK))
    {
        bSessionPrepared = true;
        Result = start_stream(StreamBufferSamples, "", BoardId, Json.Get());
    }
    if (Result != static_cast<int>(BrainFlowExitCodes::STATUS_OK))
    {
        FScenarioLog::Error(FString::Printf(TEXT("OpenBCI connect failed on %s (BrainFlow code %d); retrying in %.0fs"),
                                            *Port, Result, RetryInterval));
        ReleaseBoard();
        bStreaming = false;
        return;
    }

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

    bStreaming = true;
    FScenarioLog::Info(FString::Printf(TEXT("OpenBCI streaming on %s (board %d): %d EEG channels @ %dHz"), *Port,
                                       BoardId, EegChannelRows.Num(), EegConfig::SampleRateHz));
#else
    bWantConnection = false; // nothing to retry without the SDK
    FScenarioLog::Error(TEXT("BrainFlow SDK not linked (WITH_BRAINFLOW=0): no EEG will be streamed. "
                             "Place the SDK under DroneSim/ThirdParty/BrainFlow and rebuild."));
#endif
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
    ModelSample.SetNumZeroed(EegConfig::ChannelCount);

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
    if (PendingFrame.Samples.Num() >= EegConfig::SamplesPerFrame * EegConfig::ChannelCount)
    {
        PendingFrame.Seq = NextSeq++;
        OutFrames.Add(MoveTemp(PendingFrame));
        PendingFrame = FEegFrame();
        PendingFrame.Samples.Reserve(EegConfig::SamplesPerFrame * EegConfig::ChannelCount);
    }
}

auto FEegOpenBciSource::IsStreaming() const -> bool
{
    return bStreaming;
}

auto FEegOpenBciSource::GetChannelCount() const -> int32
{
    return BoardChannelCount;
}

auto FEegOpenBciSource::GetChannelNames() const -> const TArray<FString> &
{
    return ChannelNames;
}

auto FEegOpenBciSource::GetGraphBuffer() const -> const TArray<float> &
{
    return GraphBuffer;
}

auto FEegOpenBciSource::GetGraphWriteIndex() const -> int32
{
    return GraphWriteIndex;
}
