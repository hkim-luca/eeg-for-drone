#include "EegProto.h"

namespace
{
// proto3 wire types
constexpr uint32 WireVarint = 0;
constexpr uint32 WireFixed64 = 1;
constexpr uint32 WireLengthDelimited = 2;
constexpr uint32 WireFixed32 = 5;

void AppendVarint(TArray<uint8> &Out, uint64 Value)
{
    do
    {
        uint8 Byte = Value & 0x7F;
        Value >>= 7;
        if (Value != 0)
        {
            Byte |= 0x80;
        }
        Out.Add(Byte);
    } while (Value != 0);
}

void AppendTag(TArray<uint8> &Out, uint32 FieldNumber, uint32 WireType)
{
    AppendVarint(Out, (static_cast<uint64>(FieldNumber) << 3) | WireType);
}

/** Little-endian raw bytes; all supported platforms are little-endian */
template <typename T> void AppendRaw(TArray<uint8> &Out, T Value)
{
    const int32 Offset = Out.Num();
    Out.AddUninitialized(sizeof(T));
    FMemory::Memcpy(Out.GetData() + Offset, &Value, sizeof(T));
}

/** proto3 omits fields at their default value, hence the zero checks below */
void AppendVarintField(TArray<uint8> &Out, uint32 FieldNumber, uint64 Value)
{
    if (Value != 0)
    {
        AppendTag(Out, FieldNumber, WireVarint);
        AppendVarint(Out, Value);
    }
}

void AppendDoubleField(TArray<uint8> &Out, uint32 FieldNumber, double Value)
{
    if (Value != 0.0)
    {
        AppendTag(Out, FieldNumber, WireFixed64);
        AppendRaw(Out, Value);
    }
}

void AppendStringField(TArray<uint8> &Out, uint32 FieldNumber, const FString &Value)
{
    if (!Value.IsEmpty())
    {
        const FTCHARToUTF8 Utf8(*Value);
        AppendTag(Out, FieldNumber, WireLengthDelimited);
        AppendVarint(Out, static_cast<uint64>(Utf8.Length()));
        Out.Append(reinterpret_cast<const uint8 *>(Utf8.Get()), Utf8.Length());
    }
}

void AppendPackedFloatField(TArray<uint8> &Out, uint32 FieldNumber, const TArray<float> &Values)
{
    if (!Values.IsEmpty())
    {
        AppendTag(Out, FieldNumber, WireLengthDelimited);
        AppendVarint(Out, static_cast<uint64>(Values.Num()) * sizeof(float));
        Out.Append(reinterpret_cast<const uint8 *>(Values.GetData()), Values.Num() * sizeof(float));
    }
}

/** Wraps an encoded sub-message as one oneof field of the envelope message */
auto WrapAsEnvelope(uint32 FieldNumber, const TArray<uint8> &Inner) -> TArray<uint8>
{
    TArray<uint8> Out;
    Out.Reserve(Inner.Num() + 8);
    AppendTag(Out, FieldNumber, WireLengthDelimited);
    AppendVarint(Out, static_cast<uint64>(Inner.Num()));
    Out.Append(Inner);
    return Out;
}

/** Sequential reader over a received payload; all methods return false on malformed input */
struct FProtoReader
{
    const uint8 *Data = nullptr;
    int32 Size = 0;
    int32 Pos = 0;

    auto AtEnd() const -> bool
    {
        return Pos >= Size;
    }

    auto ReadVarint(uint64 &OutValue) -> bool
    {
        OutValue = 0;
        for (int32 Shift = 0; Shift < 64 && Pos < Size; Shift += 7)
        {
            const uint8 Byte = Data[Pos++];
            OutValue |= static_cast<uint64>(Byte & 0x7F) << Shift;
            if ((Byte & 0x80) == 0)
            {
                return true;
            }
        }
        return false;
    }

    template <typename T> auto ReadRaw(T &OutValue) -> bool
    {
        if (Pos + static_cast<int32>(sizeof(T)) > Size)
        {
            return false;
        }
        FMemory::Memcpy(&OutValue, Data + Pos, sizeof(T));
        Pos += sizeof(T);
        return true;
    }

    auto ReadLengthDelimited(TConstArrayView<uint8> &OutView) -> bool
    {
        uint64 Length = 0;
        if (!ReadVarint(Length) || Pos + static_cast<int64>(Length) > Size)
        {
            return false;
        }
        OutView = TConstArrayView<uint8>(Data + Pos, static_cast<int32>(Length));
        Pos += static_cast<int32>(Length);
        return true;
    }

