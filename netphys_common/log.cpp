#include "log.h"

#include <cstdarg>
#include <iostream>
#include <vector>
#include <string>
#include <Windows.h>

struct LogHandle
{
	int identifier;
	FILE* file;
};
static std::vector<LogHandle> s_logs;

static int s_logIdx = 1;
static char buf[1024];
//-------------------------------------------------------------------------------------------------
int Log_Init(const char* fileName)
{
	char exeFileName[MAX_PATH] = { 0 };
	GetModuleFileNameA(NULL, exeFileName, MAX_PATH);
	std::string f(exeFileName);
	std::string exePath = f.substr(0, f.find_last_of("\\/"));
	std::string logFilePath = exePath + "/Logs";
	CreateDirectoryA(logFilePath.c_str(), NULL);

	time_t t = time(0);
	struct tm* now = localtime(&t);
	snprintf(buf, 1024, "%s/%s_%02d%02d%d_%02d-%02d-%02d.txt", logFilePath.c_str(), fileName, now->tm_mon + 1, now->tm_mday, 1900 + now->tm_year, now->tm_hour, now->tm_min, now->tm_sec);
	FILE* file = fopen(buf, "w");
	int index = s_logIdx++;
	s_logs.push_back({ index, file });
	return index;
}
//-------------------------------------------------------------------------------------------------
void Log_Deinit(int handle)
{
	for(auto it = s_logs.begin(); it != s_logs.end();)
	{
		if (it->identifier == handle)
		{
			fclose((FILE*)(it->file));
			it = s_logs.erase(it);
		}
		else
		{
			++it;
		}
	}
}
//-------------------------------------------------------------------------------------------------
void Log_Write(const char* path, int lineNum, int handle, const char* fmt, ...)
{
	FILE* filePtr = nullptr;
	for (auto& log : s_logs)
	{
		if (log.identifier == handle)
		{
			filePtr = log.file;
			break;
		}
	}
	if(!filePtr)
		return;

	std::string path_string(path);
	std::string base_filename = path_string.substr(path_string.find_last_of("/\\") + 1);

	char msg[1024];
	va_list args;
	va_start(args, fmt);
	vsprintf_s(msg, fmt, args);
	va_end(args);

	std::cout << msg << std::endl;

	snprintf(buf, 1024, "[%20s:%d] %s\n", base_filename.c_str(), lineNum, msg);
	fputs(buf, filePtr);
}