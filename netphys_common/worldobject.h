#pragma once

#include "../netphys_common/object.h"


class WorldObject : public Object
{
public:
	void Init(float x, float y, float z);

	WorldObject() : Object(GetNewGUID(ObjectType_WorldObject))
	{
		// todo: make sure we're in standalone
	}
	WorldObject(const NPGUID& guid) : Object(guid) 
	{
	}
public:
	virtual dBodyID GetBodyID() const override { return m_bodyID; }
	dBodyID m_bodyID = nullptr;
	dGeomID m_geomID = nullptr;
};