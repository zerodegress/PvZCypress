#include "pch.h"
#include <Core/Program.h>
#include <Core/Logging.h>
#include <Core/Config.h>
#include <fb/Engine/Server.h>
#include <algorithm>
#include <mutex>
#include <deque>
#include <vector>

HWND* g_listBox = (HWND*)OFFSET_LISTBOX;

namespace
{
	std::mutex g_serverLogBufferMutex;
	std::deque<std::pair<uint64_t, std::string>> g_serverLogBuffer;
	uint64_t g_serverLogCounter = 0;
	constexpr size_t SERVER_LOG_BUFFER_LIMIT = 2000;

	void AddBufferedServerLog(const std::string& formattedLog)
	{
		std::scoped_lock lock(g_serverLogBufferMutex);
		g_serverLogBuffer.emplace_back(++g_serverLogCounter, formattedLog);
		if (g_serverLogBuffer.size() > SERVER_LOG_BUFFER_LIMIT)
		{
			g_serverLogBuffer.pop_front();
		}
	}
}

void Cypress_LogToServer(const char* msg, const char* fileName, int lineNumber, LogLevel logLevel)
{
#if(HAS_DEDICATED_SERVER)
    time_t now = time(nullptr);
    struct tm* tm_info = localtime(&now);

    char timePrefix[64];
    strftime(timePrefix, sizeof(timePrefix), "[%d.%m.%Y %H:%M:%S]", tm_info);

    const char* filePath = strrchr(fileName, '\\');
    if (filePath == nullptr)
        filePath = fileName;
    else
        ++filePath;

#endif
	std::string formattedLog = std::format("{} [{}] [{}:{}] {}", timePrefix, Cypress_LogLevelToStr(logLevel), filePath, lineNumber, msg);
	AddBufferedServerLog(formattedLog);

	//bool* m_logOutputEnabled = (bool*)(OFFSET_LOGOUTPUTENABLED);
	if (*g_listBox == NULL)
		std::cout << "\x1B[36m[SrvLog]" << formattedLog.c_str() << "\x1B[0m";
	else
	{
		if (g_program->GetServer()->GetServerLogEnabled())
		{
			int pos = (int)SendMessageA(*g_listBox, LB_ADDSTRING, 0, (LPARAM)formattedLog.c_str());
			if (pos >= 1000)
			{
				SendMessage(*g_listBox, LB_DELETESTRING, 0, 0);
				pos--;
			}

			SendMessage(*g_listBox, LB_SETCURSEL, pos, 1);
		}
	}
}

uint64_t Cypress_GetLatestServerLogId()
{
	std::scoped_lock lock(g_serverLogBufferMutex);
	return g_serverLogCounter;
}

std::vector<std::pair<uint64_t, std::string>> Cypress_GetServerLogsSince(uint64_t sinceExclusive, size_t limit)
{
	std::vector<std::pair<uint64_t, std::string>> logs;
	if (limit == 0)
		return logs;

	std::scoped_lock lock(g_serverLogBufferMutex);
	logs.reserve(std::min(limit, g_serverLogBuffer.size()));
	for (const auto& entry : g_serverLogBuffer)
	{
		if (entry.first > sinceExclusive)
		{
			logs.push_back(entry);
			if (logs.size() >= limit)
				break;
		}
	}
	return logs;
}
