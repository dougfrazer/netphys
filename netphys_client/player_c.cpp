#include "player_c.h"

#include "../netphys_common/world.h"

dBodyID ActivePlayer_C::m_bodyID = nullptr;
dGeomID ActivePlayer_C::m_geomID = nullptr;

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

void ActivePlayer_C::HandleInputs(int inputMask)
{
    // this is for standalone only
    //if (!m_bodyID)
    //{
    //    if (inputMask & INPUT_SPACE)
    //    {
    //        m_bodyID = World::Get()->CreateBody();
    //        m_geomID = World::Get()->CreateSphere(PLAYER_SIZE + .2l);  // make the physics sphere a little bigger
    //        Player_C* player = ObjectManager_C_CreateObject();
    //        dBodySetAutoDisableFlag(m_bodyID, 0);
    //        dBodySetPosition(m_bodyID, 0, 0, 5.f);
    //        dMass mass;
    //        dMassSetSphere(&mass, DENSITY, PLAYER_SIZE);
    //        dBodySetMass(m_bodyID, &mass);
    //        
    //        dGeomSetBody(m_geomID, m_bodyID);
    //    }
    //}
    //
   // HandleInputsInternal(m_bodyID, inputMask);
}