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

#define NET_LOG(x,...)          SYSTEM_LOG(s_logFileHandle, x, __VA_ARGS__) 
#define NET_LOG_CONSOLE(x,...)  SYSTEM_LOG_WARNING(s_logFileHandle, x, __VA_ARGS__) 
#define NET_WARNING(x,...)      SYSTEM_LOG_WARNING(s_logFileHandle, x, __VA_ARGS__) 
#define NET_ERROR(x, ...)       SYSTEM_LOG_ERROR(s_logFileHandle, x, __VA_ARGS__)

static SOCKET s_listenSocket = INVALID_SOCKET;
static constexpr DWORD STATE_TIMEOUT = 2000;
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
    void Update();
    bool IsReady() const { return state == STATE_OPEN; }

private:
    enum STATE
    {
        STATE_NONE,
        STATE_NEW_CONNECTION,
        STATE_OPEN,
    };
    DWORD stateTime = 0;
    STATE state = STATE_NONE;
    void SendNewConnection();
    FrameNum lastAckedFrame = 0;
    void Send(char* data, int size);
    bool ProcessPacket(Packet* p, unsigned int& len);
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
void Connection::SendNewConnection()
{
    if (bytesToSend != 0)
    {
        NET_ERROR("Why do we have bytes to send on a new connection...");
    }
    if (state != STATE_NONE && state != STATE_NEW_CONNECTION)
    {
        NET_ERROR("Why are we sending a new connection message to an open connection");
    }


    ClientNewConnection msg;
    World_S_FillWorldState(&msg);
    memcpy(&sendBuffer[bytesToSend], &msg, sizeof(ClientNewConnection));
    bytesToSend += sizeof(ClientNewConnection);
    stateTime = GetTickCount();
    state = STATE_NEW_CONNECTION;

    NET_LOG("Sending ClientNewConnection to %d with frame=%d", socket, msg.frame.id);
}
//-------------------------------------------------------------------------------------------------
void Connection::Send(char* data, int size)
{
    // check to make sure there is space in the buffer?
    if (bytesToSend + size >= DATA_BUFSIZE)
    {
        NET_ERROR("Packets are getting backed up.. closing connection");
        flagForRemove = true;
        return; // TODO: handle this case? 
    }
    memcpy(&sendBuffer[bytesToSend], data, size);
    bytesToSend += size;
}
//-------------------------------------------------------------------------------------------------
void Connection::Update()
{
    if (state == STATE_NONE || state == STATE_NEW_CONNECTION)
    {
        if (!stateTime || GetTickCount() - stateTime > STATE_TIMEOUT)
        {
            SendNewConnection();
        }
    }
    
    // always send the world state updates even if we haven't acked the original, the client can filter them out
    ClientWorldStateUpdatePacket msg;
    World_S_FillWorldStateUpdate(&msg, lastAckedFrame);
    LOG("Sending WorldStateUpdate for frame=%d", msg.frame.id);
    Send((char*)&msg, sizeof(ClientWorldStateUpdatePacket));
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
bool Connection::ProcessPacket(Packet* p, unsigned int& len)
{
    switch (p->GetType())
    {
        case SERVER_NEW_CONNECTION_ID:
        {
            switch (state)
            {
                case STATE_NONE:
                {
                    // expected state.  just created the connection
                    NET_LOG_CONSOLE("Got new connection message to %d", socket);
                }
                break;

                case STATE_NEW_CONNECTION:
                {
                    // valid state... previous message must have gotten dropped... just ignore we're on a timeout loop anyway
                    NET_LOG_CONSOLE("Got another new connection message for connection %d while in STATE_NEW_CONNECTION", socket);
                }
                break;

                default:
                {
                    // error, shouldnt happen
                    NET_ERROR("Got a new connection message for connection %d on a non-new connection, closing...", socket);
                    flagForRemove = true;
                }
                break;
            }

            len += sizeof(ServerNewConnection);
        }
        break;

        case SERVER_NEW_CONNECTION_ACK_ID:
        {
            if (state != STATE_NEW_CONNECTION)
            {
                NET_ERROR("Got NewConnectionAck when not expecting it");
            }
            else
            {
                ServerNewConnectionAck* msg = (ServerNewConnectionAck*)(p);
                NET_LOG_CONSOLE("New connection acked for connection %d at frame=%d", socket, msg->frameNum);
                lastAckedFrame = msg->frameNum;
                state = STATE_OPEN;
            }
            len += sizeof(ServerNewConnectionAck);
        }
        break;

        case SERVER_WORLD_UPDATE_ACK_ID:
        {
            if (state != STATE_OPEN)
            {
                NET_ERROR("Got NewConnectionAck when not expecting it");
            }

            ServerWorldUpdateAck* msg = (ServerWorldUpdateAck*)(p);
            NET_LOG("Connection %d acked update at frame=%d", socket, msg->frameNum);
            lastAckedFrame = msg->frameNum;
            len += sizeof(ServerNewConnectionAck);
        }
        break;

        case SERVER_INPUT_PACKET_ID:
        {
            if (state != STATE_OPEN)
            {
                NET_ERROR("Got an input packet for connection %d in invalid state (%d)", socket, state);
            }
            else
            {
                ServerInputPacket* msg = (ServerInputPacket*)(p);
                World_S_HandleInputs(msg->value);
            }
            len += sizeof(ServerInputPacket);
        }
        break;



        default:
        {
            NET_ERROR("ERROR: Recieved unknown packet type (%d)", p->GetType());
            return false; // unknown packet
        }
    }
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
        if (!ProcessPacket(p, idx))
        {
            bytesRecvd = 0;
            return false;
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
    s_logFileHandle = Log_InitSystem("Net_Server");

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
    NET_LOG_CONSOLE("Initialized network, listening on %s:%d", inet_ntoa(listenSockAddr.sin_addr), htons(listenSockAddr.sin_port));

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
    Log_DeinitSystem(s_logFileHandle);
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

    //
    // Give each connection a chance to move its state along if its not open yet
    //
    for (Connection& c : s_connections)
    {
        c.Update();
    }


    if (!ReadSocket())
    {
        return false;
    }

    for (Connection& c : s_connections)
    {
        if (!c.Write())
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
//bool Net_S_SendToAllClients(char* bytes, int numBytes)
//{
//    for (Connection& c : s_connections)
//    {
//        if(!c.IsReady())
//            continue;
//
//        // check to make sure there is space in the buffer?
//        if (c.bytesToSend + numBytes >= DATA_BUFSIZE)
//        {
//            NET_LOG("Packets are getting backed up.. closing connection");
//            c.flagForRemove = true;
//            continue; // TODO: handle this case? 
//        }
//        memcpy(&c.sendBuffer[c.bytesToSend], bytes, numBytes);
//        c.bytesToSend += numBytes;
//    }
//    return true;
//}
