#pragma once
#include <MemUtil.h>
#include <EASTL/string.h>
#include <fb/Engine/Client.h>
#include <fb/SecureReason.h>

DECLARE_HOOK(
	fb_Client_enterState,
	__fastcall,
	void,

	void* thisPtr,
	fb::ClientState state,
	fb::ClientState prevState
);

DECLARE_HOOK(
	fb_140DA9B90,
	__fastcall,
	void*,

	void* a1,
	void* a2,
	int localPlayerId
);

DECLARE_HOOK(
	fb_OnlineManager_connectToAddress,
	__fastcall,
	void,

	void* thisPtr,
	const char* ipAddr,
	const char* serverPassword
);

DECLARE_HOOK(
	fb_OnlineManager_onGotDisconnected,
	__fastcall,
	void,

	void* thisPtr,
	fb::SecureReason reason,
	eastl::string& reasonText
);

DECLARE_HOOK(
	fb_PVZGetNumTutorialVideos,
	__fastcall,
	void,

	void* thisPtr,
	void* dataKey
);

DECLARE_HOOK(
	fb_ClientConnection_onDisconnected,
	__fastcall,
	void,

	void* thisPtr
);