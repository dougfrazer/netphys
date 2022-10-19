#pragma once

bool Net_C_Init();
bool Net_C_Deinit();

bool Net_C_Update();

void Net_C_Send(char* bytes, int numBytes);
void Net_C_SendConnectionAck();