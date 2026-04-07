#pragma once
#include <cstdio>
#include <iostream>
#include <format>
#include <cstdint>
#include <vector>
#include <string>

#include <Core/VersionInfo.h>
#include <Core/Config.h>

enum class LogLevel
{
	Info,
	Warning,
	Error,
	Debug
};

constexpr const char* Cypress_LogLevelToStr(LogLevel inLevel)
{
	switch (inLevel)
	{
	case LogLevel::Info: return "Info";
	case LogLevel::Warning: return "Warning";
	case LogLevel::Error: return "Error";
	case LogLevel::Debug: return "Debug";
	}
}

constexpr const char* Cypress_GetColorForLogLevel(LogLevel inLevel)
{
	switch (inLevel)
	{
	case LogLevel::Info: return "\x1B[32m";
	case LogLevel::Warning: return "\x1B[33m";
	case LogLevel::Error: return "\x1B[31m";
	case LogLevel::Debug: return "\x1B[36m";
	}
}

void Cypress_LogToServer(const char* msg, const char* fileName, int lineNumber, LogLevel logLevel);
uint64_t Cypress_GetLatestServerLogId();
std::vector<std::pair<uint64_t, std::string>> Cypress_GetServerLogsSince(uint64_t sinceExclusive, size_t limit = 100);

struct CypressServerEvent
{
	uint64_t Id;
	uint64_t TimestampMs;
	std::string Type;
	std::string Source;
	std::string PayloadJson;
};

uint64_t Cypress_PublishServerEvent(const char* type, const char* source, const std::string& payloadJson);
uint64_t Cypress_GetLatestServerEventId();
std::vector<CypressServerEvent> Cypress_GetServerEventsSince(
	uint64_t sinceExclusive,
	size_t limit = 100,
	const std::string& typeFilter = "");

#if(HAS_DEDICATED_SERVER)
#define CYPRESS_LOGTOSERVER(logLevel, fmt, ...) \
do { \
	std::string formattedMsg = std::format(fmt, ##__VA_ARGS__); \
	Cypress_LogToServer(formattedMsg.c_str(), __FILE__, __LINE__, logLevel); \
} while(0)
#else
#define CYPRESS_LOGTOSERVER(logLevel, fmt, ...)
#endif

#ifdef _DEBUG
#define CYPRESS_LOG_INTERNAL(msg, logLevel) \
std::cout << Cypress_GetColorForLogLevel(logLevel) \
<< "[Cypress-" \
<< CYPRESS_BUILD_CONFIG \
<< "] [" \
<< Cypress_LogLevelToStr(logLevel) \
<< "] [" \
<< __FILE__ \
<< ":" \
<< __LINE__ \
<< "] " \
<< msg \
<< "\x1B[0m" \
<< std::endl;
#else
#define CYPRESS_LOG_INTERNAL(msg, logLevel) \
std::cout << Cypress_GetColorForLogLevel(logLevel) \
<< "[Cypress-" \
<< CYPRESS_BUILD_CONFIG \
<< "] [" \
<< Cypress_LogLevelToStr(logLevel) \
<< "] " \
<< msg \
<< "\x1B[0m" \
<< std::endl;
#endif

#define CYPRESS_LOGMESSAGE(logLevel, fmt, ...) \
do { \
	std::string formattedMsg = std::format(fmt, ##__VA_ARGS__); \
	CYPRESS_LOG_INTERNAL(formattedMsg.c_str(), logLevel); \
} while(0)

// Debug log calls will be completely stripped out of release builds.
#ifdef _DEBUG
#define CYPRESS_DEBUG_LOGMESSAGE CYPRESS_LOGMESSAGE
#else
#define CYPRESS_DEBUG_LOGMESSAGE
#endif
