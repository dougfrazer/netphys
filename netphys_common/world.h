#pragma once

#include <ode/ode.h>
#include <vector>

#include "../netphys_common/object.h"


struct WorldObject : public Object
{
	virtual dBodyID GetBodyID() const override { return m_bodyID; }
	dBodyID m_bodyID = nullptr;
	dGeomID m_geomID = nullptr;
};

class World
{
public:
	static World* Get();

	void Init();
	void Deinit();

	void Create();

	void Start();
	void Update(float dt);

	void Reset();

	dBodyID CreateBody();
	dGeomID CreateSphere(float radius);

	const std::vector<WorldObject*>& GetWorldObjects() const;
};
