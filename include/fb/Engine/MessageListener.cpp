#include "pch.h"
#include "MessageListener.h"

namespace fb
{
	typedef bool(__thiscall* tFbRegisterMessageListener)(void* self, uint32_t inCategoryHash, fb::MessageListener* inListener, int inLocalPlayerId);
	tFbRegisterMessageListener pFbRegisterMessageListener = (tFbRegisterMessageListener)(OFFSET_FB_REGISTERMESSAGELISTENER);

	void RegisterMessageListener(void* messageManager, const char* category, fb::MessageListener* listener, int localPlayerId)
	{
		bool registered = pFbRegisterMessageListener(messageManager, fnvHash(category), listener, localPlayerId);
		CYPRESS_ASSERT(registered == true, "Failed to register message listener for {} messages!", category);

		CYPRESS_DEBUG_LOGMESSAGE(LogLevel::Debug, "Registered message listener for {} messages", category);
	}
}
