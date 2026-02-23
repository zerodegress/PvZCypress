#pragma once
#include <fb/Engine/Client.h>
#include <fb/Engine/MessageListener.h>
#include <Kyber/SocketManager.h>

namespace Cypress
{
	class Client : public fb::MessageListener
	{
	public:
		Client();
		~Client();

		virtual void onMessage(fb::Message& inMessage) override;

		void* GetFbClientInstance() { return m_fbClientInstance; }
		void SetFbClientInstance(void* client) { m_fbClientInstance = client; }

		fb::ClientState GetClientState() { return m_clientState; }
		void SetClientState(fb::ClientState newState) { m_clientState = newState; }

		bool GetJoiningServer() { return m_joiningServer; }
		void SetJoiningServer(bool value) { m_joiningServer = value; }

		const char* GetPlayerName() { return m_playerName; }

		Kyber::SocketManager* GetSocketManager() { return m_socketManager; }

	private:
		Kyber::SocketManager* m_socketManager;
		const char* m_playerName;
		void* m_fbClientInstance;
		fb::ClientState m_clientState;
		bool m_joiningServer;

		friend class Program;
	};
}