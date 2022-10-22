#pragma once

#include <ode/ode.h>
#include <vector>

#include "../netphys_common/worldobject.h"

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
	dGeomID CreateCube(float size);

	const std::vector<WorldObject*>& GetWorldObjects() const;
};
