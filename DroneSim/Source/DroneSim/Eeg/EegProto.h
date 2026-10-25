#pragma once

#include "Containers/ArrayView.h"
#include "CoreMinimal.h"
#include "EegTypes.h"

/**
 *  Hand-written proto3 wire-format codec for the EEG link, implementing the schema in
 *  eeg-server/proto/eeg_link.proto (the single source of truth - keep them in sync).
 *  Writing the few fixed messages by hand avoids a protoc/libprotobuf dependency in the
 *  engine build; the Python server uses regular generated protobuf code.
 *  Encoded payloads are exchanged behind a 4-byte little-endian length prefix, which
 *  FEegClient adds and strips.
 */
struct FDronePhysicsSettings;

namespace EegProto
{
/** Encodes one EEG frame as a ClientMessage payload; SentMs is stamped by the caller */
auto EncodeEegFrame(const FEegFrame &Frame, double SentMs) -> TArray<uint8>;

/** Encodes the current drone physics parameters for the dashboard's settings panel;
 *  PresetName is the airframe the values came from (CUSTOM when hand-edited) */
auto EncodePhysicsSettings(const FDronePhysicsSettings &Settings, const FString &PresetName) -> TArray<uint8>;

/** Encodes the channel layout DroneSim streams: ModelIndices and Names per board channel (which
 *  model inputs carry signal, the rest is zero-fill) and ModelChannelNames, the model-input order
 *  from sim_to_model.json, which the server checks against the model it serves */
auto EncodeDeviceInfo(TConstArrayView<int32> ModelIndices, TConstArrayView<FString> Names,
                      TConstArrayView<FString> ModelChannelNames) -> TArray<uint8>;

/** Encodes the control ack confirming that ActionSeq reached the pawn at ControlMs */
auto EncodeControlAck(int64 ActionSeq, double ControlMs) -> TArray<uint8>;

/** Decodes a ServerMessage payload; false for anything that is not a valid action result */
auto DecodeServerMessage(TConstArrayView<uint8> Payload, FEegActionResult &OutResult) -> bool;

/** Current wall clock as Unix epoch milliseconds */
auto NowUnixMs() -> double;
} // namespace EegProto
