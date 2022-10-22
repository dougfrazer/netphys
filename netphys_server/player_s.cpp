#include "player_s.h"

#include "../netphys_common/log.h"
#include "../netphys_common/world.h"
#include "network_s.h"

//-------------------------------------------------------------------------------------------------
bool Player_S::ProcessPacket(Packet* p)
{
    switch (p->GetType())
    {
        case SERVER_INPUT_PACKET_ID:
        {
            ServerInputPacket* msg = (ServerInputPacket*)(p);
            int inputMask = msg->GetMask();
            HandleInputs(inputMask);
            return true;
        }
        break;
    }
    return false; // didn't handle the packet
}
//-------------------------------------------------------------------------------------------------
void Player_S::HandleInputs(int inputMask)
{
    if (!m_bodyID)
    {
        if (inputMask & INPUT_SPACE)
        {
            m_bodyID = World::Get()->CreateBody();
            dBodySetAutoDisableFlag(m_bodyID, 0);
            dBodySetPosition(m_bodyID, 0, 0, 5.f);
            dMass mass;
            dMassSetSphere(&mass, DENSITY, PLAYER_SIZE);
            dBodySetMass(m_bodyID, &mass);
            m_geomID = World::Get()->CreateSphere(PLAYER_SIZE);
            dGeomSetBody(m_geomID, m_bodyID);
        }
    }
    HandleInputsInternal(m_bodyID, inputMask);
}