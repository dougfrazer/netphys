#include "network_c.h"
#include "../netphys_common/common.h"
#include "../netphys_common/log.h"

#include <WinSock2.h>
#include <Windows.h>

#include <iostream>

#include "client.h"
#include "world_c.h"

static constexpr int SERVER_PORT = 5555;
static const char* SERVER_ADDR = "127.0.0.1";

static constexpr int BUFFER_SIZE = (64) * (1024);
static char s_sendBuffer[BUFFER_SIZE];
static char s_recvBuffer[BUFFER_SIZE];
static int s_sendBufferSize = 0;
static int s_recvBufferSize = 0;

static SOCKET s_serverSocket = INVALID_SOCKET;

enum CONNECTION_STATE
{
    CONNECTION_STATE_NONE,

    CONNECTION_STATE_CREATED,
    CONNECTION_STATE_SENT_ACK,
    CONNECTION_STATE_OPEN,
};
static CONNECTION_STATE s_state = CONNECTION_STATE_NONE;
static DWORD s_connectionStateTime = 0;
static FrameNum s_connectionAckServerFrame = 0;
static constexpr DWORD CONNECTION_STATE_TIMEOUT = 2000; // if we don't get an ack in 2 seconds, send another

//-------------------------------------------------------------------------------------------------
static void SendServerNewConnection()
{
    LOG("Sending new connection message to server...");
    ServerNewConnection msg;
    msg.Finalize();
    Net_C_Send(&msg);
    s_connectionStateTime = GetTickCount();
}
//-------------------------------------------------------------------------------------------------
static void SendServerConnectionAck()
{
    LOG("Sending ConnectionAck for server frame=%d", s_connectionAckServerFrame);
    ServerNewConnectionAck msg;
    msg.PutFrameNum(s_connectionAckServerFrame);
    msg.Finalize();
    Net_C_Send(&msg);
    s_connectionStateTime = GetTickCount();
}
//-------------------------------------------------------------------------------------------------
static void SendServerWorldUpdateAck(FrameNum frameNum)
{
    LOG("Sending WorldUpdateAck for server frame=%d", frameNum);
    ServerWorldUpdateAck msg;
    msg.PutFrameNum(frameNum);
    msg.Finalize();
    Net_C_Send(&msg);
}
//-------------------------------------------------------------------------------------------------
bool Net_C_Init()
{
    WSADATA wsd;
    if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0)
    {
        LOG_ERROR("Failed to initialize windows socket library");
        return false;
    }

    // Create the socket, and attempt to connect to the server
    s_serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s_serverSocket == INVALID_SOCKET)
    {
        LOG_ERROR("Failed to create socket (%d)", WSAGetLastError());
        return false;
    }

    // Change the socket mode to non-block
    ULONG NonBlock = 1;
    int error = ioctlsocket(s_serverSocket, FIONBIO, &NonBlock);
    if (error == SOCKET_ERROR)
    {
        LOG_ERROR("Failed to set non-blocking on socket (%d)", error);
        return false;
    }

    LOG_CONSOLE("Successfully created socket...");
    s_state = CONNECTION_STATE_CREATED;

	return true;
}

//-------------------------------------------------------------------------------------------------
bool Net_C_Deinit()
{
    if (s_serverSocket != INVALID_SOCKET)
    {
        if (!closesocket(s_serverSocket))
        {
            LOG_ERROR("Failed to close socket");
            return false;
        }
    }
    s_serverSocket = INVALID_SOCKET;

    if (!WSACleanup())
    {
        LOG_ERROR("Failed to cleanup windows socket library");
        return false;
    }
        
	return true;
}

