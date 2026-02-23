#include "pch.h"
#include "fbClientHooks.h"
#include <fb/Engine/MessageListener.h>
#include <fb/Engine/ExecutionContext.h>
#include <Core/Program.h>

void* g_persistenceClassThing = nullptr;

DEFINE_HOOK(
	fb_Client_enterState,
	__fastcall,
	void,

	void* thisPtr,
	fb::ClientState state,
	fb::ClientState prevState
)
{
	Cypress::Client* client = g_program->GetClient();
	client->SetFbClientInstance(thisPtr);
	client->SetClientState(state);

	/*
	static bool registeredListeners = false;
	if (state == fb::ClientState_LoadProfileOptions && !registeredListeners)
	{
		void* clientGameContext = *(void**)0x142B5BB90;
		void* msgMgr = ptrread<void*>(clientGameContext, 0x10);

		fb::RegisterMessageListener(msgMgr, "Client", client);
		registeredListeners = true;
	}
	*/

	if (state == fb::ClientState_ConnectToServer)
	{
		client->SetJoiningServer(true);
	}
	if (prevState == fb::ClientState_ConnectToServer)
	{
		client->SetJoiningServer(false);
	}

#ifndef CYPRESS_GW1
	static bool profileOptionsApplied = false;
	if (state == fb::ClientState_WaitingForGhosts && !profileOptionsApplied)
	{
		// apply profile options
		if (g_persistenceClassThing)
		{
			auto func_140DAA330 = reinterpret_cast<void(*)(void*)>(0x140DAB030);
			func_140DAA330(g_persistenceClassThing);
		}
		auto fbApplyRenderOptionsFromProfile = reinterpret_cast<void(*)(int localPlayerId)>(0x140DA9A90);
		fbApplyRenderOptionsFromProfile(0);
		profileOptionsApplied = true;
	}
#endif

	Orig_fb_Client_enterState(thisPtr, state, prevState);
}

DEFINE_HOOK(
	fb_140DA9B90,
	__fastcall,
	void*,

	void* a1,
	void* a2,
	int localPlayerId
)
{
	void* result = Orig_fb_140DA9B90(a1, a2, localPlayerId);
	if (localPlayerId == 0)
	{
		g_persistenceClassThing = result;
	}
	return result;
}

DEFINE_HOOK(
	fb_OnlineManager_connectToAddress,
	__fastcall,
	void,

	void* thisPtr,
	const char* ipAddr,
	const char* serverPassword
)
{
	const char* passwordArg;
	if ((passwordArg = fb::ExecutionContext::getOptionValue("password")))
	{
		serverPassword = passwordArg;
	}
	Orig_fb_OnlineManager_connectToAddress(thisPtr, ipAddr, serverPassword);
}

DEFINE_HOOK(
	fb_OnlineManager_onGotDisconnected,
	__fastcall,
	void,

	void* thisPtr,
	fb::SecureReason reason,
	eastl::string& reasonText
)
{
	eastl::string reasonStr = "No reason provided";
	if (!reasonText.empty())
		reasonStr = reasonText;

	std::string bodyMsg = std::format("Disconnect: {} ({})", reasonStr.c_str(), fb::SecureReason_toString[reason]);

	MessageBoxA(GetActiveWindow(), bodyMsg.c_str(), "Disconnected", MB_ICONINFORMATION | MB_OK);
	exit(0xCC1);
}

DEFINE_HOOK(
	fb_PVZGetNumTutorialVideos,
	__fastcall,
	void,

	void* thisPtr,
	void* dataKey
)
{
	Orig_fb_PVZGetNumTutorialVideos(thisPtr, dataKey);
	ptrset<int>(dataKey, 0x8, 0); // dataKey->value = 0;
}

