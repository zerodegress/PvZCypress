#pragma once
#include <EASTL/vector.h>
#include <vector>

namespace Cypress
{
	void registerConsoleMethod(const char* groupName, const char* name, const char* description, void* pfn = (void*)0x1401A89E0, void* juiceFn = (void*)0);

#define CYPRESS_REGISTER_CONSOLE_FUNCTION(groupName, name, description, pfn) \
		registerConsoleMethod(groupName, name, description, pfn, 0)
}