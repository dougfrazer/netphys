#include "log.h"

#include <cstdarg>
#include <iostream>
#include <vector>
#include <string>
#include <Windows.h>
#include <chrono>

struct LogHandle
{
	int identifier;
	FILE* file;
};
static std::vector<LogHandle> s_logs;
static HANDLE s_consoleHandle;

static int s_logIdx = 1;
static char buf[1024];

//-------------------------------------------------------------------------------------------------

void GetLastErrorString(char* buf, int bufSize)
{
	LPSTR messageBuffer = nullptr;
	DWORD error = GetLastError();
	size_t size = FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		error,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&messageBuffer,
		0, NULL);

	snprintf(buf, bufSize, "LastError=%d(%s)\n", error, messageBuffer);
	LocalFree(messageBuffer);
}
//-------------------------------------------------------------------------------------------------
static int s_logHandle = 0;
static FILE* s_genericLogFile = nullptr;
static std::chrono::steady_clock::time_point s_startTime;
void Log_Init()
{
	s_logHandle = Log_InitSystem("GenericLog");
	if (s_logs.size() > 1)
	{
		// bad...
		__debugbreak();
	}
	s_genericLogFile = s_logs[0].file;
	s_startTime = std::chrono::high_resolution_clock::now();
}
//-------------------------------------------------------------------------------------------------
void Log_Deinit()
{
	Log_DeinitSystem(s_logHandle);
	s_logHandle = 0;
	s_genericLogFile = nullptr;
}
//-------------------------------------------------------------------------------------------------
int Log_InitSystem(const char* fileName)
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

	int err = AllocConsole();
	if (err)
	{
		if (GetLastError() == ERROR_ACCESS_DENIED)
		{
			err = AttachConsole(GetCurrentProcessId());
		}
	}
	if(err)
	{
		char errMsg[1024];
		GetLastErrorString(errMsg, 1024);
		snprintf(buf, 1024, "Failed to create console: %s", errMsg);
		fputs(buf, file);
	}
	else
	{
		// this is the recommended way to get a handle to the console window if we need it...
		//char title[1024];
		//sprintf(title, "Console:%d/%d", GetTickCount(), GetCurrentProcessId());
		//SetConsoleTitleA(title);
		//Sleep(40);			// ensure window title has been updated
		//s_consoleWindow = FindWindowA(NULL, title);
		///SetConsoleTitleA("Console");
		s_consoleHandle = CreateFileA("CONOUT$", GENERIC_WRITE, FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
	}

	return index;
}
//-------------------------------------------------------------------------------------------------
void Log_DeinitSystem(int handle)
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
void Log_Write(const char* path, int lineNum, int handle, bool toConsole, bool error, const char* fmt, ...)
{
	FILE* filePtr = nullptr;
	if (handle)
	{
		for (auto& log : s_logs)
		{
			if (log.identifier == handle)
			{
				filePtr = log.file;
				break;
			}
		}
		if (!filePtr)
			return;
	}
	else
	{
		filePtr = s_genericLogFile;
	}


	std::string path_string(path);
	std::string base_filename = path_string.substr(path_string.find_last_of("/\\") + 1);

	char msg[1024];
	va_list args;
	va_start(args, fmt);
	vsprintf_s(msg, fmt, args);
	va_end(args);

	if (s_consoleHandle && toConsole)
	{
		snprintf(buf, 1024, "%s\n", msg);
		if (!WriteConsoleA(s_consoleHandle, buf, strlen(buf), 0, NULL))
		{
			char errMsg[1024];
			GetLastErrorString(errMsg, 1024);
			snprintf(buf, 1024, "Failed to create console: %s", errMsg);
			fputs(buf, filePtr);
		}
	}
	auto now = std::chrono::high_resolution_clock::now();
	float secondsSinceStart = (now - s_startTime).count() / (1000.f * 1000.f * 1000.f);
	snprintf(buf, 1024, "[%20s:%4d][%7.2f] %s\n", base_filename.c_str(), lineNum, secondsSinceStart, msg);
	fputs(buf, filePtr);
	if (filePtr != s_genericLogFile)
	{
		fputs(buf, s_genericLogFile); // everything goes in this log
	}

	if (error)
	{
		#ifdef _DEBUG
		if (IsDebuggerPresent())
		{
			__debugbreak();
		}
		#endif
	}
}