    /** Skips one field of the given wire type, tolerating unknown fields */
    auto SkipField(uint32 WireType) -> bool
    {
        uint64 Unused = 0;
        TConstArrayView<uint8> UnusedView;
        switch (WireType)
        {
        case WireVarint:
            return ReadVarint(Unused);
        case WireFixed64:
            double UnusedDouble;
            return ReadRaw(UnusedDouble);
        case WireLengthDelimited:
            return ReadLengthDelimited(UnusedView);
        case WireFixed32:
            float UnusedFloat;
            return ReadRaw(UnusedFloat);
        default:
            return false;
        }
    }
};

/** Parses an ActionResult sub-message (see eeg_link.proto for the field numbers) */
auto ParseActionResult(TConstArrayView<uint8> Bytes, FEegActionResult &OutResult) -> bool
{
    FProtoReader Reader{Bytes.GetData(), Bytes.Num()};
    FString ActionLabel;

    while (!Reader.AtEnd())
    {
        uint64 Tag = 0;
        if (!Reader.ReadVarint(Tag))
        {
            return false;
        }
        const uint32 FieldNumber = static_cast<uint32>(Tag >> 3);
        const uint32 WireType = static_cast<uint32>(Tag & 0x7);
        bool bOk = true;

        switch (FieldNumber)
        {
        case 1: { // action_seq
            uint64 Value = 0;
            bOk = Reader.ReadVarint(Value);
            OutResult.ActionSeq = static_cast<int64>(Value);
            break;
        }
        case 3: { // action
            TConstArrayView<uint8> View;
            bOk = Reader.ReadLengthDelimited(View);
            if (bOk)
            {
                const FUTF8ToTCHAR Converted(reinterpret_cast<const ANSICHAR *>(View.GetData()), View.Num());
                ActionLabel = FString::ConstructFromPtrSize(Converted.Get(), Converted.Length());
            }
            break;
        }
        case 4: // confidence
            bOk = Reader.ReadRaw(OutResult.Confidence);
            break;
        case 5: // t_infer_ms
            bOk = Reader.ReadRaw(OutResult.InferMs);
            break;
        case 6: // accuracy_percent
            bOk = Reader.ReadRaw(OutResult.AccuracyPercent);
            break;
        default: // eeg_seq and future fields
            bOk = Reader.SkipField(WireType);
            break;
        }

        if (!bOk)
        {
            return false;
        }
    }

    return ParseScenarioActionName(ActionLabel, OutResult.Action);
}
} // namespace

namespace EegProto
{

auto EncodeEegFrame(const FEegFrame &Frame, double SentMs) -> TArray<uint8>
{
    TArray<uint8> Inner;
    Inner.Reserve(Frame.Samples.Num() * sizeof(float) + 64);
    AppendVarintField(Inner, 1, static_cast<uint64>(Frame.Seq));
    AppendDoubleField(Inner, 2, SentMs);
    AppendVarintField(Inner, 3, EegConfig::SampleRateHz);
    AppendVarintField(Inner, 4, EegConfig::ChannelCount);
    AppendStringField(Inner, 5, ScenarioActionName(Frame.TrueAction));
    AppendPackedFloatField(Inner, 6, Frame.Samples);
    return WrapAsEnvelope(1, Inner); // ClientMessage.eeg
}

auto EncodeControlAck(int64 ActionSeq, double ControlMs) -> TArray<uint8>
{
    TArray<uint8> Inner;
    AppendVarintField(Inner, 1, static_cast<uint64>(ActionSeq));
    AppendDoubleField(Inner, 2, ControlMs);
    return WrapAsEnvelope(2, Inner); // ClientMessage.ack
}

auto DecodeServerMessage(TConstArrayView<uint8> Payload, FEegActionResult &OutResult) -> bool
{
    FProtoReader Reader{Payload.GetData(), Payload.Num()};

    while (!Reader.AtEnd())
    {
        uint64 Tag = 0;
        if (!Reader.ReadVarint(Tag))
        {
            return false;
        }

        if ((Tag >> 3) == 1 && (Tag & 0x7) == WireLengthDelimited) // ServerMessage.action
        {
            TConstArrayView<uint8> View;
            return Reader.ReadLengthDelimited(View) && ParseActionResult(View, OutResult);
        }

        if (!Reader.SkipField(static_cast<uint32>(Tag & 0x7)))
        {
            return false;
        }
    }

    return false;
}

auto NowUnixMs() -> double
{
    return (FDateTime::UtcNow() - FDateTime(1970, 1, 1)).GetTotalMilliseconds();
}

} // namespace EegProto
