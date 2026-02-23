#pragma once
#include <string>
#include <format>
#include <ctime>

#define CYPRESS_VERSION_MAJOR 0
#define CYPRESS_VERSION_MINOR 0
#define CYPRESS_VERSION_PATCH 1

#ifdef _DEBUG
#define CYPRESS_BUILD_CONFIG "Debug"
#else
#define CYPRESS_BUILD_CONFIG "Release"
#endif

#ifdef CYPRESS_GW1
#define CYPRESS_GAME_NAME "GW1"
#else
#define CYPRESS_GAME_NAME "GW2"
#endif

#define CYPRESS_BUILDDATE (__DATE__ " " __TIME__)

static std::string GetCypressVersion()
{
	return std::format("{}.{}.{} ({})", CYPRESS_VERSION_MAJOR, CYPRESS_VERSION_MINOR, CYPRESS_VERSION_PATCH, CYPRESS_BUILDDATE);
}