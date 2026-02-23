#include "pch.h"
#include "fbMainHooks.h"
#include <Core/Program.h>

DEFINE_HOOK(
	fb_Environment_getHostIdentifier,
	__cdecl,
	const char*
)
{
	if (!g_program->IsServer())
	{
		return g_program->GetClient()->GetPlayerName();
	}
	return Orig_fb_Environment_getHostIdentifier();
}

DEFINE_HOOK(
	fb_Console_writeConsole,
	__cdecl,
	void,

	const char* tag,
	const char* buffer,
	unsigned int size
)
{
	// we only care about messages tagged with ingame
	if(g_program->IsServer() && strcmp(tag, "ingame") == 0)
	{
		std::string_view msgView(buffer, size);
		CYPRESS_LOGTOSERVER(LogLevel::Info, "{}", msgView);
	}
	Orig_fb_Console_writeConsole(tag, buffer, size);
}

DEFINE_HOOK(
	fb_realMain,
	__cdecl,
	__int64,

	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	const char* lpCmdLine
)
{
	g_program->InitConfig();

	CYPRESS_LOGMESSAGE(LogLevel::Info, "Initializing {} (Cypress version {})", CYPRESS_GAME_NAME, GetCypressVersion().c_str());
	return Orig_fb_realMain(hInstance, hPrevInstance, lpCmdLine);
}

DEFINE_HOOK(
	fb_PVZMain_Ctor,
	__fastcall,
	__int64,

	void* a1,
	bool isDedicatedServer
)
{
	isDedicatedServer = g_program->IsServer();
	return Orig_fb_PVZMain_Ctor(a1, isDedicatedServer);
}

DEFINE_HOOK(
	fb_getServerBackendType,
	__fastcall,
	int,

	void* a1
)
{
	int origBackendType = Orig_fb_getServerBackendType(a1);
	if (origBackendType == 0 || g_program->IsServer())
		return 3;

	return origBackendType;
}

DEFINE_HOOK(
	fb_main_createWindow,
	__fastcall,
	__int64,

	HINSTANCE hInstance,
	bool dedicatedServer,
	bool consoleClient,
	DWORD gameLoopThreadId
)
{
	dedicatedServer = g_program->IsServer();
	return Orig_fb_main_createWindow(hInstance, dedicatedServer, consoleClient, gameLoopThreadId);
}

DEFINE_HOOK(
	fb_isInteractionAllowed,
	__fastcall,
	bool,

	void* a1,
	unsigned int a2
)
{
	return true;
}

DEFINE_HOOK(
	fb_tickersAllowedToShow,
	__fastcall,
	bool,

	void* a1
)
{
	return true;
}