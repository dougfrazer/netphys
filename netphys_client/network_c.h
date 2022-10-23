#pragma once

bool Net_C_Init();
bool Net_C_Deinit();

bool Net_C_Update(float dt);

void Net_C_Send(struct Packet* packet);
void Net_C_SendConnectionAck();

float Net_C_GetAverageReadBandwidth(float time);
float Net_C_GetAverageWriteBandwidth(float time);