#pragma once
#include "ITypedObject.h"
#include <cstdint>

namespace fb
{
	enum NetworkMessage : uint32_t
	{
		OnPlayerSpawnedMessage = 0xCAB70D78,
	};

	enum ClientWeaponMessage : uint32_t
	{
		PrimaryOutOfAmmoMessage = 0xA0325B03,
		ReloadBeginMessage = 0x929B8152,
	};

	enum ClientPVZCharacterMessage : uint32_t
	{
		DamagedEnemeyMessage = 0xBE722327,
	};

	enum UINetworkMessage : uint32_t
	{
		PlayerConnectMessage = 0xD3A50DC1,
		PlayerDisconnectMessage = 0xCDCD179F,
	};

	enum UIMessage : uint32_t
	{
		DamageMessage = 0x318336B9,
	};

	class Message : public ITypedObject
	{
	public:
		int m_category;
		int m_type;
		const char* senderCallstack;
		long double m_dispatchTime;
		const Message* m_next;
		int m_postedAtProcessMessageCounter;
		bool m_ownedByMessageManager;
	};
}