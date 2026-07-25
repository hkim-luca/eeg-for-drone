#pragma once

#include "Containers/Array.h"
#include "Containers/ArrayView.h"
#include "CoreMinimal.h"
#include "Templates/Function.h"
#include "Templates/SharedPointer.h"

class FSocket;
class FInternetAddr;

/**
 *  Minimal TCP client for the EEG server carrying length-prefixed protobuf messages
 *  (4-byte little-endian length, then the EegProto payload), driven entirely from the
 *  game thread: the socket is non-blocking and Tick() pumps connect/send/receive, so no
 *  extra thread is needed at the ~10 small messages per second this link carries.
 *  Lost connections are retried automatically every few seconds.
 */
class FEegClient
{
  public:
    ~FEegClient();

    /** Starts connecting to Host:Port; safe to call while connected (reconnects) */
    void Connect(const FString &InHost, int32 InPort);

    /** Closes the socket and stops reconnecting */
    void Disconnect();

    /** Pumps the connection; calls OnMessage once per complete received payload
     *  (length prefix already stripped). NowSeconds is any monotonic clock. */
    void Tick(double NowSeconds, const TFunctionRef<void(TConstArrayView<uint8>)> &OnMessage);

    /** Queues one payload for sending; the length prefix is added here */
    void SendPayload(const TArray<uint8> &Payload);

    /** True while the TCP connection is established */
    auto IsConnected() const -> bool;

    /** Total payloads queued that were dropped because the connection was down */
    auto GetDroppedMessageCount() const -> int64;

  private:
    enum class EState : uint8
    {
        Idle,
        Connecting,
        Connected
    };

    /** Creates the socket and starts the non-blocking connect */
    void BeginConnect(double NowSeconds);

    /** Destroys the socket and schedules the next retry */
    void DropConnection(double NowSeconds, const TCHAR *Reason);

    /** Sends as much of OutBuffer as the socket accepts; false on socket error */
    auto PumpSend() -> bool;

    /** Reads available bytes and dispatches complete messages; false on socket error */
    auto PumpReceive(const TFunctionRef<void(TConstArrayView<uint8>)> &OnMessage) -> bool;

    FSocket *Socket = nullptr;
    TSharedPtr<FInternetAddr> ServerAddr;
    FString Host;
    int32 Port = 0;
    EState State = EState::Idle;

    /** True between Connect() and Disconnect(); enables automatic reconnects */
    bool bWantConnection = false;

    /** Monotonic time a stalled connect attempt is abandoned */
    double ConnectDeadline = 0.0;

    /** Monotonic time of the next reconnect attempt */
    double NextRetryTime = 0.0;

    /** Bytes queued for sending (length prefixes already included) */
    TArray<uint8> OutBuffer;

    /** Bytes received but not yet forming a complete message */
    TArray<uint8> InBuffer;

    /** Payloads dropped while disconnected, reported for the reliability metric */
    int64 DroppedMessages = 0;
};
