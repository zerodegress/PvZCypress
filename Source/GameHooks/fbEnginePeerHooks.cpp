#include "pch.h"
#include "fbEnginePeerHooks.h"

#include <Core/Program.h>
#include <Core/Logging.h>
#include <Kyber/SocketManager.h>

DEFINE_HOOK(
	fb_EnginePeer_init,
	__fastcall,
	void,

	void* thisPtr,
	void* crypto,
	Kyber::SocketManager* socketManager,
	const char* address,
	int titleId,
	int versionId
)
{
	CYPRESS_DEBUG_LOGMESSAGE(LogLevel::Debug, "EnginePeer::init (address = {}, titleId = {}, versionId = {})", address, titleId, versionId);

	if (!g_program->GetInitialized())
	{
		bool wsaInit = g_program->InitWSA();
		CYPRESS_ASSERT(wsaInit, "WSA failed to initialize!");
		g_program->SetInitialized(true);
	}

#if(CAN_HOST_SERVER)
	if (strstr(address, ":252"))
	{
		socketManager = g_program->GetServer()->GetSocketManager();
	}
#endif
	if (strstr(address, ":251"))
	{
		socketManager = g_program->GetClient()->GetSocketManager();
	}

	Orig_fb_EnginePeer_init(thisPtr, crypto, socketManager, address, titleId, versionId);
}