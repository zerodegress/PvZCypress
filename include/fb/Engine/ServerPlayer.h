#pragma once
#include <cstdint>
#include <MemUtil.h>
#include <fb/TypeInfo/SecureReason.h>

#define OFFSET_SERVERPLAYER_DISCONNECT CYPRESS_GW_SELECT(0x14075D860, 0x140614B60)

namespace fb
{
    class ServerPlayer
    {
    public:
        char pad_0000[24]; //0x0000
        const char* m_name; //0x0018
#ifdef CYPRESS_GW2
        char pad_0020[24]; //0x0020
        uint64_t m_onlineId; //0x0038
        char pad_0040[4952]; //0x0040
        int64_t m_stateMask; //0x1398
#endif

        unsigned int getPlayerId()
        {
            return ptrread<unsigned int>(this, CYPRESS_GW_SELECT(0xD90, 0x1490));
        }

        bool isAIPlayer()
        {
#ifdef CYPRESS_GW1
            return ptrread<bool>(this, 0xCB8);
#else
            return (m_stateMask & StateMask::k_isAI) != 0;
#endif
        }

        bool isSpectator()
        {
#ifdef CYPRESS_GW1
            //TODO
            return false;
#else
            return (m_stateMask & StateMask::k_isSpectator) != 0;
#endif
        }

        bool isPersistentAIPlayer()
        {
#ifdef CYPRESS_GW1
            return isAIPlayer();
#else
            return (m_stateMask & StateMask::k_isPersistentAI) != 0;
#endif
        }

        bool isAIOrPersistentAIPlayer()
        {
            return isAIPlayer() || isPersistentAIPlayer();
        }

        void disconnect(fb::SecureReason reason, eastl::string& reasonText)
        {
            auto ServerPlayer__disconnect = reinterpret_cast<void (*)(void* inst, fb::SecureReason reason, eastl::string & reasonText)>(OFFSET_SERVERPLAYER_DISCONNECT);
            ServerPlayer__disconnect(this, reason, reasonText);
        }

    private:
        enum StateMask
        {
            k_isAI = 1 << 0,
            k_isSpectator = 1 << 1,
            k_isPersistentAI = 1 << 2,
        };
    }; //Size: 0x1988
}