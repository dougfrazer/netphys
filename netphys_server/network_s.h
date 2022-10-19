#pragma once

#include "../netphys_common/lib.h"

bool Net_S_Init();
bool Net_S_Deinit();

bool Net_S_Update();

//bool Net_S_SendToAllClients(char* bytes, int numBytes);