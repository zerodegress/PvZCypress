#include "pch.h"
#include "Client.h"
#include <fb/Engine/Message.h>
#include <fb/Engine/TypeInfo.h>

namespace Cypress
{
	Client::Client()
		: m_socketManager(new Kyber::SocketManager(Kyber::ProtocolDirection::Serverbound, Kyber::SocketSpawnInfo(false, "", "")))
		, m_playerName(nullptr)
		, m_fbClientInstance(nullptr)
		, m_clientState(fb::ClientState::ClientState_None)
		, m_joiningServer(false)
	{

	}

	Client::~Client()
	{
	}

	void Client::onMessage(fb::Message& inMessage)
	{

	}
}
