#pragma once

#include "../netphys_common/common.h"
#include "../netphys_common/player.h"

#include "network_s.h"

class Player_S : public Player
{
public:
	void SetConnection(const Connection* conn) { m_connection = conn; }
private:
	const Connection* m_connection;

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