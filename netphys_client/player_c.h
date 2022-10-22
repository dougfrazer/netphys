#pragma once

#include "../netphys_common/player.h"

class Player_C : public Player
{
public:
	Player_C(const NPGUID& guid) : Player(guid) {}
	void Init(float x, float y, float z);
public:
	virtual dBodyID GetBodyID() const override { return m_bodyID; }
	dBodyID m_bodyID;
	dGeomID m_geomID;
};

class ActivePlayer_C : public Player_C
{
public:
	ActivePlayer_C(const NPGUID& guid) : Player_C(guid) {}
public:
	static void HandleInputs(int inputMask);

	virtual dBodyID GetBodyID() const override { return m_bodyID; }
	static dBodyID m_bodyID;
	static dGeomID m_geomID;
};