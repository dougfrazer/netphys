#pragma once

#include "../netphys_common/object.h"

// player constants
static constexpr float PLAYER_ACCEL = 20.0f; // for now same in all directions
static constexpr dReal PLAYER_SIZE = 2.f;

class Player : public Object
{
public:
	void HandleInputsInternal(dBodyID bodyID, int inputMask);
};