//-------------------------------------------------------------------------------------------------
static bool Send()
{
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);
    addr.sin_port = htons(SERVER_PORT);
    int ret = sendto(s_serverSocket, s_sendBuffer, s_sendBufferSize, 0, (sockaddr*)&addr, sizeof(addr));
    if (ret == SOCKET_ERROR)
    {
        int error = WSAGetLastError();
        if (error == WSAEWOULDBLOCK)
        {
            return true;
        }
        LOG_ERROR("Failed to send(), (%d)", error);
        return false;
    }
    if (ret != s_sendBufferSize)
    {
        // error for now if we couldn't send the whole thing
        LOG_ERROR("Didn't send all the packets");
        return false;
    }
    s_sendBufferSize = 0;
    return true;
}
//-------------------------------------------------------------------------------------------------
static bool Recieve()
{
    if(s_recvBufferSize)
    {
        LOG_ERROR("Can't recieve until we clear our buffer");
        return false;
    }

    sockaddr_in addr;
    int sockAddrSize = sizeof(sockaddr_in);
    while (true)
    {
        int bufferRemainingSize = BUFFER_SIZE - s_recvBufferSize;
        if (!bufferRemainingSize)
        {
            LOG_ERROR("No space to recieve packets... getting backed up");
            return false;
        }

        int recvLength = recvfrom(s_serverSocket, &s_recvBuffer[s_recvBufferSize], BUFFER_SIZE - s_recvBufferSize, 0, (sockaddr*)&addr, &sockAddrSize);
        if (recvLength == INVALID_SOCKET)
        {
            int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK)
            {
                return true; // no big deal
            }
            if (error == WSAECONNRESET)
            {
                // i think we can ignore this?  UDP is connectionless?
                return true;
            }
            // error with accept
            LOG_ERROR("ERROR: Got error on recvfrom: (%d)", WSAGetLastError());
            return false;
        }
        else
        {
            if (addr.sin_addr.S_un.S_addr != inet_addr(SERVER_ADDR) && addr.sin_port != SERVER_PORT)
            {
                LOG_ERROR("Got a packet from an address other than the server?");
                return false;
            }
            s_recvBufferSize += recvLength;
        }
    }
    return true;
}
//-------------------------------------------------------------------------------------------------
static bool ProcessPacket(Packet* p)
{
    switch (p->GetType())
    {
        case CLIENT_WORLD_STATE_UPDATE_ID:
        {
            ClientWorldStateUpdatePacket* msg = (ClientWorldStateUpdatePacket*)(p);
            FrameNum frameID = World_C_HandleUpdate(msg);
            SendServerWorldUpdateAck(frameID);

            if (s_state == CONNECTION_STATE_SENT_ACK)
            {
                // no need to keep retrying the ack
                LOG_CONSOLE("Got our first world update, setting state to open.");
                s_state = CONNECTION_STATE_OPEN;
            }
            else if (s_state != CONNECTION_STATE_OPEN)
            {
                LOG_ERROR("In an unexpected state when recieving world state update: %d", s_state);
            }

        }
        break;

        case CLIENT_HANDLE_WORLD_RESET_ID:
        {
            ClientHandleWorldStateResetPacket* msg = (ClientHandleWorldStateResetPacket*)(p);
            ResetCamera();
        }
        break;

        case CLIENT_NEW_CONNECTION_ID:
        {
            ClientNewConnection* msg = (ClientNewConnection*)(p);
            FrameNum frameID = World_C_HandleNewConnection(msg);
            s_state = CONNECTION_STATE_SENT_ACK;
            s_connectionAckServerFrame = frameID;
            SendServerConnectionAck();
        }
        break;

        default:
        {
            LOG_ERROR("Unknown packet (%d)", p->GetType());
            return false; // unknown packet
        }
    }
    return true;
}
//-------------------------------------------------------------------------------------------------
static bool Process()
{
    if(!s_recvBufferSize)
        return true;

    int idx = 0;
    while (idx < s_recvBufferSize)
    {
        Packet p;
        int size = p.Deserialize(&s_recvBuffer[idx]);
        if (!size)
        {
            LOG_ERROR("Error parsing packet");
            return false;
        }
        bool success = ProcessPacket(&p);
        p.data.Free();
        if (!success)
        {
            LOG_ERROR("No handler for packet");
            return false;
        }
        idx += size;
    }

    if (idx != s_recvBufferSize)
    {
        LOG_ERROR("Got a partial packet or buffer error");
        return false; // somehow didn't get packets to line up
    }
    s_recvBufferSize = 0;
    return true;
}
//-------------------------------------------------------------------------------------------------
bool Net_C_Update()
{
    if (s_serverSocket == INVALID_SOCKET)
    {
        LOG_ERROR("No server socket");
        return false;
    }

    if (s_state < CONNECTION_STATE_OPEN)
    {
        if(!s_connectionStateTime || GetTickCount() - s_connectionStateTime > CONNECTION_STATE_TIMEOUT)
        {
            switch (s_state)
            {
                case CONNECTION_STATE_CREATED:
                    if(s_connectionStateTime) { LOG_CONSOLE("Timed out on new connection message, sending again..."); }
                    SendServerNewConnection();
                    break;
                case CONNECTION_STATE_SENT_ACK:
                    if(s_connectionStateTime) { LOG_CONSOLE("Timed out on new connection ack message, sending again..."); }
                    SendServerConnectionAck();
                    break;
                default:
                    LOG_ERROR("Timed out in an invalid state (%d)", s_state);
                    break;
            }
        }
    }

    // process any data that we recieved
    if (!Process())
    {
        LOG_ERROR("Process failed");
        return false;
    }

	// Send inputs to server
    if (!Send())
    {
        LOG_ERROR("Send failed");
        return false;
    }

	// check for new data from server
    if (!Recieve())
    {
        LOG_ERROR("Recieve failed");
        return false;
    }

	// notify world of error(? should this happen here?)
	return true;
}
//-------------------------------------------------------------------------------------------------
void Net_C_Send(Packet* packet)
{
    int numBytes = packet->Serialize(&s_sendBuffer[s_sendBufferSize], BUFFER_SIZE - s_sendBufferSize);
    if (!numBytes)
    {
        LOG_CONSOLE("Too many bytes in send buffer");
        return;
    }
    s_sendBufferSize += numBytes;

}
//-------------------------------------------------------------------------------------------------