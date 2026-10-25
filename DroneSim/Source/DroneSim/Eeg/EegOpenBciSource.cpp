#include "EegOpenBciSource.h"
#include "ScenarioLog.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#if WITH_BRAINFLOW
#include "board_shim.h"
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
    const FString Path = MappingFilePath.IsEmpty()
                             ? FPaths::Combine(FPaths::ProjectDir(), TEXT("../mappings/openbci_to_sim.json"))
                             : MappingFilePath;

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
        try
        {
            BoardShim::set_log_file(TCHAR_TO_UTF8(*(FPaths::ProjectLogDir() / TEXT("brainflow.log"))));
            BoardShim::enable_dev_board_logger();
        }
        catch (const BrainFlowException &)
        {
        }
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

    try
    {
        const int BoardId = BoardIdForChannels(BoardChannelCount);
        BrainFlowInputParams Params;
        Params.serial_port = TCHAR_TO_UTF8(*Port);
        auto *Board = new BoardShim(BoardId, Params);
        BoardHandle = Board; // set before prepare so a failed prepare/start still gets released
        Board->prepare_session();
        Board->start_stream();

        EegChannelRows.Reset();
        for (int Row : BoardShim::get_eeg_channels(BoardId))
        {
            EegChannelRows.Add(Row);
        }
        bStreaming = true;
        FScenarioLog::Info(FString::Printf(TEXT("OpenBCI streaming on %s (board %d): %d EEG channels @ %dHz"),
                                           *Port, BoardId, EegChannelRows.Num(), EegConfig::SampleRateHz));
    }
    catch (const BrainFlowException &Error)
    {
        FScenarioLog::Error(FString::Printf(TEXT("OpenBCI connect failed on %s: %hs (code %d); retrying in %.0fs"),
                                            *Port, Error.what(), Error.exit_code, RetryInterval));
        ReleaseBoard();
        bStreaming = false;
    }
#else
    bWantConnection = false; // nothing to retry without the SDK
    FScenarioLog::Error(TEXT("BrainFlow SDK not linked (WITH_BRAINFLOW=0): no EEG will be streamed. "
                             "Place the SDK under DroneSim/ThirdParty/BrainFlow and rebuild."));
#endif
}

void FEegOpenBciSource::ReleaseBoard()
{
#if WITH_BRAINFLOW
    if (BoardHandle != nullptr)
    {
        auto *Board = static_cast<BoardShim *>(BoardHandle);
        // release_session unregisters the board in BrainFlow's process-wide registry even when
        // prepare/start only partly succeeded; skipping it leaks the registration and the next
        // attempt fails with ANOTHER_BOARD_IS_CREATED (code 16). It also stops any running stream,
        // so no separate stop_stream is needed. Swallow errors (e.g. board was never prepared).
        try
        {
            Board->release_session();
        }
        catch (const BrainFlowException &)
        {
        }
        delete Board;
        BoardHandle = nullptr;
    }
#endif
}

void FEegOpenBciSource::Stop()
{
    bWantConnection = false;
    bStreaming = false;
    ReleaseBoard();
}

void FEegOpenBciSource::Tick(float DeltaTime, TArray<FEegFrame> &OutFrames)
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
    auto *Board = static_cast<BoardShim *>(BoardHandle);
    BrainFlowArray<double, 2> Data;
    try
    {
        Data = Board->get_board_data();
    }
    catch (const BrainFlowException &Error)
    {
        FScenarioLog::Error(FString::Printf(TEXT("OpenBCI read error: %hs (code %d); reconnecting"), Error.what(),
                                            Error.exit_code));
        ReleaseBoard();
        bStreaming = false;
        SecondsSinceRetry = 0.0f;
        return;
    }

    const int32 SampleCount = Data.get_size(1);
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
            const float Value = static_cast<float>(Data.at(EegChannelRows[Channel], Sample));
            BoardSample[Channel] = Value;
            const int32 ModelIndex = OpenBciToModelIndex[Channel];
            if (ModelIndex != INDEX_NONE)
            {
                ModelSample[ModelIndex] = Value;
            }
        }
        AppendGraphSample(BoardSample);
        AppendFrameSample(ModelSample, OutFrames);
    }
#else
    (void)OutFrames;
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
