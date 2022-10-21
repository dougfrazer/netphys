#include "object.h"

#include <vector>

#include "../netphys_common/lib.h"

static LIST<Object> s_objectList;

Object::Object()
	: m_guid(GetNewGUID())
{
	s_objectList.Add(this);
}

Object::~Object()
{
	s_objectList.Remove(this);
}

Object* GetFirstObject()
{
	return s_objectList.GetFirst();
}

Object* GetNextObject(Object* obj)
{
	return s_objectList.GetNext(obj);
}

Object* LookupObject(const NPGUID& guid)
{
	for (Object* obj = GetFirstObject(); obj != nullptr; obj = GetNextObject(obj))
	{
		if (obj->GetGUID() == guid)
		{
			return obj;
		}
	}
	return nullptr;
}