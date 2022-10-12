#pragma once


int Log_Init(const char* fileName);

void Log_Deinit(int handle);

void Log_Write(const char* file, int lineNum, int handle, const char* fmt, ...);

#define LOG(h, x, ...) Log_Write(__FILE__, __LINE__, h, x, __VA_ARGS__)