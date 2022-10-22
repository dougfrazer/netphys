#pragma once

#include "../netphys_common/object.h"

Object* ObjectManager_C_CreateObject(const NPGUID& guid);
//class Player_C* ObjectManager_C_CreatePlayer();
class WorldObject* ObjectManager_C_CreateWorldObject(const NPGUID& guid);
//void    ObjectManager_C_FreePlayer(Player_C* player);
void    ObjectManager_C_FreeWorldObject(WorldObject* worldObject);
Object* ObjectManager_C_GetFirst();
Object* ObjectManager_C_GetNext(Object* obj);
Object* ObjectManager_C_LookupObject(const NPGUID& guid);
