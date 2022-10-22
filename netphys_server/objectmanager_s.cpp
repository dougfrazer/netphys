#include "objectmanager_s.h"
#include "player_s.h"
#include "../netphys_common/worldobject.h"

static CyclicalList<Object> s_objectList;

static BlockAllocator<Player_S> s_playerBlockAllocator(128);
static BlockAllocator<WorldObject> s_worldObjectBlockAllocator(128);

Player_S* ObjectManager_S_CreatePlayer()
{
	Player_S* buf = s_playerBlockAllocator.Get();
	Player_S* player = new(buf) Player_S;
	s_objectList.Add(player);
	return player;
}
WorldObject* ObjectManager_S_CreateWorldObject()
{
	WorldObject* buf = s_worldObjectBlockAllocator.Get();
	WorldObject* worldObject = new(buf) WorldObject;
	s_objectList.Add(worldObject);
	return worldObject;
}
void ObjectManager_S_FreePlayer(Player_S* player)
{
	s_objectList.Remove(player);
	player->~Player_S();
	s_playerBlockAllocator.Free(player);
}
void ObjectManager_S_FreeWorldObject(WorldObject* worldObject)
{
	s_objectList.Remove(worldObject);
	worldObject->~WorldObject();
	s_worldObjectBlockAllocator.Free(worldObject);
}
Object* ObjectManager_S_GetFirst()
{
	return s_objectList.GetFirst();
}
Object* ObjectManager_S_GetNext(Object* obj)
{
	return s_objectList.GetNext(obj);
}
Object* ObjectManager_S_LookupObject(const NPGUID& guid)
{
	for (Object* obj = ObjectManager_S_GetFirst(); obj != nullptr; obj = ObjectManager_S_GetNext(obj))
	{
		if (obj->GetGUID() == guid)
		{
			return obj;
		}
	}
	return nullptr;
}