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
    OpenBciToModelIndex.Reset();

    const FString Path = MappingFilePath.IsEmpty()
                             ? FPaths::Combine(FPaths::ProjectDir(), TEXT("../mappings/openbci_to_channel.json"))
                             : MappingFilePath;

    FString Json;
    if (!FFileHelper::LoadFileToString(Json, *Path))
    {
        FScenarioLog::Error(FString::Printf(TEXT("OpenBCI channel map not found: %s (all channels zero-filled)"),
                                            *Path));
        OpenBciToModelIndex.Init(INDEX_NONE, DefaultBoardChannels);
        return;
    }

    const TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(Json);
    TSharedPtr<FJsonObject> Root;
    const TSharedPtr<FJsonObject> *MappingObj = nullptr;
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid() ||
        !Root->TryGetObjectField(TEXT("mapping"), MappingObj))
    {
        FScenarioLog::Error(FString::Printf(TEXT("OpenBCI channel map malformed: %s"), *Path));
        OpenBciToModelIndex.Init(INDEX_NONE, DefaultBoardChannels);
        return;
    }

    int32 ChannelCount = DefaultBoardChannels;
    Root->TryGetNumberField(TEXT("channel_count"), ChannelCount);
    BoardChannelCount = ChannelCount;
    OpenBciToModelIndex.Init(INDEX_NONE, FMath::Max(ChannelCount, DefaultBoardChannels));

    int32 Mapped = 0;
    for (const TPair<FString, TSharedPtr<FJsonValue>> &Pair : (*MappingObj)->Values)
    {
        const int32 BoardIndex = FCString::Atoi(*Pair.Key);
        if (BoardIndex < 0 || !Pair.Value.IsValid())
        {
            continue;
        }
        if (BoardIndex >= OpenBciToModelIndex.Num())
        {
            OpenBciToModelIndex.SetNum(BoardIndex + 1);
            for (int32 i = ChannelCount; i < OpenBciToModelIndex.Num(); ++i)
            {
                OpenBciToModelIndex[i] = INDEX_NONE;
            }
        }
        const int32 ModelIndex = ModelIndexForName(Pair.Value->AsString());
        OpenBciToModelIndex[BoardIndex] = ModelIndex;
        if (ModelIndex != INDEX_NONE)
        {
            ++Mapped;
        }
    }

    FScenarioLog::Info(FString::Printf(TEXT("OpenBCI channel map: %d/%d board channels placed into the %d-channel model"),
                                       Mapped, OpenBciToModelIndex.Num(), EegConfig::ChannelCount));
}

void FEegOpenBciSource::Start(const FString &SerialPort, const FString &MappingFilePath)
{
    BuildChannelMap(MappingFilePath);

    GraphBuffer.Init(0.0f, EegConfig::ChannelCount * EegConfig::GraphWindowSamples);
    GraphWriteIndex = 0;
    NextSeq = 0;
    PendingFrame = FEegFrame();
    PendingFrame.Seq = NextSeq;
    PendingFrame.Samples.Reserve(EegConfig::SamplesPerFrame * EegConfig::ChannelCount);
    EegChannelRows.Reset();
    bStreaming = false;

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
        FScenarioLog::Error(FString::Printf(TEXT("OpenBCI start failed on %s: %hs (code %d)"), *SerialPort, Error.what(),
                                            Error.exit_code));
        Stop();
    }
#else
    (void)SerialPort;
    FScenarioLog::Error(TEXT("BrainFlow SDK not linked (WITH_BRAINFLOW=0): no EEG will be streamed. "
                             "Place the SDK under DroneSim/ThirdParty/BrainFlow and rebuild."));
#endif
}

void FEegOpenBciSource::Stop()
{
    bStreaming = false;
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
            FScenarioLog::Error(FString::Printf(TEXT("OpenBCI stop error: %hs (code %d)"), Error.what(), Error.exit_code));
        }
        delete Board;
        BoardHandle = nullptr;
    }
#endif
}

void FEegOpenBciSource::Tick(float DeltaTime, TArray<FEegFrame> &OutFrames)
{
    (void)DeltaTime; // the board's own sample clock drives pacing, not the frame delta
    if (!bStreaming)
    {
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
        FScenarioLog::Error(FString::Printf(TEXT("OpenBCI read error: %hs (code %d)"), Error.what(), Error.exit_code));
        return;
    }

    const int32 SampleCount = Data.get_size(1);
    TArray<float> ModelSample;
    ModelSample.SetNumZeroed(EegConfig::ChannelCount);
    for (int32 Sample = 0; Sample < SampleCount; ++Sample)
    {
        for (int32 &Value : ModelSample)
        {
            Value = 0.0f; // zero-fill every model channel the board does not cover
        }
        const int32 BoardChannels = FMath::Min(EegChannelRows.Num(), OpenBciToModelIndex.Num());
        for (int32 Channel = 0; Channel < BoardChannels; ++Channel)
        {
            const int32 ModelIndex = OpenBciToModelIndex[Channel];
            if (ModelIndex != INDEX_NONE)
            {
                ModelSample[ModelIndex] = static_cast<float>(Data.at(EegChannelRows[Channel], Sample));
            }
        }
        AppendSample(ModelSample, OutFrames);
    }
#else
    (void)OutFrames;
#endif
}

void FEegOpenBciSource::AppendSample(const TArray<float> &ModelSample, TArray<FEegFrame> &OutFrames)
{
    for (int32 Channel = 0; Channel < EegConfig::ChannelCount; ++Channel)
    {
        const float Value = ModelSample[Channel];
        PendingFrame.Samples.Add(Value);
        GraphBuffer[Channel * EegConfig::GraphWindowSamples + GraphWriteIndex] = Value;
    }
    GraphWriteIndex = (GraphWriteIndex + 1) % EegConfig::GraphWindowSamples;

    if (PendingFrame.Samples.Num() >= EegConfig::SamplesPerFrame * EegConfig::ChannelCount)
    {
        PendingFrame.Seq = NextSeq++;
        OutFrames.Add(MoveTemp(PendingFrame));
        PendingFrame = FEegFrame();
        PendingFrame.Samples.Reserve(EegConfig::SamplesPerFrame * EegConfig::ChannelCount);
    }
}

auto FEegOpenBciSource::GetGraphBuffer() const -> const TArray<float> &
{
    return GraphBuffer;
}

auto FEegOpenBciSource::GetGraphWriteIndex() const -> int32
{
    return GraphWriteIndex;
}

auto FEegOpenBciSource::IsStreaming() const -> bool
{
    return bStreaming;
}
