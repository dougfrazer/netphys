#pragma once

#include <ode/ode.h>

struct Object
{
	dBodyID m_bodyID;
	dGeomID m_geometry;
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

	void HandleInputs(int inputMask);

	void CreatePlayer();

	const Object& GetPlayer();
	const Object& GetInteract(int index);
};
