#pragma once

bool Net_C_Init();
bool Net_C_Deinit();

bool Net_C_Update(int inputMask);

bool Net_C_Send(int numBytes, char* bytes);