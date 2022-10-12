#pragma once

bool Net_S_Init();
bool Net_S_Deinit();

bool Net_S_Update();

bool Net_S_SendToAllClients(char* data, int size);