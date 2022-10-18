#include "network_s.h"

#include <WinSock2.h>
#include <Windows.h>

#include "../netphys_common/common.h"
#include "../netphys_common/world.h"
#include "../netphys_common/log.h"

#include "world_s.h"

#include <iostream>
#include <vector>

//-------------------------------------------------------------------------------------------------
// Constants
//-------------------------------------------------------------------------------------------------
static constexpr int DATA_BUFSIZE = (64) * (1024);
static constexpr int LISTEN_PORT = 5555;
static const char* LISTEN_ADDR = "127.0.0.1";

static int s_logFileHandle = 0;

#define NET_LOG(x,...) LOG(s_logFileHandle, x, __VA_ARGS__) 
#define NET_ERROR(x, ...) NET_LOG(x, __VA_ARGS__)

static SOCKET s_listenSocket = INVALID_SOCKET;

//-------------------------------------------------------------------------------------------------
// Connection structure
//-------------------------------------------------------------------------------------------------
struct Connection
{
    CHAR recvBuffer[DATA_BUFSIZE];
    CHAR sendBuffer[DATA_BUFSIZE];
    sockaddr_in address;
    DWORD bytesToSend;
    DWORD bytesRecvd;
    bool flagForRemove = false;

    bool Write();
    bool Process();
};
//-------------------------------------------------------------------------------------------------
static std::vector<Connection> s_connections;
//-------------------------------------------------------------------------------------------------
static void PushToReceiveBuffer(char* data, int length, const sockaddr_in& from)
{
    for (Connection& c : s_connections)
    {
        if (c.address.sin_addr.S_un.S_addr == from.sin_addr.S_un.S_addr && 
            c.address.sin_port == from.sin_port)
        {
            if (c.bytesRecvd + length > DATA_BUFSIZE)
            {
                NET_ERROR("Not enough space in the buffer for packet of length %d", length);
            }
            else
            {
                memcpy(&c.recvBuffer[c.bytesRecvd], data, length);
                c.bytesRecvd = length;
            }
            return;
        }
    }

    // if we get here we couldn't find a matching connection... so create one
    Connection* newConn = new Connection;
    newConn->address = from;
    memcpy(&newConn->recvBuffer, data, length);
    newConn->bytesRecvd = length;
    newConn->bytesToSend = 0;
    s_connections.push_back(std::move(*newConn));
    NET_LOG("New Connection: %s:%u", inet_ntoa(from.sin_addr), ntohs(from.sin_port));
}
//-------------------------------------------------------------------------------------------------
static void ClearBadConnections()
{
    for (auto it = s_connections.begin(); it != s_connections.end();)
    {
        if (it->flagForRemove)
        {
            NET_LOG("Removing connection %s:%u", inet_ntoa(it->address.sin_addr), ntohs(it->address.sin_port));
            it = s_connections.erase(it);
        }
        else
        {
            ++it;
        }
    }
}
//-------------------------------------------------------------------------------------------------
bool Connection::Write()
{
    if (!bytesToSend)
        return true;

    int ret = sendto(s_listenSocket, sendBuffer, bytesToSend, 0, (sockaddr*) & address, sizeof(sockaddr_in));
    if (ret == SOCKET_ERROR)
    {
        int error = WSAGetLastError();
        if (error != WSAEWOULDBLOCK)
        {
            NET_ERROR("ERROR: Connection error during Write(): %d", error);
            return false;
        }
        return true; // this is fine, try again next frame
    }

    if (ret != bytesToSend)
    {
        NET_ERROR("ERROR: Failed to send all the bytes in Write(), only sent %d of %d", ret, bytesToSend);
    }

    bytesToSend = 0;
    return true;
}
//-------------------------------------------------------------------------------------------------
bool Connection::Process()
{
    if(!bytesRecvd)
        return true;

    unsigned int idx = 0;
    while (idx < bytesRecvd)
    {
        Packet* p = (Packet*)(&recvBuffer[idx]);
        switch (p->GetType())
        {
            case INPUT_PACKET_ID:
            {
                InputPacket* i = (InputPacket*)(p);
                World_S_HandleInput(i->value);
                idx += sizeof(InputPacket);
            }
            break;
            default:
            {
                NET_ERROR("ERROR: Recieved unknown packet type (%d)", p->GetType());
                bytesRecvd = 0;
                return false; // unknown packet
            }
        }
    }

    if (idx != bytesRecvd)
    {
        NET_ERROR("ERROR: Did not process all input bytes, %d bytes remaining", (bytesRecvd - idx));
        bytesRecvd = 0;
        return false; // error case for now... somehow got a partial packet?
    }
    
    bytesRecvd = 0;

    return true;
}
//-------------------------------------------------------------------------------------------------
bool Net_S_Init()
{
    s_logFileHandle = Log_Init("Net_Server");

    WSADATA wsaData;
    int err = WSAStartup(0x0202, &wsaData);
    if (err != 0)
    {
        NET_ERROR("Failed to set intialize windows sockets");
        WSACleanup();
        return false;
    }

    // Prepare a socket to listen for connections
    s_listenSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s_listenSocket == INVALID_SOCKET)
    {
        NET_ERROR("Failed to set create listen socket");
        return false;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY; //inet_addr(LISTEN_ADDR);
    addr.sin_port = htons(LISTEN_PORT);
    if (bind(s_listenSocket, (sockaddr*)&addr, sizeof(sockaddr_in)) == SOCKET_ERROR)
    {
        NET_ERROR("Failed to set bind() on listen socket - (%d)", err);
        return false;
    }

    // Change the socket mode on the listening socket from blocking to
    // non-block so the application will not block waiting for requests
    ULONG NonBlock = 1;
    if (ioctlsocket(s_listenSocket, FIONBIO, &NonBlock) == SOCKET_ERROR)
    {
        NET_ERROR("Failed to set non-blocking on listen socket");
        return false;
    }

    sockaddr_in listenSockAddr;
    int size = sizeof(listenSockAddr);
    getsockname(s_listenSocket, (SOCKADDR*)&listenSockAddr, &size);
    NET_LOG("Initialized network, listening on %s:%d", inet_ntoa(listenSockAddr.sin_addr), htons(listenSockAddr.sin_port));

    return true;
}
//-------------------------------------------------------------------------------------------------
bool Net_S_Deinit()
{
    if (!closesocket(s_listenSocket))
    {
        NET_ERROR("ERROR: failed to close listen socket - err (%d)", WSAGetLastError());
        return false;
    }
    s_listenSocket = INVALID_SOCKET;

    // TODO: anything to do on each connection?  send client a 'shutdown' message or something?
    s_connections.clear();

    if (!WSACleanup())
    {
        NET_ERROR("ERROR: cleanup windows sockets (%d)", WSAGetLastError());
        return false;
    }
        
    return true;
}
//-------------------------------------------------------------------------------------------------
static char s_recvBuffer[DATA_BUFSIZE];
static bool ReadSocket()
{
    while (true)
    {
        sockaddr_in from;
        int fromSize = sizeof(sockaddr_in);
        int recvLength = recvfrom(s_listenSocket, s_recvBuffer, DATA_BUFSIZE, 0, (sockaddr*)&from, &fromSize);
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
            NET_ERROR("ERROR: Got error on recvfrom: (%d)", WSAGetLastError());
            return false;
        }
        else
        {
            PushToReceiveBuffer(s_recvBuffer, recvLength, from);
        }
    }

    return true;
}
//-------------------------------------------------------------------------------------------------
bool Net_S_Update()
{
    ClearBadConnections();

    if (!ReadSocket())
    {
        return false;
    }

    //
    // Write to all our sockets
    //
    for (Connection& c : s_connections)
    {
        if (!c.Write())
        {
            return false;
        }
    }

    //
    // Run update on all sockets
    //
    for (Connection& c : s_connections)
    {
        if (!c.Process())
        {
            return false;
        }
    }

    return true;
}
//-------------------------------------------------------------------------------------------------
bool Net_S_SendToAllClients(char* data, int size)
{
    for (Connection& c : s_connections)
    {
        // check to make sure there is space in the buffer?
        if (c.bytesToSend + size >= DATA_BUFSIZE)
        {
            NET_LOG("Packets are getting backed up.. closing connection");
            c.flagForRemove = true;
            continue; // TODO: handle this case? 
        }
        memcpy(&c.sendBuffer[c.bytesToSend], data, size);
        c.bytesToSend += size;
    }
    return true;
}
