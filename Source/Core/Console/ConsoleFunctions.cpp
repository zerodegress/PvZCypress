#include "pch.h"
#include "ConsoleFunctions.h"
#include <sstream>
#include <string>

namespace Cypress
{
    ConsoleFunction* GetFunction(const char* name)
    {
        for (auto& function : g_consoleFunctions)
        {
            if (strcmp(name, function.FunctionName) == 0)
            {
                return &function;
            }
        }
        return nullptr;
    }

    std::vector<std::string> ParseCommandString(const std::string& str)
    {
        std::vector<std::string> words;
        std::string word;
        char ch;
        bool inQuotes = false;

        for (auto&& ch : str) {
            if (ch == '"') {
                inQuotes = !inQuotes;
            }
            else if (ch == ' ' && !inQuotes) {
                if (!word.empty()) {
                    words.push_back(std::move(word));
                    word.clear();
                }
            }
            else {
                word += ch;
            }
        }

        if (!word.empty()) {
            words.push_back(std::move(word));
        }

        return words;
    }

    void HandleCommand(const std::string& command)
    {
        std::vector<std::string> commandAndArgs = ParseCommandString(command);
        if (commandAndArgs.size() != 0)
        {
            std::string& commandName = commandAndArgs[0];
            ConsoleFunction* func = GetFunction(commandName.c_str());

            if (func)
            {
                commandAndArgs.erase(commandAndArgs.begin());
                func->FunctionPtr(commandAndArgs);
            }
        }
    }
}