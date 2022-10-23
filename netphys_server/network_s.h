#pragma once

#include "../netphys_common/lib.h"
#include "../netphys_common/common.h"


#include <WinSock2.h>


bool Net_S_Init();
bool Net_S_Deinit();

bool Net_S_Update();

//-------------------------------------------------------------------------------------------------
// Connection structure
//-------------------------------------------------------------------------------------------------
enum CONNECTION_STATE
{
    CONNECTION_STATE_NONE,
    CONNECTION_STATE_NEW_CONNECTION,
    CONNECTION_STATE_OPEN,
};
static constexpr int DATA_BUFSIZE = (64) * (1024);
struct Connection
{
    Connection(class Player_S* owner, const struct sockaddr_in& addr);
    ~Connection();

    bool Write();
    bool Process();
    void Update();
    CONNECTION_STATE GetState() const { return m_state; }
    bool IsReady() const { return m_state == CONNECTION_STATE_OPEN; }
    bool MatchesAddress(const struct sockaddr_in& addr) const;
    void AddRecvBytes(char* data, int length);

    void Send(struct Packet* p);

private:
    class Player_S* m_owner;

    char m_recvBuffer[DATA_BUFSIZE];
    char m_sendBuffer[DATA_BUFSIZE];
    const struct sockaddr_in m_address;
    unsigned int m_bytesToSend = 0;
    unsigned int m_bytesRecvd = 0;
    bool m_flagForRemove = false;


    DWORD m_stateTime = 0;
    CONNECTION_STATE m_state = CONNECTION_STATE_NONE;
    void SendNewConnection();
    bool ProcessPacket(struct Packet* p);
    FrameNum m_lastAckedFrame = 0;
};

//bool Net_S_SendToAllClients(char* bytes, int numBytes);