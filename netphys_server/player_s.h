#pragma once

#include "../netphys_common/common.h"
#include "../netphys_common/player.h"

#include "network_s.h"

class Player_S : public Player
{
public:
	Player_S() : Player(GetNewGUID(ObjectType_Player)) {}
public:
	void SetConnection(Connection* conn) { m_connection = conn; }
private:
	Connection* m_connection;

public:
	bool ProcessPacket(Packet* p);

private:
	void HandleInputs(int inputMask);

public:
	virtual dBodyID GetBodyID() const override { return m_bodyID; }
private:
	dBodyID m_bodyID = nullptr;
	dGeomID m_geomID = nullptr;
};