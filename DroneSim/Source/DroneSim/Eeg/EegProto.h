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
namespace EegProto
{
/** Encodes one EEG frame as a ClientMessage payload; SentMs is stamped by the caller */
auto EncodeEegFrame(const FEegFrame &Frame, double SentMs) -> TArray<uint8>;

/** Encodes the control ack confirming that ActionSeq reached the pawn at ControlMs */
auto EncodeControlAck(int64 ActionSeq, double ControlMs) -> TArray<uint8>;

/** Decodes a ServerMessage payload; false for anything that is not a valid action result */
auto DecodeServerMessage(TConstArrayView<uint8> Payload, FEegActionResult &OutResult) -> bool;

/** Current wall clock as Unix epoch milliseconds */
auto NowUnixMs() -> double;
} // namespace EegProto
