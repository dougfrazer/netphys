#include "objectmanager_c.h"
#include "player_c.h"
#include "../netphys_common/worldobject.h"

static CyclicalList<Object> s_objectList;

static BlockAllocator<Player_C> s_playerBlockAllocator = BlockAllocator<Player_C>(128);
static BlockAllocator<WorldObject> s_worldObjectBlockAllocator = BlockAllocator<WorldObject>(128);


Player_C* ObjectManager_C_CreatePlayer(const NPGUID& guid)
{
	Player_C* obj = s_playerBlockAllocator.Get();
	Player_C* player = new(obj) Player_C(guid);
	s_objectList.Add(player);
	return player;
}
WorldObject* ObjectManager_C_CreateWorldObject(const NPGUID& guid)
{
	WorldObject* obj = s_worldObjectBlockAllocator.Get();
	WorldObject* worldObject = new(obj) WorldObject(guid);
	s_objectList.Add(worldObject);
	return worldObject;
}

Object* ObjectManager_C_CreateObject(const NPGUID& guid)
{
	switch (guid.GetType())
	{
		case ObjectType_Player: return ObjectManager_C_CreatePlayer(guid);
		case ObjectType_WorldObject: return ObjectManager_C_CreateWorldObject(guid);
		default: break;
	}

	return nullptr;
}

void ObjectManager_C_FreeWorldObject(WorldObject* obj)
{
	s_objectList.Remove(obj);
	obj->~WorldObject();
	s_worldObjectBlockAllocator.Free(obj);
}
Object* ObjectManager_C_GetFirst()
{
	return s_objectList.GetFirst();
}
Object* ObjectManager_C_GetNext(Object* obj)
{
	return s_objectList.GetNext(obj);
}
Object* ObjectManager_C_LookupObject(const NPGUID& guid)
{
	for (Object* obj = ObjectManager_C_GetFirst(); obj != nullptr; obj = ObjectManager_C_GetNext(obj))
	{
		if (obj->GetGUID() == guid)
		{
			return obj;
		}
	}
	return nullptr;
}