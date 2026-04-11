#pragma once
#include <json.hpp>
#include <fstream>
#include <cstring>
#include <string>
#include <vector>

class ServerWhitelist
{
public:
	bool LoadFromFile(const char* filename)
	{
		m_allowedPlayers.clear();
		m_enabled = false;

		std::ifstream input(filename);
		if (!input.is_open())
		{
			return false;
		}

		try
		{
			nlohmann::json json;
			input >> json;

			if (!json.is_array())
			{
				return false;
			}

			for (const auto& entry : json)
			{
				if (entry.is_string())
				{
					m_allowedPlayers.push_back(entry.get<std::string>());
					continue;
				}

				if (entry.is_object() && entry.contains("Name") && entry["Name"].is_string())
				{
					m_allowedPlayers.push_back(entry["Name"].get<std::string>());
				}
			}
		}
		catch (...)
		{
			return false;
		}

		m_enabled = true;
		return true;
	}

	bool IsEnabled() const
	{
		return m_enabled;
	}

	bool IsWhitelisted(const char* name) const
	{
		if (!name)
			return false;

		for (const auto& allowedName : m_allowedPlayers)
		{
			if (!strcmp(allowedName.c_str(), name))
			{
				return true;
			}
		}
		return false;
	}

	size_t GetPlayerCount() const
	{
		return m_allowedPlayers.size();
	}

private:
	bool m_enabled = false;
	std::vector<std::string> m_allowedPlayers;
};
