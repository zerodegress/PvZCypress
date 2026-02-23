#pragma once

#include <fb/Main.h>

#define OFFSET_ENVIRONMENT_GETHOSTIDENTIFIER CYPRESS_GW_SELECT(0x1403A9170, 0x1401F7C00)

#define OFFSET_CONSOLE__WRITECONSOLE CYPRESS_GW_SELECT(0x140392720, 0x1401A7850)
#define OFFSET_CONSOLE__ENQUEUECOMMAND CYPRESS_GW_SELECT(0x14038FA50, 0x1401A7680)

#define OFFSET_FB_ISINTERACTIONALLOWED 0x140B8F820
#define OFFSET_FB_TICKERSALLOWEDTOSHOW 0x140E0A8F0

DECLARE_HOOK(
	fb_Environment_getHostIdentifier,
	__cdecl,
	const char*
);

DECLARE_HOOK(
	fb_Console_writeConsole,
	__cdecl,
	void,

	const char* tag,
	const char* buffer,
	unsigned int size
);

DECLARE_HOOK(
	fb_realMain,
	__cdecl,
	__int64,

	HINSTANCE hIntance,
	HINSTANCE hPrevInstance,
	const char* lpCmdLine
);

DECLARE_HOOK(
	fb_PVZMain_Ctor,
	__fastcall,
	__int64,

	void* a1,
	bool isDedicatedServer
);

DECLARE_HOOK(
	fb_getServerBackendType,
	__fastcall,
	int,

	void* a1
);

DECLARE_HOOK(
	fb_main_createWindow,
	__fastcall,
	__int64,

	HINSTANCE hInstance,
	bool dedicatedServer,
	bool consoleClient,
	DWORD gameLoopThreadId
);

DECLARE_HOOK(
	fb_isInteractionAllowed,
	__fastcall,
	bool,

	void* a1,
	unsigned int a2
);

DECLARE_HOOK(
	fb_tickersAllowedToShow,
	__fastcall,
	bool,

	void* a1
);