#pragma once
#include <EASTL/vector.h>
#include <vector>

namespace Cypress
{
	typedef std::vector<std::string> ArgList;
	typedef void(__cdecl* ConsoleFunctionPtr)(ArgList);

	struct ConsoleFunction
	{
		ConsoleFunctionPtr FunctionPtr;
		const char* FunctionName;
		const char* FunctionDescription;

		ConsoleFunction(ConsoleFunctionPtr pFunc, const char* funcName, const char* funcDesc)
			: FunctionPtr(pFunc)
			, FunctionName(funcName)
			, FunctionDescription(funcDesc)
		{ }
	};

	extern std::vector<ConsoleFunction> g_consoleFunctions;

	ConsoleFunction* GetFunction(const char* name);
	std::vector<std::string> ParseCommandString(const std::string& str);
	void HandleCommand(const std::string& command);

#define CYPRESS_CONSOLE_FUNCTION(func, name, desc) \
	{ &func, name, desc }

#define CYPRESS_REGISTER_CONSOLE_FUNCTION(func, name, desc) \
	g_consoleFunctions.push_back({&func, name, desc});
}