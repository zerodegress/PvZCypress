#pragma once
#include <Core/Client.h>
#include <Core/Server.h>
#include <Core/Settings.h>
#include <fb/Engine/Server.h>
#include <IGameModule.h>

namespace Cypress
{
	class Program
	{
	public:
		Program(HMODULE inModule);
		~Program();

		void InitConfig();
		bool InitWSA();

#if(HAS_DEDICATED_SERVER)
		Server* GetServer() { return m_server; }
#endif
		Client* GetClient() { return m_client; }
		IGameModule* GetGameModule() { return m_gameModule; }

		bool GetInitialized() { return m_initialized; }
		void SetInitialized(bool value) { m_initialized = value; }

		bool IsClient() { return m_client != nullptr; }
#if(HAS_DEDICATED_SERVER)
		bool IsServer() { return m_server != nullptr && !m_client; }
#else
		bool IsServer() { return false; }
#endif
		bool AllowMultipleInstances() { return m_allowMultipleInstances; }
		bool IsHeadless() { return m_headlessMode; }

	private:
		HMODULE m_hModule;
		Client* m_client;
#if(HAS_DEDICATED_SERVER)
		Server* m_server;
#endif
		IGameModule* m_gameModule;
		bool m_initialized;
		bool m_wsaStartupInitialized;
		bool m_allowMultipleInstances;
		bool m_consoleEnabled;
		bool m_headlessMode;
		bool m_logFileEnabled;
	};
} // namespace Cypress

extern Cypress::Program* g_program;