#pragma once
#include <ctype.h>
#include <EASTL/vector.h>
#include <fb/Engine/String.h>

#define OFFSET_EXECUTIONCONTEXT_GETOPTIONVALUE CYPRESS_GW_SELECT(0x1403A97D0, 0x1401C51E0)
#define OFFSET_EXECUTIONCONTEXT_ADDOPTIONS 0x1403A7A80 // gw1
#define OFFSET_ECDATA_START 0x142B7EA80

namespace fb
{
	class ExecutionContext
	{
	public:
		static const char* getOptionValue(const char* optionName, const char* defaultValue = nullptr, int* token = nullptr)
		{
			auto ExecutionContext__getOptionValue =
				reinterpret_cast<char* (*)(const char* optionName, const char* defaultValue, int* token)>(OFFSET_EXECUTIONCONTEXT_GETOPTIONVALUE);
			return ExecutionContext__getOptionValue(optionName, defaultValue, token);
		}

		static int argc()
		{
#ifdef CYPRESS_GW1
            auto ExecutionContext_argc = reinterpret_cast<__int64(*)()>(0x1403A7C40);
            return ExecutionContext_argc();
#else
            eastl::vector<fb::String>* ecDataOptions = (eastl::vector<fb::String>*)OFFSET_ECDATA_START;
            return ecDataOptions->size();
#endif
		}

		static void addOptions(bool replace, const char* text)
		{
#ifdef CYPRESS_GW1
            auto ExecutionContext__addOptions = reinterpret_cast<void(*)(bool replace, const char* text)>(OFFSET_EXECUTIONCONTEXT_ADDOPTIONS);
            ExecutionContext__addOptions(replace, text);
#else
            // inlined in gw2 of course, impl based off of bf3's ExecutionContextData::addOptions
            eastl::vector<fb::String>* ecDataOptions = (eastl::vector<fb::String>*)0x142B7EA80;
            eastl::vector<fb::String>& options = *ecDataOptions;

            if (replace)
            {
                options.empty() ? options.push_back("<no argv[0]>") : options.resize(1);
            }

            const char* optStrStart = text;

            while (*optStrStart)
            {
                while (*optStrStart && isspace(*optStrStart))
                    optStrStart++;

                if (*optStrStart == 0)
                    break;

                const char* optStrEnd = optStrStart;
                if (*optStrStart == '"')
                {
                    optStrStart++;
                    optStrEnd = optStrStart;

                    while (*optStrEnd && *optStrEnd != '"')
                        optStrEnd++;

                    if (*optStrEnd == '"')
                    {
                        fb::String newOption(optStrStart, eastl_size_t(optStrEnd - optStrStart));
                        options.push_back(newOption);
                        optStrStart = optStrEnd + 1;
                    }
                }
                else
                {
                    while (*optStrEnd && !isspace(*optStrEnd))
                        optStrEnd++;

                    fb::String newOption(optStrStart, eastl_size_t(optStrEnd - optStrStart));
                    options.push_back(newOption);
                    optStrStart = optStrEnd;
                }
            }
#endif
		}
	};
}