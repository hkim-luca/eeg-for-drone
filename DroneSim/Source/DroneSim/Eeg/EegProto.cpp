#include "EegProto.h"
#include "DronePhysicsSettings.h"

#include "Dom/JsonObject.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

// JSON wire-format codec for the EEG link, implementing the message shapes in
// eeg-server/proto/eeg_link.proto (now a schema reference; the server (de)serializes the
// same JSON). Payloads are UTF-8 JSON objects exchanged behind a 4-byte little-endian
// length prefix, which FEegClient adds and strips (so this file deals in bare payloads).

namespace
{
/** Encodes an FString as bare UTF-8 bytes (no length prefix, no null terminator) */
auto Utf8Bytes(const FString &Text) -> TArray<uint8>
{
    const FTCHARToUTF8 Utf8(*Text);
    TArray<uint8> Out;
    Out.Append(reinterpret_cast<const uint8 *>(Utf8.Get()), Utf8.Length());
    return Out;
}

/** Decodes bare UTF-8 bytes back to an FString */
auto Utf8ToString(TConstArrayView<uint8> Bytes) -> FString
{
    const FUTF8ToTCHAR Converted(reinterpret_cast<const ANSICHAR *>(Bytes.GetData()), Bytes.Num());
    return FString::ConstructFromPtrSize(Converted.Get(), Converted.Length());
}

/** Serializes a JSON object to a framed-ready UTF-8 byte payload (condensed, no whitespace) */
auto SerializeObject(const TSharedRef<FJsonObject> &Object) -> TArray<uint8>
{
    FString Text;
    const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
        TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Text);
    FJsonSerializer::Serialize(Object, Writer);
    return Utf8Bytes(Text);
}

/** Appends a float array as a JSON number array field */
void SetFloatArrayField(const TSharedRef<FJsonObject> &Object, const TCHAR *Field, const TArray<float> &Values)
{
    TArray<TSharedPtr<FJsonValue>> Array;
    Array.Reserve(Values.Num());
    for (const float Value : Values)
    {
        Array.Add(MakeShared<FJsonValueNumber>(Value));
    }
    Object->SetArrayField(Field, Array);
}

/** Reads a numeric field as double, returning 0 when absent */
auto GetNumber(const TSharedPtr<FJsonObject> &Object, const TCHAR *Field) -> double
{
    double Value = 0.0;
    Object->TryGetNumberField(Field, Value);
    return Value;
}
} // namespace

