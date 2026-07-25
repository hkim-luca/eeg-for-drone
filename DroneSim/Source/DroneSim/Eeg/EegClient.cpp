#include "EegClient.h"
#include "ScenarioLog.h"
#include "SocketSubsystem.h"
#include "Sockets.h"

namespace
{
/** Seconds between reconnect attempts after a failure */
constexpr double RetryIntervalSeconds = 2.0;

/** Seconds after which a stalled connect attempt is abandoned */
constexpr double ConnectTimeoutSeconds = 3.0;

/** An announced message longer than this is a protocol error, not real traffic */
constexpr uint32 MaxMessageBytes = 1024 * 1024;

/** Reads the 4-byte little-endian length prefix at the start of the buffer */
auto ReadLengthPrefix(const TArray<uint8> &Buffer) -> uint32
{
    uint32 Length = 0;
    FMemory::Memcpy(&Length, Buffer.GetData(), sizeof(Length));
    return Length;
}
} // namespace

FEegClient::~FEegClient()
{
    Disconnect();
}

void FEegClient::Connect(const FString &InHost, int32 InPort)
{
    Disconnect();
    Host = InHost;
    Port = InPort;
    bWantConnection = true;
    NextRetryTime = 0.0; // connect on the next Tick
}

void FEegClient::Disconnect()
{
    bWantConnection = false;
    State = EState::Idle;
    OutBuffer.Reset();
    InBuffer.Reset();

    if (Socket != nullptr)
    {
        Socket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
        Socket = nullptr;
    }
}

void FEegClient::Tick(double NowSeconds, const TFunctionRef<void(TConstArrayView<uint8>)> &OnMessage)
{
    if (!bWantConnection)
    {
        return;
    }

    if (State == EState::Idle && NowSeconds >= NextRetryTime)
    {
        BeginConnect(NowSeconds);
    }

    if (State == EState::Connecting)
    {
        // a non-blocking connect is complete when the socket turns writable; do not use
        // GetConnectionState() to promote - right after creation it reports SCS_Connected
        // for up to 5 seconds regardless of the actual handshake, which made the first
        // send fail with "not connected" and drop the link immediately
        if (Socket->Wait(ESocketWaitConditions::WaitForWrite, FTimespan::Zero()))
        {
            State = EState::Connected;
            FScenarioLog::Info(FString::Printf(TEXT("EEG server connected: %s:%d"), *Host, Port));
        }
        else if (Socket->GetConnectionState() == SCS_ConnectionError || NowSeconds >= ConnectDeadline)
        {
            DropConnection(NowSeconds, TEXT("connect failed"));
        }
    }

    if (State == EState::Connected)
    {
        if (!PumpSend() || !PumpReceive(OnMessage))
        {
            DropConnection(NowSeconds, TEXT("connection lost"));
        }
    }
}

void FEegClient::SendPayload(const TArray<uint8> &Payload)
{
    if (State != EState::Connected)
    {
        ++DroppedMessages;
        return;
    }

    const auto Length = static_cast<uint32>(Payload.Num());
    const int32 Offset = OutBuffer.Num();
    OutBuffer.AddUninitialized(sizeof(Length));
    FMemory::Memcpy(OutBuffer.GetData() + Offset, &Length, sizeof(Length));
    OutBuffer.Append(Payload);
}

auto FEegClient::IsConnected() const -> bool
{
    return State == EState::Connected;
}

auto FEegClient::GetDroppedMessageCount() const -> int64
{
    return DroppedMessages;
}

void FEegClient::BeginConnect(double NowSeconds)
{
    ISocketSubsystem *Sockets = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);

    // resolve every attempt so the server can come up or move while we retry
    FAddressInfoResult Resolved =
        Sockets->GetAddressInfo(*Host, nullptr, EAddressInfoFlags::Default, NAME_None, SOCKTYPE_Streaming);
    if (Resolved.ReturnCode != SE_NO_ERROR || Resolved.Results.IsEmpty())
    {
        FScenarioLog::Error(FString::Printf(TEXT("EEG server host not resolved: %s"), *Host));
        NextRetryTime = NowSeconds + RetryIntervalSeconds;
        return;
    }

    ServerAddr = Resolved.Results[0].Address;
    ServerAddr->SetPort(Port);

    Socket = Sockets->CreateSocket(NAME_Stream, TEXT("EegClient"), ServerAddr->GetProtocolType());
    if (Socket == nullptr)
    {
        FScenarioLog::Error(TEXT("EEG client socket creation failed"));
        NextRetryTime = NowSeconds + RetryIntervalSeconds;
        return;
    }

    Socket->SetNonBlocking(true);
    Socket->SetNoDelay(true);
    Socket->Connect(*ServerAddr); // non-blocking: completion is polled in Tick()

    State = EState::Connecting;
    ConnectDeadline = NowSeconds + ConnectTimeoutSeconds;
}

void FEegClient::DropConnection(double NowSeconds, const TCHAR *Reason)
{
    FScenarioLog::Error(
        FString::Printf(TEXT("EEG server %s:%d - %s; retrying in %.0fs"), *Host, Port, Reason, RetryIntervalSeconds));

    if (Socket != nullptr)
    {
        Socket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
        Socket = nullptr;
    }

    State = EState::Idle;
    OutBuffer.Reset();
    InBuffer.Reset();
    NextRetryTime = NowSeconds + RetryIntervalSeconds;
}

auto FEegClient::PumpSend() -> bool
{
    while (!OutBuffer.IsEmpty())
    {
        int32 BytesSent = 0;
        if (!Socket->Send(OutBuffer.GetData(), OutBuffer.Num(), BytesSent))
        {
            // a full send buffer reports a would-block error; anything else is fatal
            return ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLastErrorCode() == SE_EWOULDBLOCK;
        }
        if (BytesSent <= 0)
        {
            return true; // nothing accepted this tick; try again next tick
        }
        OutBuffer.RemoveAt(0, BytesSent, EAllowShrinking::No);
    }
    return true;
}

auto FEegClient::PumpReceive(const TFunctionRef<void(TConstArrayView<uint8>)> &OnMessage) -> bool
{
    uint8 Chunk[16 * 1024];

    while (true)
    {
        int32 BytesRead = 0;
        if (!Socket->Recv(Chunk, sizeof(Chunk), BytesRead))
        {
            // BytesRead == 0 is an orderly shutdown by the server (recv itself succeeded,
            // so the last error code would be stale); negative means a real socket error
            return BytesRead < 0 &&
                   ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLastErrorCode() == SE_EWOULDBLOCK;
        }

        InBuffer.Append(Chunk, BytesRead);

        // dispatch every complete length-prefixed message in the buffer
        while (InBuffer.Num() >= static_cast<int32>(sizeof(uint32)))
        {
            const uint32 Length = ReadLengthPrefix(InBuffer);
            if (Length > MaxMessageBytes)
            {
                FScenarioLog::Error(
                    FString::Printf(TEXT("EEG server announced an oversized message (%u bytes)"), Length));
                return false; // framing is broken; force a reconnect
            }
            const int32 Total = static_cast<int32>(sizeof(uint32) + Length);
            if (InBuffer.Num() < Total)
            {
                break;
            }
            OnMessage(TConstArrayView<uint8>(InBuffer.GetData() + sizeof(uint32), static_cast<int32>(Length)));
            InBuffer.RemoveAt(0, Total, EAllowShrinking::No);
        }

        if (BytesRead < static_cast<int32>(sizeof(Chunk)))
        {
            return true; // drained everything currently available
        }
    }
}