DEFINE_HOOK(
	fb_ClientConnection_onDisconnected,
	__fastcall,
	void,

	void* thisPtr
)
{
	struct EngineConnection_GW1
	{
	public:
		char pad_0000[1616]; //0x0000
		eastl::string m_reasonText; //0x0650
		int32_t m_reason; //0x0670
	};

	EngineConnection_GW1* thisConnection = reinterpret_cast<EngineConnection_GW1*>(thisPtr);

	char buf[1024];
	const char* secureReasonStr;
	switch (thisConnection->m_reason)
	{
	case 0x0: secureReasonStr = "Ok"; break;
	case 0x1: secureReasonStr = "Wrong Protocol Version"; break;
	case 0x2: secureReasonStr = "Wrong Title Version"; break;
	case 0x3: secureReasonStr = "Server Full"; break;
	case 0x4: secureReasonStr = "Kicked Out"; break;
	case 0x5: secureReasonStr = "Banned"; break;
	case 0x6: secureReasonStr = "Generic Error"; break;
	case 0x7: secureReasonStr = "Wrong Password"; break;
	case 0x8: secureReasonStr = "Kicked Out (Demo Over)"; break;
	case 0x9: secureReasonStr = "Rank Restricted"; break;
	case 0xA: secureReasonStr = "Configuration Not Allowed"; break;
	case 0xB: secureReasonStr = "Server Reclaimed"; break;
	case 0xC: secureReasonStr = "Missing Content"; break;
	case 0xD: secureReasonStr = "Not Verified"; break;
	case 0xE: secureReasonStr = "Timed Out"; break;
	case 0xF: secureReasonStr = "Connect Failed"; break;
	case 0x10: secureReasonStr = "No Reply"; break;
	case 0x11: secureReasonStr = "Accept Failed"; break;
	case 0x12: secureReasonStr = "Mismatching Content"; break;
	case 0x13: secureReasonStr = "Interactivity Timeout"; break;
	case 0x14: secureReasonStr = "Kicked From Queue"; break;
	case 0x15: secureReasonStr = "Team Kills"; break;
	case 0x16: secureReasonStr = "Kicked By Admin"; break;
	case 0x17: secureReasonStr = "Kicked Via PunkBuster"; break;
	case 0x18: secureReasonStr = "Kicked Out (Server Full)"; break;
	case 0x19: secureReasonStr = "ESports Match Starting"; break;
	case 0x1A: secureReasonStr = "Not In ESports Rosters"; break;
	case 0x1B: secureReasonStr = "ESports Match Ending"; break;
	case 0x1C: secureReasonStr = "Virtual Server Expired"; break;
	case 0x1D: secureReasonStr = "Virtual Server Recreate"; break;
	case 0x1E: secureReasonStr = "ESports Team Full"; break;
	case 0x1F: secureReasonStr = "ESports Match Aborted"; break;
	case 0x20: secureReasonStr = "ESports Match Walkover"; break;
	case 0x21: secureReasonStr = "ESports Match Warmup Timed Out"; break;
	case 0x22: secureReasonStr = "Not Allowed To Spectate"; break;
	case 0x23: secureReasonStr = "No Spectate Slot Available"; break;
	case 0x24: secureReasonStr = "Invalid Spectate Join"; break;
	case 0x25: secureReasonStr = "Kicked Via FairFight"; break;
	case 0x26: secureReasonStr = "Kicked Commander On Leave"; break;
	case 0x27: secureReasonStr = "Kicked Commander After Mutiny"; break;
	case 0x28: secureReasonStr = "Server Maintenance"; break;
	default: secureReasonStr = "Unknown Reason"; break;
	}

	if (thisConnection->m_reasonText.empty())
	{
		sprintf_s(buf, "Disconnect: %s", secureReasonStr);
	}
	else
	{
		sprintf_s(buf, "Disconnect: %s (%s)", thisConnection->m_reasonText.c_str(), secureReasonStr);
	}
	Orig_fb_ClientConnection_onDisconnected(thisPtr);
	if (MessageBoxA(NULL, buf, "Disconnected from Server", MB_ICONINFORMATION | MB_OK))
	{
		exit(0);
	}
}