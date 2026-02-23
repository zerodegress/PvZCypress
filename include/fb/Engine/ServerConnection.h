#pragma once
#include <EASTL/string.h>

#define OFFSET_SERVERCONNECTION__ONCREATEPLAYERMESSAGE 0x14064FA40

namespace fb
{
    class ServerConnection
    {
    public:
        void sendMessage(void* networkableMessage)
        {
            auto func = reinterpret_cast<void(*)(ServerConnection*, void*)>(0x14064F6A0);
            func(this, networkableMessage);
        }

#ifdef CYPRESS_GW1
        char pad_0000[2896]; //0x0000
        class N00000E6B* N00000787; //0x0B50
        class N00004555* N00000788; //0x0B58
        char pad_0B60[6288]; //0x0B60
        eastl::string m_machineId; //0x23F0
        char pad_2410[33]; //0x2410
        bool m_shouldDisconnect; //0x2431
        char pad_2432[2]; //0x2432
        uint32_t m_disconnectReason; //0x2434
        eastl::string m_reasonText;
#else
        char pad_0000[23072]; //0x0000
        eastl::string m_machineId; //0x5A20
        char pad_5A40[33]; //0x5A40
        bool m_shouldDisconnect; //0x5A61
        uint32_t m_disconnectReason; //0x5A64
        eastl::string m_reasonText; //0x5A68
#endif
    }; //Size: 0x6098
}