#include "worldobject.h"

#include <ode/ode.h>

#include "../netphys_common/world.h"

void WorldObject::Init(float x, float y, float z)
{
	dBodyID bodyID = World::Get()->CreateBody();
	dGeomID geomID = World::Get()->CreateCube(BOX_SIZE);
	m_bodyID = bodyID;
	dBodySetPosition(m_bodyID, x, y, z);

	dMass mass;
	dMassSetBox(&mass, DENSITY, BOX_SIZE, BOX_SIZE, BOX_SIZE);
	m_geomID = geomID;

	dGeomSetBody(m_geomID, m_bodyID);
	dBodySetMass(m_bodyID, &mass);
}