#include "pch.h"
#include <Core/Program.h>
#include <Core/Logging.h>
#include <Core/Config.h>
#include <fb/Engine/Server.h>
#include <algorithm>
#include <chrono>
#include <mutex>
#include <deque>
#include <vector>
#include <fstream>

HWND* g_listBox = (HWND*)OFFSET_LISTBOX;

namespace
{
	std::mutex g_serverLogBufferMutex;
	std::deque<std::pair<uint64_t, std::string>> g_serverLogBuffer;
	uint64_t g_serverLogCounter = 0;
	constexpr size_t SERVER_LOG_BUFFER_LIMIT = 2000;
	std::mutex g_serverLogFileMutex;
	std::ofstream g_serverLogFile;
	bool g_serverLogFileOpenAttempted = false;

	std::mutex g_serverEventBufferMutex;
	std::deque<CypressServerEvent> g_serverEventBuffer;
	uint64_t g_serverEventCounter = 0;
	constexpr size_t SERVER_EVENT_BUFFER_LIMIT = 5000;

	void AddBufferedServerLog(const std::string& formattedLog)
	{
		std::scoped_lock lock(g_serverLogBufferMutex);
		g_serverLogBuffer.emplace_back(++g_serverLogCounter, formattedLog);
		if (g_serverLogBuffer.size() > SERVER_LOG_BUFFER_LIMIT)
		{
			g_serverLogBuffer.pop_front();
		}
	}

	void AppendServerLogToFile(const std::string& formattedLog)
	{
		std::scoped_lock lock(g_serverLogFileMutex);
		if (!g_serverLogFileOpenAttempted)
		{
			g_serverLogFileOpenAttempted = true;
			g_serverLogFile.open("cypress-log.txt", std::ios::out | std::ios::app);
		}

		if (g_serverLogFile.is_open())
		{
			g_serverLogFile << formattedLog << '\n';
			g_serverLogFile.flush();
		}
	}

	uint64_t GetTimeNowMs()
	{
		const auto now = std::chrono::system_clock::now();
		const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
		return (uint64_t)millis;
	}
}

void Cypress_LogToServer(const char* msg, const char* fileName, int lineNumber, LogLevel logLevel)
{
	if (!g_program || !g_program->IsServer())
		return;

	Cypress::Server* server = g_program->GetServer();
	if (!server || !server->GetServerLogEnabled())
		return;

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
	AppendServerLogToFile(formattedLog);

	//bool* m_logOutputEnabled = (bool*)(OFFSET_LOGOUTPUTENABLED);
	if (*g_listBox == NULL)
		std::cout << "\x1B[36m[SrvLog]" << formattedLog.c_str() << "\x1B[0m";
	else
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

uint64_t Cypress_PublishServerEvent(const char* type, const char* source, const std::string& payloadJson)
{
	CypressServerEvent evt;
	evt.TimestampMs = GetTimeNowMs();
	evt.Type = type ? type : "";
	evt.Source = source ? source : "";
	evt.PayloadJson = payloadJson;

	std::scoped_lock lock(g_serverEventBufferMutex);
	evt.Id = ++g_serverEventCounter;
	g_serverEventBuffer.emplace_back(evt);
	if (g_serverEventBuffer.size() > SERVER_EVENT_BUFFER_LIMIT)
	{
		g_serverEventBuffer.pop_front();
	}
	return evt.Id;
}

uint64_t Cypress_GetLatestServerEventId()
{
	std::scoped_lock lock(g_serverEventBufferMutex);
	return g_serverEventCounter;
}

std::vector<CypressServerEvent> Cypress_GetServerEventsSince(uint64_t sinceExclusive, size_t limit, const std::string& typeFilter)
{
	std::vector<CypressServerEvent> events;
	if (limit == 0)
		return events;

	std::scoped_lock lock(g_serverEventBufferMutex);
	events.reserve(std::min(limit, g_serverEventBuffer.size()));
	for (const auto& evt : g_serverEventBuffer)
	{
		if (evt.Id <= sinceExclusive)
			continue;
		if (!typeFilter.empty() && evt.Type != typeFilter)
			continue;
		events.push_back(evt);
		if (events.size() >= limit)
			break;
	}
	return events;
}
