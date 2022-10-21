#pragma once


void World_S_HandleInputs(int inputMask);
void World_S_Update(double now);

void World_S_FillNewConnectionMessage(struct ClientNewConnection*);
void World_S_FillWorldUpdateMessage(struct ClientWorldStateUpdatePacket*,unsigned int lastAckedFrame);
