#pragma once
#include <EASTL/vector.h>
#include <fb/Engine/ServerPlayer.h>

#define OFFSET_SERVERPLAYERMANAGER_ADDPLAYER CYPRESS_GW_SELECT(0x14075B800, 0x140616DC0)

namespace fb
{
    class ServerPlayerManager
    {
    public:
#ifdef CYPRESS_GW1
        char pad_0000[1320]; //0x0000
        eastl::vector<ServerPlayer*> m_players; //0x0528
#else
        char pad_0000[616]; //0x0000
        eastl::vector<ServerPlayer*> m_players; //0x0268
        char pad_0288[520]; //0x0288
        eastl::vector<ServerPlayer*> m_spectators; //0x0490
        char pad_04B0[1088]; //0x04B0
        ServerPlayer** m_idToPlayerMap; //0x08F0
#endif

        int playerCount()
        {
            return m_players.size();
        }

        int spectatorCount()
        {
#ifdef CYPRESS_GW1
            //TODO
            return 0;
#else
            return m_spectators.size();
#endif
        }

        unsigned int maxSpectatorCount()
        {
            return ptrread<unsigned int>(this, 0x8E0);
        }

        int humanPlayerCount()
        {
            int count = 0;
            for (const auto& player : m_players)
            {
                if (!player->isAIOrPersistentAIPlayer())
                {
                    ++count;
                }
            }
            return count;
        }

        int aiPlayerCount()
        {
            int count = 0;
            for (const auto& player : m_players)
            {
                if (player->isAIOrPersistentAIPlayer())
                {
                    ++count;
                }
            }
            return count;
        }

        ServerPlayer* findByName(const char* name)
        {
            for (size_t i = 0; i < m_players.size(); i++)
            {
                ServerPlayer* player = m_players.at(i);
                if (player && player->m_name && (strcmp(player->m_name, name) == 0))
                {
                    return player;
                }
            }
            return nullptr;
        }

        ServerPlayer* findHumanByName(const char* name)
        {
            int numPlayers = m_players.size();
            for (size_t i = 0; i < numPlayers; i++)
            {
                ServerPlayer* player = m_players.at(i);
                if (player->isAIOrPersistentAIPlayer())
                    continue;

                if (player && player->m_name && (strcmp(player->m_name, name) == 0))
                {
                    return player;
                }
            }
            return nullptr;
        }

        ServerPlayer* getById(unsigned int id)
        {
#ifdef CYPRESS_GW1
            //TODO
            return nullptr;
#else
            if (id < 0 || id > 63)
            {
                return nullptr;
            }
            return m_idToPlayerMap[id];
#endif
        }
    }; //Size: 0x10A0
}