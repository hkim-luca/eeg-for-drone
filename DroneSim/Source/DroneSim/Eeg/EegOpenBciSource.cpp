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
    try
    {
        const int BoardId = BoardIdForChannels(BoardChannelCount);
        BrainFlowInputParams Params;
        Params.serial_port = TCHAR_TO_UTF8(*SerialPort);
        auto *Board = new BoardShim(BoardId, Params);
        Board->prepare_session();
        Board->start_stream();
        BoardHandle = Board;

        EegChannelRows.Reset();
        for (int Row : BoardShim::get_eeg_channels(BoardId))
        {
            EegChannelRows.Add(Row);
        }
        bStreaming = true;
        FScenarioLog::Info(FString::Printf(TEXT("OpenBCI streaming on %s (board %d): %d EEG channels @ %dHz"),
                                           *SerialPort, BoardId, EegChannelRows.Num(), EegConfig::SampleRateHz));
    }
    catch (const BrainFlowException &Error)
    {
        FScenarioLog::Error(FString::Printf(TEXT("OpenBCI connect failed on %s: %hs (code %d); retrying in %.0fs"),
                                            *SerialPort, Error.what(), Error.exit_code, RetryInterval));
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
        try
        {
            if (Board->is_prepared())
            {
                Board->stop_stream();
                Board->release_session();
            }
        }
        catch (const BrainFlowException &Error)
        {
            FScenarioLog::Error(FString::Printf(TEXT("OpenBCI release error: %hs (code %d)"), Error.what(),
                                                Error.exit_code));
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
