#pragma once
#include <json.hpp>
#include <iostream>
#include <fstream>

class ServerBanlist
{
public:
    struct PlayerEntry
    {
        std::string Name;
        std::string MachineId;
        std::string BanReason;

        void to_json(nlohmann::json& j, const PlayerEntry& p) {
            j = nlohmann::json{ {"Name", p.Name}, {"MachineId", p.MachineId}, {"BanReason", p.BanReason} };
        }

        void from_json(const nlohmann::json& j, PlayerEntry& p) {
            j.at("Name").get_to(p.Name);
            j.at("MachineId").get_to(p.MachineId);
            j.at("BanReason").get_to(p.BanReason);
        }
    };

    void LoadFromFile(const char* filename)
    {
        std::ifstream input(filename);
        if (input.is_open())
        {
            m_filename = filename;
            nlohmann::json json;
            input >> json;

            for (const auto& jsonEntry : json)
            {
                PlayerEntry entry;
                entry.Name = jsonEntry.at("Name").get<std::string>();
                entry.MachineId = jsonEntry.at("MachineId").get<std::string>();
                entry.BanReason = jsonEntry.at("BanReason").get<std::string>();
                m_bannedPlayers.push_back(entry);
            }
        }
    }

    void SaveToFile()
    {
        nlohmann::json j = nlohmann::json::array();
        for (const auto& player : m_bannedPlayers)
        {
            j.push_back({
                {"Name", player.Name},
                {"MachineId", player.MachineId},
                {"BanReason", player.BanReason}
                });
        }

        std::ofstream jsonFile("bans.json");
        if (jsonFile.is_open())
        {
            jsonFile << j.dump(4);
        }
    }

    void AddToList(const char* name, const char* machineId, const char* reasonText)
    {
        if (IsBanned(name))
            return;

        PlayerEntry entry;
        entry.Name = name;
        entry.MachineId = machineId;
        entry.BanReason = reasonText;
        m_bannedPlayers.push_back(entry);

        SaveToFile();
    }

    const PlayerEntry& GetPlayerEntry(const char* name)
    {
        for (auto it = m_bannedPlayers.begin(); it != m_bannedPlayers.end();)
        {
            if (!strcmp(it->Name.c_str(), name))
            {
                return *it;
            }
            else
            {
                ++it;
            }
        }
    }

    void RemoveFromList(const char* name)
    {
        if (!IsBanned(name))
            return;

        for (auto it = m_bannedPlayers.begin(); it != m_bannedPlayers.end();)
        {
            if (!strcmp(it->Name.c_str(), name))
            {
                m_bannedPlayers.erase(it);
                break;
            }
            else
            {
                ++it;
            }
        }

        SaveToFile();
    }

    bool IsBanned(const char* name)
    {
        for (const auto& player : m_bannedPlayers)
        {
            if (!strcmp(player.Name.c_str(), name))
            {
                return true;
            }
        }
        return false;
    }

    const std::vector<PlayerEntry>& GetBannedPlayers() { return m_bannedPlayers; }

private:
    const char* m_filename;
    std::vector<PlayerEntry> m_bannedPlayers;
};