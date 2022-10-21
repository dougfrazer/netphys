#pragma once

#include "../netphys_common/common.h"

FrameNum World_C_HandleNewConnection(ClientNewConnection* msg);
FrameNum World_C_HandleUpdate(ClientWorldStateUpdatePacket* msg);

void World_C_Init();
void World_C_Deinit();
void World_C_Update(float dt);