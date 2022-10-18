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
static FILE* s_consoleLog = nullptr;
static HWND s_consoleWindow;

static int s_logIdx = 1;
static char buf[1024];
//-------------------------------------------------------------------------------------------------
int Log_Init(const char* fileName)
{
	char exeFileName[MAX_PATH] = { 0 };
	GetModuleFileNameA(NULL, exeFileName, MAX_PATH);
	std::string f(exeFileName);
	std::string exePath = f.substr(0, f.find_last_of("\\/"));
	std::string logFilePath = exePath + "/../Logs/";
	CreateDirectoryA(logFilePath.c_str(), NULL);

	time_t t = time(0);
	struct tm* now = localtime(&t);
	snprintf(buf, 1024, "%s/%s_%02d%02d%d_%02d-%02d-%02d.txt", logFilePath.c_str(), fileName, now->tm_mon + 1, now->tm_mday, 1900 + now->tm_year, now->tm_hour, now->tm_min, now->tm_sec);
	FILE* file = fopen(buf, "w");
	int index = s_logIdx++;
	s_logs.push_back({ index, file });

	//freopen("CONOUT$", "wt", file);

	//s_consoleLog = fopen("CONOUT$", "wt");
	
	// should we also redirect stdout/stderr?  
	// dont think this works with multiple processes on same machine
	//freopen(buf, "wt", stdout);
	//freopen(buf, "wt", stderr);

	if (AllocConsole())
	{
		char title[1024];
		sprintf(title, "Console:%d/%d", GetTickCount(), GetCurrentProcessId());
		SetConsoleTitleA(title);
		Sleep(40);			// ensure window title has been updated
		s_consoleWindow = FindWindowA(NULL, title);
		SetConsoleTitleA("Console");

		s_consoleLog = fopen("CONOUT$", "w+");
	}



	return index;
}
//-------------------------------------------------------------------------------------------------
void Log_Deinit(int handle)
{
	//fclose(s_consoleLog);

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

	std::string s(msg);
	s.append("\n");
	//fwrite(msg, sizeof(char), s.size(), s_consoleLog);
	//std::cout << msg << std::endl;
	if (!WriteConsoleA(s_consoleLog, s.c_str(), s.size(), 0, NULL))
	{
		LPVOID lpMsgBuf;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0, NULL);
		char buf[1024];
		snprintf(buf, 1024, "%s LastError=%d(%s)\n", msg, GetLastError(), lpMsgBuf);

		int x = 5;
	}

	snprintf(buf, 1024, "[%20s:%d] %s\n", base_filename.c_str(), lineNum, msg);
	fputs(buf, filePtr);
}