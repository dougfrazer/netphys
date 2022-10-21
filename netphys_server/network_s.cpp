#include "network_s.h"

#include <Windows.h>

#include "../netphys_common/common.h"
#include "../netphys_common/world.h"
#include "../netphys_common/log.h"

#include "world_s.h"
#include "player_s.h"

#include <iostream>
#include <vector>

//-------------------------------------------------------------------------------------------------
// Constants
//-------------------------------------------------------------------------------------------------
static constexpr int LISTEN_PORT = 5555;
static const char* LISTEN_ADDR = "127.0.0.1";

static SOCKET s_listenSocket = INVALID_SOCKET;
static constexpr DWORD STATE_TIMEOUT = 2000;

//-------------------------------------------------------------------------------------------------
static std::vector<Connection*> s_connections;
//-------------------------------------------------------------------------------------------------
static void PushToReceiveBuffer(char* data, int length, const sockaddr_in& from)
{
    for (Connection* c : s_connections)
    {
        if (c->MatchesAddress(from))
        {
            c->AddRecvBytes(data, length);
            return;
        }
    }

    // if we get here we couldn't find a matching connection... so create one
    Player_S* newPlayer = new Player_S;
    Connection* newConn = new Connection(newPlayer, from);
    newConn->AddRecvBytes(data, length);
    newPlayer->SetConnection(newConn);
    s_connections.push_back(newConn);
    LOG_CONSOLE("New Connection: %s:%u", inet_ntoa(from.sin_addr), ntohs(from.sin_port));
}
//-------------------------------------------------------------------------------------------------
//static void ClearBadConnections()
//{
//    for (auto it = s_connections.begin(); it != s_connections.end();)
//    {
//        if (it->m_flagForRemove)
//        {
//            LOG("Removing connection %s:%u", inet_ntoa(it->address.sin_addr), ntohs(it->address.sin_port));
//            it = s_connections.erase(it);
//        }
//        else
//        {
//            ++it;
//        }
//    }
//}
//-------------------------------------------------------------------------------------------------
Connection::Connection(class Player_S* owner, const struct sockaddr_in& addr) 
    : m_owner(owner)
    , m_address(addr) 
{}
//-------------------------------------------------------------------------------------------------
Connection::~Connection() 
{ 
    delete m_owner; 
}
//-------------------------------------------------------------------------------------------------
void Connection::AddRecvBytes(char* data, int length)
{
    if (m_bytesRecvd + length > DATA_BUFSIZE)
    {
        LOG_ERROR("Not enough space in the buffer for packet of length %d", length);
    }
    else
    {
        memcpy(&m_recvBuffer[m_bytesRecvd], data, length);
        m_bytesRecvd = length;
    }
}
//-------------------------------------------------------------------------------------------------
bool Connection::MatchesAddress(const sockaddr_in& addr) const
{
    return m_address.sin_addr.S_un.S_addr == addr.sin_addr.S_un.S_addr &&
           m_address.sin_port == addr.sin_port;
}
//-------------------------------------------------------------------------------------------------
void Connection::SendNewConnection()
{
    if (m_bytesToSend != 0)
    {
        LOG_ERROR("Why do we have bytes to send on a new connection...");
    }
    if (m_state != CONNECTION_STATE_NONE && m_state != CONNECTION_STATE_NEW_CONNECTION)
    {
        LOG_ERROR("Why are we sending a new connection message to an open connection");
    }

    LOG("Sending client new connection message");
    ClientNewConnection msg;
    World_S_FillNewConnectionMessage(&msg);
    Send(&msg);
    m_stateTime = GetTickCount();
    m_state = CONNECTION_STATE_NEW_CONNECTION;
}
//-------------------------------------------------------------------------------------------------
void Connection::Send(Packet* msg)
{
    int bytesSerialized = msg->Serialize(&m_sendBuffer[m_bytesToSend], DATA_BUFSIZE - m_bytesToSend);
    if (!bytesSerialized)
    {
        LOG_ERROR("Failed to serialize message");
    }
    m_bytesToSend += bytesSerialized;
}
//-------------------------------------------------------------------------------------------------
void Connection::Update()
{
    if (m_state == CONNECTION_STATE_NONE || m_state == CONNECTION_STATE_NEW_CONNECTION)
    {
        if (!m_stateTime || GetTickCount() - m_stateTime > STATE_TIMEOUT)
        {
            SendNewConnection();
        }
    }
    
    // always send the world state updates even if we haven't acked the original, the client can filter them out
    ClientWorldStateUpdatePacket msg;
    World_S_FillWorldUpdateMessage(&msg, m_lastAckedFrame);
    Send(&msg);
}
//-------------------------------------------------------------------------------------------------
bool Connection::Write()
{
    if (!m_bytesToSend)
        return true;

    int ret = sendto(s_listenSocket, m_sendBuffer, m_bytesToSend, 0, (sockaddr*)&m_address, sizeof(sockaddr_in));
    if (ret == SOCKET_ERROR)
    {
        int error = WSAGetLastError();
        if (error != WSAEWOULDBLOCK)
        {
            LOG_ERROR("ERROR: Connection error during Write(): %d", error);
            return false;
        }
        return true; // this is fine, try again next frame
    }

    if (ret != m_bytesToSend)
    {
        LOG_ERROR("ERROR: Failed to send all the bytes in Write(), only sent %d of %d", ret, m_bytesToSend);
    }

    m_bytesToSend = 0;
    return true;
}
//-------------------------------------------------------------------------------------------------
bool Connection::ProcessPacket(Packet* p)
{
    switch (p->GetType())
    {
        case SERVER_NEW_CONNECTION_ID:
        {
            switch (m_state)
            {
                case CONNECTION_STATE_NONE:
                {
                    // expected state.  just created the connection
                    LOG_CONSOLE("Got new connection message to " F_GUID, VA_GUID(m_owner->GetGUID()));
                }
                break;

                case CONNECTION_STATE_NEW_CONNECTION:
                {
                    // valid state... previous message must have gotten dropped... just ignore we're on a timeout loop anyway
                    LOG_CONSOLE("Got another new connection message for player " F_GUID " while in STATE_NEW_CONNECTION", VA_GUID(m_owner->GetGUID()));
                }
                break;

                default:
                {
                    // error, shouldnt happen
                    LOG_ERROR("Got a new connection message for player " F_GUID " on a non - new connection, closing...", VA_GUID(m_owner->GetGUID()));
                    return false;
                }
                break;
            }
            return true;
        }
        break;

        case SERVER_NEW_CONNECTION_ACK_ID:
        {
            if (m_state != CONNECTION_STATE_NEW_CONNECTION)
            {
                LOG_ERROR("Got NewConnectionAck when not expecting it, state=%d", m_state);
            }
            else
            {
                ServerNewConnectionAck* msg = (ServerNewConnectionAck*)(p);
                FrameNum frameNum = msg->GetFrameNum();
                LOG_CONSOLE("New connection acked for player " F_GUID " at frame = %d", VA_GUID(m_owner->GetGUID()), frameNum);
                m_lastAckedFrame = frameNum;
                m_state = CONNECTION_STATE_OPEN;
            }
            return true;
        }
        break;

        case SERVER_WORLD_UPDATE_ACK_ID:
        {
            if (m_state != CONNECTION_STATE_OPEN)
            {
                LOG_ERROR("Got WorldUpdateAck when not expecting it");
            }

            ServerWorldUpdateAck* msg = (ServerWorldUpdateAck*)(p);
            FrameNum frameNum = msg->GetFrameNum();
            LOG("Player " F_GUID " acked update at frame=%d", VA_GUID(m_owner->GetGUID()), frameNum);
            m_lastAckedFrame = frameNum;
            return true;
        }
        break;
    }
    return false;
}
//-------------------------------------------------------------------------------------------------
bool Connection::Process()
{
    if(!m_bytesRecvd)
        return true;

    unsigned int idx = 0;
    while (idx < m_bytesRecvd)
    {
        Packet p;
        int size = p.Deserialize(&m_recvBuffer[idx]);    
        if (!ProcessPacket(&p))
        {
            if (m_state != CONNECTION_STATE_OPEN)
            {
                LOG_ERROR("Got a packet (%d) for player " F_GUID " before the connection was ready for it", p.GetType(), VA_GUID(m_owner->GetGUID()));
                // maybe return false?
            }

            if (!m_owner->ProcessPacket(&p))
            {
                // noone processed the packet, something went wrong
                m_bytesRecvd = 0;
                LOG("Failed to process packet %d for player " F_GUID " ...", p.GetType(), VA_GUID(m_owner->GetGUID()));
                return false;
            }
        }
        idx += size;
        p.data.Free();
    }

    if (idx != m_bytesRecvd)
    {
        LOG_ERROR("ERROR: Did not process all input bytes, %d bytes remaining", (m_bytesRecvd - idx));
        m_bytesRecvd = 0;
        return false; // error case for now... somehow got a partial packet?
    }
    
    m_bytesRecvd = 0;

    return true;
}
//-------------------------------------------------------------------------------------------------
bool Net_S_Init()
{
    WSADATA wsaData;
    int err = WSAStartup(0x0202, &wsaData);
    if (err != 0)
    {
        LOG_ERROR("Failed to set intialize windows sockets");
        WSACleanup();
        return false;
    }

    // Prepare a socket to listen for connections
    s_listenSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s_listenSocket == INVALID_SOCKET)
    {
        LOG_ERROR("Failed to set create listen socket");
        return false;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY; //inet_addr(LISTEN_ADDR);
    addr.sin_port = htons(LISTEN_PORT);
    if (bind(s_listenSocket, (sockaddr*)&addr, sizeof(sockaddr_in)) == SOCKET_ERROR)
    {
        LOG_ERROR("Failed to set bind() on listen socket - (%d)", err);
        return false;
    }

    // Change the socket mode on the listening socket from blocking to
    // non-block so the application will not block waiting for requests
    ULONG NonBlock = 1;
    if (ioctlsocket(s_listenSocket, FIONBIO, &NonBlock) == SOCKET_ERROR)
    {
        LOG_ERROR("Failed to set non-blocking on listen socket");
        return false;
    }

    sockaddr_in listenSockAddr;
    int size = sizeof(listenSockAddr);
    getsockname(s_listenSocket, (SOCKADDR*)&listenSockAddr, &size);
    LOG_CONSOLE("Initialized network, listening on %s:%d", inet_ntoa(listenSockAddr.sin_addr), htons(listenSockAddr.sin_port));

    return true;
}
//-------------------------------------------------------------------------------------------------
bool Net_S_Deinit()
{
    if (!closesocket(s_listenSocket))
    {
        LOG_ERROR("ERROR: failed to close listen socket - err (%d)", WSAGetLastError());
        return false;
    }
    s_listenSocket = INVALID_SOCKET;

    // TODO: anything to do on each connection?  send client a 'shutdown' message or something?
    s_connections.clear();

    if (!WSACleanup())
    {
        LOG_ERROR("ERROR: cleanup windows sockets (%d)", WSAGetLastError());
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
            LOG_ERROR("ERROR: Got error on recvfrom: (%d)", WSAGetLastError());
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
   // ClearBadConnections();

    //
    // Give each connection a chance to move its state along if its not open yet
    //
    for (Connection* c : s_connections)
    {
        c->Update();
    }


    if (!ReadSocket())
    {
        return false;
    }

    for (Connection* c : s_connections)
    {
        if (!c->Write())
        {
            return false;
        }
    }

    //
    // Write to all our sockets
    //
    for (Connection* c : s_connections)
    {
        if (!c->Write())
        {
            return false;
        }
    }

    //
    // Run update on all sockets
    //
    for (Connection* c : s_connections)
    {
        if (!c->Process())
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
//            LOG("Packets are getting backed up.. closing connection");
//            c.flagForRemove = true;
//            continue; // TODO: handle this case? 
//        }
//        memcpy(&c.sendBuffer[c.bytesToSend], bytes, numBytes);
//        c.bytesToSend += numBytes;
//    }
//    return true;
//}
