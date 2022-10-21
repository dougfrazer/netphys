#pragma once

#include "../netphys_common/player.h"

class Player_C : public Player
{
public:
	void HandleInputs(int inputMask);

	virtual dBodyID GetBodyID() const override { return m_bodyID; }
	dBodyID m_bodyID;
	dGeomID m_geomID;
};