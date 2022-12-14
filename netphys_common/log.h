#pragma once


void Log_Init(const char* applicationName);
void Log_Deinit();

int Log_InitSystem(const char* fileName);
void Log_DeinitSystem(int handle);

void Log_Write(const char* file, int lineNum, int handle, bool toConsole, bool error, const char* fmt, ...);

// Write to a specific log file
#define SYSTEM_LOG(h, str, ...)       Log_Write(__FILE__, __LINE__, h, false, false, str, __VA_ARGS__)
#define SYSTEM_LOG_WARNING(h,str,...) Log_Write(__FILE__, __LINE__, h, true,  false, str, __VA_ARGS__)
#define SYSTEM_LOG_ERROR(h,str,...)   Log_Write(__FILE__, __LINE__, h, true,  true,  str, __VA_ARGS__)

// Write to the generic log file
#define LOG(str, ...)        Log_Write(__FILE__, __LINE__, 0, false, false, str, __VA_ARGS__)
#define LOG_CONSOLE(str,...) Log_Write(__FILE__, __LINE__, 0, true,  false, str, __VA_ARGS__)  // maybe different than warning some day
#define LOG_WARNING(str,...) Log_Write(__FILE__, __LINE__, 0, true,  false, str, __VA_ARGS__)
#define LOG_ERROR(str,...)   Log_Write(__FILE__, __LINE__, 0, true,  true,  str, __VA_ARGS__)