namespace EegProto
{

auto EncodeEegFrame(const FEegFrame &Frame, double SentMs) -> TArray<uint8>
{
    const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
    Object->SetStringField(TEXT("type"), TEXT("eeg"));
    Object->SetNumberField(TEXT("seq"), static_cast<double>(Frame.Seq));
    Object->SetNumberField(TEXT("t_sent_ms"), SentMs);
    Object->SetNumberField(TEXT("rate"), EegConfig::SampleRateHz);
    Object->SetNumberField(TEXT("channels"), EegConfig::ChannelCount);
    SetFloatArrayField(Object, TEXT("data"), Frame.Samples);
    return SerializeObject(Object);
}

auto EncodePhysicsSettings(const FDronePhysicsSettings &Settings, const FString &PresetName) -> TArray<uint8>
{
    // field names follow message PhysicsSettings in eeg_link.proto
    const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
    Object->SetStringField(TEXT("type"), TEXT("physics"));
    Object->SetNumberField(TEXT("mass_kg"), Settings.MassKg);
    Object->SetNumberField(TEXT("arm_length_m"), Settings.ArmLengthM);
    Object->SetNumberField(TEXT("inertia_xx"), Settings.InertiaXX);
    Object->SetNumberField(TEXT("inertia_yy"), Settings.InertiaYY);
    Object->SetNumberField(TEXT("inertia_zz"), Settings.InertiaZZ);
    Object->SetNumberField(TEXT("motor_time_constant_s"), Settings.MotorTimeConstantS);
    Object->SetNumberField(TEXT("thrust_coefficient"), Settings.ThrustCoefficient);
    Object->SetNumberField(TEXT("torque_coefficient"), Settings.TorqueCoefficient);
    Object->SetNumberField(TEXT("motor_max_rad_s"), Settings.MotorMaxRadS);
    Object->SetNumberField(TEXT("rotor_inertia"), Settings.RotorInertiaKgM2);
    Object->SetNumberField(TEXT("rotor_radius_m"), Settings.RotorRadiusM);
    Object->SetNumberField(TEXT("air_density"), Settings.AirDensity);
    Object->SetNumberField(TEXT("drag_linear"), Settings.DragLinear);
    Object->SetNumberField(TEXT("drag_quadratic"), Settings.DragQuadratic);
    Object->SetNumberField(TEXT("gravity"), Settings.GravityMS2);
    Object->SetNumberField(TEXT("ground_effect_strength"), Settings.GroundEffectStrength);
    Object->SetNumberField(TEXT("wind_x"), Settings.WindXMS);
    Object->SetNumberField(TEXT("wind_y"), Settings.WindYMS);
    Object->SetNumberField(TEXT("gust_intensity"), Settings.GustIntensityMS);
    Object->SetNumberField(TEXT("max_tilt_deg"), Settings.MaxTiltDeg);
    Object->SetNumberField(TEXT("max_speed_ms"), Settings.MaxSpeedMS);
    Object->SetNumberField(TEXT("substep_hz"), Settings.SubstepHz);
    Object->SetStringField(TEXT("preset_name"), PresetName);
    Object->SetNumberField(TEXT("motor_count"), Settings.MotorCount);
    return SerializeObject(Object);
}

auto EncodeControlAck(int64 ActionSeq, double ControlMs) -> TArray<uint8>
{
    const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
    Object->SetStringField(TEXT("type"), TEXT("ack"));
    Object->SetNumberField(TEXT("action_seq"), static_cast<double>(ActionSeq));
    Object->SetNumberField(TEXT("t_control_ms"), ControlMs);
    return SerializeObject(Object);
}

auto DecodeServerMessage(TConstArrayView<uint8> Payload, FEegActionResult &OutResult) -> bool
{
    const TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(Utf8ToString(Payload));
    TSharedPtr<FJsonObject> Object;
    if (!FJsonSerializer::Deserialize(Reader, Object) || !Object.IsValid())
    {
        return false;
    }

    FString Type;
    if (!Object->TryGetStringField(TEXT("type"), Type) || Type != TEXT("action"))
    {
        return false; // only action results are expected from the server
    }

    OutResult.ActionSeq = static_cast<int64>(GetNumber(Object, TEXT("action_seq")));
    OutResult.Confidence = static_cast<float>(GetNumber(Object, TEXT("confidence")));
    OutResult.InferDurationMs = GetNumber(Object, TEXT("infer_duration_ms"));

    const TArray<TSharedPtr<FJsonValue>> *Probs = nullptr;
    if (Object->TryGetArrayField(TEXT("action_probs"), Probs))
    {
        const int32 Count = FMath::Min(Probs->Num(), EegConfig::ProbCount);
        for (int32 Index = 0; Index < Count; ++Index)
        {
            OutResult.ActionProbs[Index] = static_cast<float>((*Probs)[Index]->AsNumber());
        }
    }

    FEegMetrics &Metrics = OutResult.Metrics;
    Metrics.LatencyDeviceToInferMeanMs = static_cast<float>(GetNumber(Object, TEXT("latency_device_to_infer_mean_ms")));
    Metrics.LatencyDeviceToInferLastMs = static_cast<float>(GetNumber(Object, TEXT("latency_device_to_infer_last_ms")));
    Metrics.LatencyDeviceToInferP95Ms = static_cast<float>(GetNumber(Object, TEXT("latency_device_to_infer_p95_ms")));
    Metrics.LatencyInferToControlMeanMs =
        static_cast<float>(GetNumber(Object, TEXT("latency_infer_to_control_mean_ms")));
    Metrics.LatencyInferToControlLastMs =
        static_cast<float>(GetNumber(Object, TEXT("latency_infer_to_control_last_ms")));
    Metrics.LatencyInferToControlP95Ms = static_cast<float>(GetNumber(Object, TEXT("latency_infer_to_control_p95_ms")));
    Metrics.LatencyDeviceToControlMeanMs =
        static_cast<float>(GetNumber(Object, TEXT("latency_device_to_control_mean_ms")));
    Metrics.LatencyDeviceToControlLastMs =
        static_cast<float>(GetNumber(Object, TEXT("latency_device_to_control_last_ms")));
    Metrics.LatencyDeviceToControlP95Ms =
        static_cast<float>(GetNumber(Object, TEXT("latency_device_to_control_p95_ms")));
    Metrics.ReliabilityOverallPercent = static_cast<float>(GetNumber(Object, TEXT("reliability_overall_percent")));
    Metrics.ReliabilityFramePercent = static_cast<float>(GetNumber(Object, TEXT("reliability_frame_percent")));
    Metrics.ReliabilityFramesLost = static_cast<int32>(GetNumber(Object, TEXT("reliability_frames_lost")));
    Metrics.ReliabilityAckPercent = static_cast<float>(GetNumber(Object, TEXT("reliability_ack_percent")));

    FString ActionLabel;
    Object->TryGetStringField(TEXT("action"), ActionLabel);
    return ParseScenarioActionName(ActionLabel, OutResult.Action);
}

auto NowUnixMs() -> double
{
    return (FDateTime::UtcNow() - FDateTime(1970, 1, 1)).GetTotalMilliseconds();
}

} // namespace EegProto
