#pragma once

#include "object.h"

//class ObjectMgr
//{
//	virtual Object* CreatePlayer() = 0;
//	virtual Object* CreateWorldObject() = 0;
//};

Object* ObjectMgr_CreateWorldObject();
Object* ObjectMgr_FreeObject(Object* obj);

