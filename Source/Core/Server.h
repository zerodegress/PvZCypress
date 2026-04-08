#pragma once
#include <Core/Config.h>
#include <Kyber/SocketManager.h>
#include <ServerBanlist.h>
#include <ServerPlaylist.h>
#include <cstdint>
#include <mutex>
#include <string>

#include <fb/Engine/ConsoleContext.h>
#include <fb/Engine/LevelSetup.h>
#include <fb/Engine/Server.h>

#if(HAS_DEDICATED_SERVER)
namespace Cypress
{
	class Server
	{
	public:
		struct ApiStatusSnapshot
		{
			bool Running;
			unsigned int UptimeSec;
			int Fps;
			std::string PlayerCount;
			int GhostCount;
			size_t MemoryMb;
			std::string Level;
			std::string GameMode;
			std::string HostedMode;
			std::string Tod;
			std::string Platform;
			std::string StatusLine1;
			std::string StatusLine2;
		};

		Server();
		~Server();

		void UpdateStatus(void* fbServerInstance, float deltaTime);

		HWND* GetMainWindow() { return m_mainWindow; }
		HWND* GetCommandBox() { return m_commandBox; }
		HWND* GetToggleLogButtonBox() { return m_toggleLogButtonBox; }

		bool GetRunning() { return m_running; }
		void SetRunning(bool running) { m_running = running; }
		
		bool GetStatusUpdated() { return m_statusUpdated; }
		void SetStatusUpdated(bool statusUpdated) { m_statusUpdated = statusUpdated; }

		bool GetServerLogEnabled() { return true; }
		void SetServerLogEnabled(bool value)
		{
			m_serverLogEnabled = value;
#ifdef CYPRESS_GW1
#else
			*(bool*)(OFFSET_M_LOGOUTPUTENABLED) = value;
#endif
		}

		bool IsUsingPlaylist() { return m_usingPlaylist; }

		bool GetIsLoadRequestFromLevelControl() { return m_loadRequestFromLevelControl; }
		void SetLoadRequestFromLevelControl(bool value) { m_loadRequestFromLevelControl = value; }

		void* GetFbServerInstance() { return m_fbServerInstance; }
		Kyber::SocketManager* GetSocketManager() { return m_socketManager; }
		size_t GetMemoryUsage();
		unsigned int GetSystemTime();
		ApiStatusSnapshot GetApiStatusSnapshot();
		std::string& GetStatusColumn1() { return m_statusCol1; }
		void SetStatusColumn1(std::string newStatus) { m_statusCol1 = newStatus; }
		std::string& GetStatusColumn2() { return m_statusCol2; }
		void SetStatusColumn2(std::string newStatus) { m_statusCol2 = newStatus; }

		ServerBanlist* GetServerBanlist() { return &m_banlist; }
		ServerPlaylist* GetServerPlaylist() { return &m_playlist; }

		void LoadPlaylistSetup(const PlaylistLevelSetup* nextSetup);
		void LevelSetupFromPlaylistSetup(fb::LevelSetup* setup, const PlaylistLevelSetup* playlistSetup);
		void ApplySettingsFromPlaylistSetup(const PlaylistLevelSetup* playlistSetup);

		static void InitDedicatedServer(void* thisPtr);

		// Commands
		static void ServerRestartLevel(fb::ConsoleContext& cc);
		static void ServerLoadLevel(fb::ConsoleContext& cc);
		static void ServerKickPlayer(fb::ConsoleContext& cc);
		static void ServerKickPlayerById(fb::ConsoleContext& cc);
		static void ServerBanPlayer(fb::ConsoleContext& cc);
		static void ServerBanPlayerById(fb::ConsoleContext& cc);
		static void ServerUnbanPlayer(fb::ConsoleContext& cc);
		static void ServerSay(fb::ConsoleContext& cc);
		static void ServerSayToPlayer(fb::ConsoleContext& cc);
		static void ServerLoadNextPlaylistSetup(fb::ConsoleContext& cc);

	private:
		Kyber::SocketManager* m_socketManager;
		void* m_fbServerInstance;
		HWND* m_mainWindow;
		HWND* m_commandBox;
		HWND* m_toggleLogButtonBox;
		bool m_running;
		bool m_statusUpdated;
		bool m_serverLogEnabled;
		bool m_usingPlaylist;
		bool m_loadRequestFromLevelControl;
		std::string m_statusCol1;
		std::string m_statusCol2;
		ServerBanlist m_banlist;
		ServerPlaylist m_playlist;
		std::mutex m_apiStatusMutex;
		ApiStatusSnapshot m_apiStatusSnapshot;

		friend class Program;

		void UpdateApiStatusSnapshot(
			unsigned int sec,
			int fps,
			const std::string& playerCountStr,
			int ghostCount,
			size_t memoryMb,
			const char* levelName,
			const char* gameMode,
			const char* hostedMode,
			const char* timeOfDay,
			const char* platformName);
	};
}
#endif
