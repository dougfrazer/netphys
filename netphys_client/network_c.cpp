#include "network_c.h"
#include "../netphys_common/common.h"
#include "../netphys_common/log.h"

#include <WinSock2.h>
#include <Windows.h>

#include <iostream>

#include "client.h"


static constexpr int SERVER_PORT = 5555;
static const char* SERVER_ADDR = "127.0.0.1";

static constexpr int BUFFER_SIZE = (64) * (1024);
static char s_sendBuffer[BUFFER_SIZE];
static char s_recvBuffer[BUFFER_SIZE];
static int s_sendBufferSize = 0;
static int s_recvBufferSize = 0;

static SOCKET s_serverSocket = 0;
static DWORD s_lastConnectAttempt = 0;

static int s_logFileHandle = 0;

#define NET_LOG(x,...) LOG(s_logFileHandle, x, __VA_ARGS__) 
#define NET_ERROR(x, ...) NET_LOG(x, __VA_ARGS__)

//-------------------------------------------------------------------------------------------------
static void ConnectToServer()
{
    if (s_serverSocket)
    {
        NET_LOG("Tried to connect to server when already connected");
        return; // we're already connected
    }

    DWORD tickCount = GetTickCount();
    if (s_lastConnectAttempt && tickCount - s_lastConnectAttempt < 1000)
    {
        return; // only try once a second
    }
    s_lastConnectAttempt = tickCount;

    // Create the socket, and attempt to connect to the server
    s_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s_serverSocket == INVALID_SOCKET)
    {
        NET_ERROR("Failed to create socket (%d)", WSAGetLastError());
        return;
    }

    sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    server.sin_addr.s_addr = inet_addr(SERVER_ADDR);


    int error = connect(s_serverSocket, (struct sockaddr*)&server, sizeof(server));
    if (error == SOCKET_ERROR)
    {
        NET_ERROR("Failed to connect to socket (%d)", error);
        return;
    }
    // Change the socket mode on the listening socket from blocking to
    // non-block so the application will not block waiting for requests
    ULONG NonBlock = 1;
    error = ioctlsocket(s_serverSocket, FIONBIO, &NonBlock);
    if (error == SOCKET_ERROR)
    {
        NET_ERROR("Failed to set non-blocking on socket (%d)", error);
        return;
    }

    NET_LOG("Successfully established connection to %s:%d...", SERVER_ADDR, SERVER_PORT);
}
//-------------------------------------------------------------------------------------------------
bool Net_C_Init()
{
    s_logFileHandle = Log_Init("Net_Client");

    WSADATA wsd;
    if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0)
    {
        NET_ERROR("Failed to initialize windows socket library");
        return false;
    }

    ConnectToServer();

	return true;
}

//-------------------------------------------------------------------------------------------------
bool Net_C_Deinit()
{
    Log_Deinit(s_logFileHandle);

    if (s_serverSocket)
    {
        if (!closesocket(s_serverSocket))
        {
            NET_ERROR("Failed to close socket");
            return false;
        }
    }

    if (!WSACleanup())
    {
        NET_ERROR("Failed to cleanup windows socket library");
        return false;
    }
        
	return true;
}
//-------------------------------------------------------------------------------------------------
static bool Send(int inputMask)
{
    int idx = 0;

    InputPacket p;
    p.value = inputMask;
    memcpy(&s_sendBuffer[idx], &p, sizeof(p));
    idx += sizeof(p);

    s_sendBufferSize = idx;

    int ret = send(s_serverSocket, s_sendBuffer, s_sendBufferSize, 0);
    if (ret == SOCKET_ERROR)
    {
        int error = WSAGetLastError();
        if (error == WSAEWOULDBLOCK)
        {
            return true;
        }
        NET_ERROR("Failed to send(), (%d)", error);
        return false;
    }
    if (ret != s_sendBufferSize)
    {
        // error for now if we couldn't send the whole thing
        NET_ERROR("Didn't send all the packets");
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
        NET_ERROR("Can't recieve until we clear our buffer");
        return false;
    }

    int ret = recv(s_serverSocket, s_recvBuffer, BUFFER_SIZE, 0);
    if (ret == SOCKET_ERROR)
    {
        int error = WSAGetLastError();
        if (error == WSAEWOULDBLOCK)
        {
            return true;
        }
        NET_ERROR("Failed to recv(), (%d)", error);
        return false;
    }
    if (ret == 0)
    {
        return true; // nothing here, no big deal
    }

    if (ret == BUFFER_SIZE)
    {
        NET_ERROR("Recieved more packets than can fit in a single buffer");
        return false; // error case for now... have to handle multiple packets
    }


    // cache off new command frames and ack them
    //std::cout << " [Net] Recieved " << ret << " bytes" << std::endl;
    s_recvBufferSize = ret;
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
        // first character defines packet type
        Packet* p = (Packet*)(&s_recvBuffer[idx]);
        switch (p->GetType())
        {
            case WORLD_STATE_UPDATE_ID:
            {
                WorldStateUpdatePacket* w = (WorldStateUpdatePacket*)(p);
                NET_LOG("[WorldStateUpdate] t=%.2f", w->now);
                idx += sizeof(WorldStateUpdatePacket);
            }
            break;

            case HANDLE_WORLD_RESET_ID:
            {
                ResetCamera();
                idx += sizeof(HandleWorldStateResetPacket);
            }
            break;
            
            default:
            {
                NET_ERROR("Unknown packet (%d)", p->GetType());
                return false; // unknown packet
            }
        }
    }

    if (idx != s_recvBufferSize)
    {
        NET_ERROR("Got a partial packet or buffer error");
        return false; // somehow didn't get packets to line up
    }
    s_recvBufferSize = 0;
    return true;
}
//-------------------------------------------------------------------------------------------------
bool Net_C_Update(int inputMask)
{
    if (!s_serverSocket)
    {
        ConnectToServer();
    }

    if (!s_serverSocket)
    {
        return false;
    }

	// Send inputs to server
    if (!Send(inputMask))
    {
        NET_ERROR("Send failed");
        return false;
    }
        

	// check for new data from server
    if (!Recieve())
    {
        NET_ERROR("Recieve failed");
        return false;
    }
        

    // process any data that we recieved
    if (!Process())
    {
        NET_ERROR("Process failed");
        return false;
    }
        

	// notify world of error(? should this happen here?)
	return true;
}
