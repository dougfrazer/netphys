#pragma once

#include "../netphys_common/object.h"

class Player_S* ObjectManager_S_CreatePlayer();
class WorldObject* ObjectManager_S_CreateWorldObject();
void    ObjectManager_S_FreePlayer(Player_S* player);
void    ObjectManager_S_FreeWorldObject(WorldObject* worldObject);
Object* ObjectManager_S_GetFirst();
Object* ObjectManager_S_GetNext(Object* obj);
Object* ObjectManager_S_LookupObject(const NPGUID& guid);
