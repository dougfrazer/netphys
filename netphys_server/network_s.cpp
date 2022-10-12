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
    SOCKET socket;
    DWORD bytesToSend;
    DWORD bytesRecvd;
    bool flagForRemove = false;

    bool Read();
    bool Write();
    bool Process();
};
//-------------------------------------------------------------------------------------------------
static std::vector<Connection> s_connections;
//-------------------------------------------------------------------------------------------------
static bool AddConnection(SOCKET socket)
{
    // Prepare SocketInfo structure for use
    Connection* newConn = new Connection;
    newConn->socket = socket;
    newConn->bytesRecvd = 0;
    newConn->bytesToSend = 0;
    s_connections.push_back(std::move(*newConn));

    NET_LOG("Got a new connection %d!", socket);
    return true;
}
//-------------------------------------------------------------------------------------------------
static void ClearBadConnections()
{
    for (auto it = s_connections.begin(); it != s_connections.end();)
    {
        if (it->flagForRemove)
        {
            NET_LOG("Removing connection %d", it->socket);
            closesocket(it->socket);
            it = s_connections.erase(it);
        }
        else
        {
            ++it;
        }
    }
}
//-------------------------------------------------------------------------------------------------
bool Connection::Read()
{
    WSABUF buf;
    buf.buf = recvBuffer;
    buf.len = DATA_BUFSIZE;
    DWORD BytesRecieved = 0;
    DWORD Flags = 0;
    int ret = WSARecv(socket, &buf, 1, &BytesRecieved, &Flags, NULL, NULL);
    if (ret == SOCKET_ERROR)
    {
        int error = WSAGetLastError();
        if (error != WSAEWOULDBLOCK)
        {
            // error
            NET_ERROR("ERROR: Connection error during Write(): (%d)", error);
            return false;
        }
    }
    else if (BytesRecieved == 0)
    {
        // remote end closed the connection.. 
        NET_LOG("Remote connection closed for %d", socket);
        flagForRemove = true;
        return true; // hmm this might be dangerous we deleted ourselves
    }
    bytesRecvd = BytesRecieved;
    return true;
}
//-------------------------------------------------------------------------------------------------
bool Connection::Write()
{
    if (!bytesToSend)
        return true;

    DWORD SendBytes = 0;
    WSABUF buf;
    buf.buf = sendBuffer;
    buf.len = bytesToSend;
    int ret = WSASend(socket, &buf, 1, &SendBytes, 0, NULL, NULL);
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
    else if (ret == 0)
    {
        // successfully sent SendBytes
        bytesToSend = 0;
        return true;
    }

    NET_ERROR("ERROR: Unknown error in Write(): %d", ret);
    return false;
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
    WSADATA wsaData;
    int err = WSAStartup(0x0202, &wsaData);
    if (err != 0)
    {
        NET_ERROR("Failed to set intialize windows sockets");
        WSACleanup();
        return false;
    }

    // Prepare a socket to listen for connections
    s_listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (s_listenSocket == INVALID_SOCKET)
    {
        NET_ERROR("Failed to set create listen socket");
        return false;
    }

    SOCKADDR_IN InternetAddr;
    InternetAddr.sin_family = AF_INET;
    InternetAddr.sin_addr.s_addr = inet_addr(LISTEN_ADDR);
    InternetAddr.sin_port = htons(LISTEN_PORT);
    if (bind(s_listenSocket, (PSOCKADDR)&InternetAddr, sizeof(InternetAddr)) == SOCKET_ERROR)
    {
        NET_ERROR("Failed to set bind() on listen socket - (%d)", err);
        return false;
    }

    err = listen(s_listenSocket, 5);
    if (err != 0)
    {
        NET_ERROR("Failed to set listen() on listen socket- (%d)", err);
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

    s_logFileHandle = Log_Init("Net_Server");

    NET_LOG("Initialized network, listening on %s:%d", LISTEN_ADDR, LISTEN_PORT);

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

    for (Connection& c : s_connections)
    {
        if (!closesocket(c.socket))
        {
            NET_ERROR("ERROR: failed to close client socket (%d) - err (%d)", c.socket, WSAGetLastError());
            return false;
        }
    }
    s_connections.clear();
    if (!WSACleanup())
    {
        NET_ERROR("ERROR: cleanup windows sockets (%d)", WSAGetLastError());
        return false;
    }
        
    return true;
}
//-------------------------------------------------------------------------------------------------
static bool LookForNewConnections()
{
    // Look for new connections
    SOCKET newConnection = accept(s_listenSocket, NULL, NULL);
    if (newConnection != INVALID_SOCKET)
    {
        ULONG NonBlock = 1;
        int err = ioctlsocket(newConnection, FIONBIO, &NonBlock);
        if (err == SOCKET_ERROR)
        {
            NET_ERROR("ERROR: failed to set connection to non-blocking after accept (%d)", err);
            return false;
        }

        if (!AddConnection(newConnection))
        {
            NET_ERROR("ERROR: failed to add new connection");
            return false;
        }
    }
    else
    {
        if (WSAGetLastError() != WSAEWOULDBLOCK)
        {
            // error with accept
            NET_ERROR("ERROR: Got error on accept: (%d)", WSAGetLastError());
            return false;
        }
    }
    return true;
}
//-------------------------------------------------------------------------------------------------
bool Net_S_Update()
{
    ClearBadConnections();

    if (!LookForNewConnections())
    {
        return false;
    }

    //
    // Read from all our sockets
    //
    for (Connection& c : s_connections)
    {
        if (!c.Read())
        {
            return false;
        }
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
