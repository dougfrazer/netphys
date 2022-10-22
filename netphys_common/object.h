#pragma once

#include "../netphys_common/common.h"

#include <ode/ode.h>

class Object
{
public:
	Object(const NPGUID& guid) : m_guid(guid) {}
	const NPGUID& GetGUID() const { return m_guid; }
	//void* operator new(size_t size) = delete; // TODO: get rid of the regular new, but placement new is ok... possible?
	const NPGUID m_guid;
// child functions
public:
	virtual dBodyID GetBodyID() const { return nullptr; }

public:
	Object* m_prev;
	Object* m_next;
};