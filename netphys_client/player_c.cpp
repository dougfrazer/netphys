#include "player_c.h"

#include "../netphys_common/world.h"

void Player_C::HandleInputs(int inputMask)
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
            m_geomID = World::Get()->CreateSphere(PLAYER_SIZE + .2f);  // make the physics sphere a little bigger
            dGeomSetBody(m_geomID, m_bodyID);
        }
    }

    HandleInputsInternal(m_bodyID, inputMask);
}