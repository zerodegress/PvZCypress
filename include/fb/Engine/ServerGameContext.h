#pragma once
#include "ServerPlayerManager.h"
#include "ServerPeer.h"

#define OFFSET_SERVERGAMECONTEXT CYPRESS_GW_SELECT(0x141FC3180, 0x142B65680)

namespace fb
{
    class ServerGameContext
    {
    public:
        char pad_0000[104]; //0x0000
        ServerPlayerManager* m_serverPlayerManager; //0x0068
        ServerPeer* m_serverPeer; //0x0070

        static ServerGameContext* GetInstance(void)
        {
            return *(ServerGameContext**)OFFSET_SERVERGAMECONTEXT;
        }
    }; //Size: 0x0080
}