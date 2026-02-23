#pragma once

#define OFFSET_FB_CLIENT_ENTERSTATE CYPRESS_GW_SELECT(0x14053F180, 0x140443220)
#define OFFSET_FB_140DA9B90 0x140DA9B90
#define OFFSET_FB_ONLINEMANAGER_ONGOTDISCONNECTED 0x14054B6E0
#define OFFSET_FB_ONLINEMANAGER_CONNECTTOADDRESS CYPRESS_GW_SELECT(0x140559CD0, 0x14054ABE0)

namespace fb
{
    enum ClientState
    {
        ClientState_WaitingForStaticBundleLoad,

        ClientState_LoadProfileOptions,
        ClientState_LostConnection,
        ClientState_WaitingForUnload,
        ClientState_Startup,
        ClientState_StartServer,

        ClientState_WaitingForLevel,
        ClientState_StartLoadingLevel,
        ClientState_WaitingForLevelLoaded,
        ClientState_WaitingForLevelLink,
        ClientState_LevelLinked,
        ClientState_WaitingForGhosts,

        ClientState_Ingame,
        ClientState_LeaveIngame,

        ClientState_ConnectToServer,

        ClientState_ShuttingDown,
        ClientState_Shutdown,

        ClientState_None,
    };

	class Client
	{

	};
}