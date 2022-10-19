#pragma once

void World_C_HandleNewConnection(const struct ClientNewConnection* msg);
void World_C_HandleUpdate(const struct ClientWorldStateUpdatePacket* msg);

void World_C_Init();
void World_C_Deinit();
void World_C_Update(float dt);