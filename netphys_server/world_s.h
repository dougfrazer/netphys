#pragma once


void World_S_HandleInputs(int inputMask);
void World_S_Update(double now);

void World_S_FillWorldState(struct ClientNewConnection* msg);
void World_S_FillWorldStateUpdate(struct ClientWorldStateUpdatePacket* msg, unsigned int lastAckedFrame);
