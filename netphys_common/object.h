#pragma once

#include "../netphys_common/common.h"

#include <ode/ode.h>

class Object
{
public:
	Object();
	~Object();
	const NPGUID& GetGUID() const { return m_guid; }
private:
	const NPGUID m_guid;
// child functions
public:
	virtual dBodyID GetBodyID() const { return nullptr; }

public:
	Object* m_prev;
	Object* m_next;
};

Object* GetFirstObject();
Object* GetNextObject(Object* obj);
Object* LookupObject(const NPGUID& guid);