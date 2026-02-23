#pragma once
#include <cstdint>
#include <StringUtil.h>

#define OFFSET_FB_REGISTERMESSAGELISTENER 0x1401A1450
namespace fb
{
	class Message;

	class MessageListener
	{
	public:
		enum
		{
			kDefaultOrdering = 100
		};

		virtual void     onMessage(fb::Message& inMessage) = 0;
		virtual uint16_t ordering() const { return kDefaultOrdering; }
	};

	void RegisterMessageListener(
		void* messageManager,
		const char* category,
		fb::MessageListener* listener,
		int localPlayerId = 255);
}