#include "player_c.h"

#include "../netphys_common/world.h"

void Player_C::Init(float x, float y, float z)
{
    m_bodyID = World::Get()->CreateBody();
    m_geomID = World::Get()->CreateSphere(PLAYER_SIZE);  // make the physics sphere a little bigger
    dBodySetAutoDisableFlag(m_bodyID, 0);
    dBodySetPosition(m_bodyID,x, y, z);
    dMass mass;
    dMassSetSphere(&mass, DENSITY, PLAYER_SIZE);
    dBodySetMass(m_bodyID, &mass);
    
    dGeomSetBody(m_geomID, m_bodyID);
}
