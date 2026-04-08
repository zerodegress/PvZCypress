#include "pch.h"
#include "Psapi.h"
#include "Server.h"
#include <string>
#include <sstream>
#include <array>
#include <vector>
#define _WINSOCKAPI_
#include <ws2tcpip.h>
#include <Core/Program.h>
#include <Core/Settings.h>
#include <fb/Engine/LevelSetup.h>
#include <fb/Engine/Server.h>
#include <fb/Main.h>
#include <fb/Engine/String.h>
#include <fb/Engine/ServerPlayerManager.h>
#include <fb/Engine/ServerPeer.h>
#include <fb/Engine/ExecutionContext.h>
#include <Core/Console/ConsoleFunctions.h>
#include <fb/Engine/ServerGameContext.h>
#include <fb/TypeInfo/NetworkSettings.h>
#include <fb/TypeInfo/SystemSettings.h>
#include <fb/TypeInfo/GameSettings.h>

using namespace fb;

#if(HAS_DEDICATED_SERVER)
namespace
{
	using ServerConsoleFn = void(*)(fb::ConsoleContext& cc);

	struct CommandMapEntry
	{
		const char* name;
		ServerConsoleFn fn;
	};

	static const CommandMapEntry kServerCommandMap[] =
	{
		{ "RestartLevel", &Cypress::Server::ServerRestartLevel },
		{ "LoadLevel", &Cypress::Server::ServerLoadLevel },
		{ "KickPlayer", &Cypress::Server::ServerKickPlayer },
		{ "KickPlayerById", &Cypress::Server::ServerKickPlayerById },
		{ "BanPlayer", &Cypress::Server::ServerBanPlayer },
		{ "BanPlayerById", &Cypress::Server::ServerBanPlayerById },
		{ "Say", &Cypress::Server::ServerSay },
#ifdef CYPRESS_GW2
		{ "SayToPlayer", &Cypress::Server::ServerSayToPlayer },
#endif
		{ "LoadNextPlaylistSetup", &Cypress::Server::ServerLoadNextPlaylistSetup },
		{ "UnbanPlayer", &Cypress::Server::ServerUnbanPlayer }
	};

	static std::string TrimString(std::string value)
	{
		size_t start = value.find_first_not_of(" \t\r\n");
		if (start == std::string::npos)
			return "";
		size_t end = value.find_last_not_of(" \t\r\n");
		return value.substr(start, end - start + 1);
	}

	static std::string EscapeJsonString(const std::string& value)
	{
		std::string escaped;
		escaped.reserve(value.size() + 16);
		for (unsigned char c : value)
		{
			switch (c)
			{
			case '\\': escaped += "\\\\"; break;
			case '"': escaped += "\\\""; break;
			case '\n': escaped += "\\n"; break;
			case '\r': escaped += "\\r"; break;
			case '\t': escaped += "\\t"; break;
			default:
				if (c < 0x20)
				{
					escaped += '?';
				}
				else
				{
					escaped += (char)c;
				}
				break;
			}
		}
		return escaped;
	}

	struct TcpStatusSnapshot
	{
		bool Running = false;
		unsigned int UptimeSec = 0;
		int Fps = 0;
		std::string PlayerCount;
		int GhostCount = 0;
		size_t MemoryMb = 0;
		std::string Level;
		std::string GameMode;
		std::string HostedMode;
		std::string Tod;
		std::string Platform;
		std::string StatusLine1;
		std::string StatusLine2;
	};

	struct TcpClient
	{
		SOCKET Socket = INVALID_SOCKET;
		std::string Buffer;
		unsigned int ConnectedAtMs = 0;
	};

	SOCKET g_tcpListenSocket = INVALID_SOCKET;
	std::vector<TcpClient> g_tcpClients;
	TcpStatusSnapshot g_tcpStatus;

	void CloseClientByIndex(size_t index)
	{
		if (index >= g_tcpClients.size())
			return;
		if (g_tcpClients[index].Socket != INVALID_SOCKET)
			closesocket(g_tcpClients[index].Socket);
		g_tcpClients.erase(g_tcpClients.begin() + index);
	}

	std::string BuildStatusJson()
	{
		return std::format(
			"{{\"ok\":true,\"running\":{},\"uptimeSec\":{},\"fps\":{},\"playerCount\":\"{}\",\"ghostCount\":{},\"memoryMb\":{},\"level\":\"{}\",\"gameMode\":\"{}\",\"hostedMode\":\"{}\",\"tod\":\"{}\",\"platform\":\"{}\",\"statusLine1\":\"{}\",\"statusLine2\":\"{}\"}}",
			g_tcpStatus.Running ? "true" : "false",
			g_tcpStatus.UptimeSec,
			g_tcpStatus.Fps,
			EscapeJsonString(g_tcpStatus.PlayerCount),
			g_tcpStatus.GhostCount,
			g_tcpStatus.MemoryMb,
			EscapeJsonString(g_tcpStatus.Level),
			EscapeJsonString(g_tcpStatus.GameMode),
			EscapeJsonString(g_tcpStatus.HostedMode),
			EscapeJsonString(g_tcpStatus.Tod),
			EscapeJsonString(g_tcpStatus.Platform),
			EscapeJsonString(g_tcpStatus.StatusLine1),
			EscapeJsonString(g_tcpStatus.StatusLine2)
		);
	}

	void SendLineAndClose(size_t clientIndex, const std::string& line)
	{
		if (clientIndex >= g_tcpClients.size())
			return;
		const std::string out = line + "\n";
		send(g_tcpClients[clientIndex].Socket, out.c_str(), (int)out.size(), 0);
		CloseClientByIndex(clientIndex);
	}

	bool StartTcpApi(const char* bindAddress, int port)
	{
		if (g_tcpListenSocket != INVALID_SOCKET)
			return true;

		SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (listenSocket == INVALID_SOCKET)
		{
			CYPRESS_LOGMESSAGE(LogLevel::Error, "TCP API socket create failed ({})", WSAGetLastError());
			return false;
		}

		sockaddr_in addr{};
		addr.sin_family = AF_INET;
		addr.sin_port = htons((u_short)port);
		const char* bind = bindAddress ? bindAddress : "127.0.0.1";
		if (inet_pton(AF_INET, bind, &addr.sin_addr) != 1)
		{
			CYPRESS_LOGMESSAGE(LogLevel::Error, "Invalid TCP API bind address '{}'", bind);
			closesocket(listenSocket);
			return false;
		}

		u_long nonBlocking = 1;
		ioctlsocket(listenSocket, FIONBIO, &nonBlocking);

		int opt = 1;
		setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
		if (bind(listenSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
		{
			CYPRESS_LOGMESSAGE(LogLevel::Error, "TCP API bind failed on {}:{} ({})", bind, port, WSAGetLastError());
			closesocket(listenSocket);
			return false;
		}

		if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
		{
			CYPRESS_LOGMESSAGE(LogLevel::Error, "TCP API listen failed ({})", WSAGetLastError());
			closesocket(listenSocket);
			return false;
		}

		g_tcpListenSocket = listenSocket;
		CYPRESS_LOGMESSAGE(LogLevel::Info, "TCP API listening on {}:{}", bind, port);
		return true;
	}

	void StopTcpApi()
	{
		for (size_t i = 0; i < g_tcpClients.size(); ++i)
		{
			if (g_tcpClients[i].Socket != INVALID_SOCKET)
				closesocket(g_tcpClients[i].Socket);
		}
		g_tcpClients.clear();
		if (g_tcpListenSocket != INVALID_SOCKET)
		{
			closesocket(g_tcpListenSocket);
			g_tcpListenSocket = INVALID_SOCKET;
		}
	}

	void PollTcpApi(Cypress::Server* server)
	{
		if (g_tcpListenSocket == INVALID_SOCKET || !server)
			return;

		while (true)
		{
			sockaddr_in clientAddr{};
			int clientAddrLen = sizeof(clientAddr);
			SOCKET clientSocket = accept(g_tcpListenSocket, (sockaddr*)&clientAddr, &clientAddrLen);
			if (clientSocket == INVALID_SOCKET)
			{
				int err = WSAGetLastError();
				if (err != WSAEWOULDBLOCK)
					CYPRESS_LOGMESSAGE(LogLevel::Warning, "TCP API accept failed ({})", err);
				break;
			}

			u_long nonBlocking = 1;
			ioctlsocket(clientSocket, FIONBIO, &nonBlocking);
			g_tcpClients.push_back({ clientSocket, "", server->GetSystemTime() });
		}

		for (size_t i = 0; i < g_tcpClients.size();)
		{
			char buf[1024];
			bool closed = false;
			while (true)
			{
				int received = recv(g_tcpClients[i].Socket, buf, sizeof(buf), 0);
				if (received > 0)
				{
					g_tcpClients[i].Buffer.append(buf, received);
					if (g_tcpClients[i].Buffer.size() > 4096)
					{
						CloseClientByIndex(i);
						closed = true;
						break;
					}
					continue;
				}
				if (received == 0)
				{
					CloseClientByIndex(i);
					closed = true;
					break;
				}
				int err = WSAGetLastError();
				if (err != WSAEWOULDBLOCK)
				{
					CloseClientByIndex(i);
					closed = true;
				}
				break;
			}
			if (closed)
				continue;

			size_t newline = g_tcpClients[i].Buffer.find('\n');
			if (newline != std::string::npos)
			{
				std::string line = g_tcpClients[i].Buffer.substr(0, newline);
				if (!line.empty() && line.back() == '\r')
					line.pop_back();
				line = TrimString(line);

				if (line == "PING")
				{
					SendLineAndClose(i, "PONG");
					continue;
				}
				if (line == "STATUS")
				{
					SendLineAndClose(i, BuildStatusJson());
					continue;
				}
				if (line.rfind("CMD ", 0) == 0)
				{
					const std::string cmd = TrimString(line.substr(4));
					const bool ok = server->ExecuteServerCommandLine(cmd);
					SendLineAndClose(i, std::format("{{\"ok\":{},\"cmd\":\"{}\"}}", ok ? "true" : "false", EscapeJsonString(cmd)));
					continue;
				}

				SendLineAndClose(i, "{\"ok\":false,\"error\":\"Unknown command\"}");
				continue;
			}

			unsigned int now = server->GetSystemTime();
			if (now - g_tcpClients[i].ConnectedAtMs > 5000)
			{
				CloseClientByIndex(i);
				continue;
			}
			++i;
		}
	}
}

namespace Cypress
{
	void Server::ServerRestartLevel(fb::ConsoleContext& cc)
	{ 
		//fb::PVZServerLevelManager::restartLevel();
		reinterpret_cast<void(__fastcall*)()>(CYPRESS_GW_SELECT(0x14078EDA0, 0x140674180))();
	}

	void Server::ServerLoadLevel(fb::ConsoleContext& cc)
	{
		auto stream = cc.stream();
		std::string levelName;
		std::string inclusionOptions;
		std::string loadScreenGameMode;
		std::string loadScreenLevelName;
		std::string loadScreenLevelDescription;
		std::string loadScreenUIAssetPath;

		stream >> levelName >> inclusionOptions >> loadScreenGameMode >> loadScreenLevelName >> loadScreenLevelDescription >> loadScreenUIAssetPath;

		if (inclusionOptions.find("GameMode=") == std::string::npos)
		{
			cc.push("InclusionOptions must contain GameMode, syntax is \'GameMode=GameModeName\'");
			return;
		}

		if (inclusionOptions.find("TOD=") == std::string::npos)
		{
			cc.push("TOD InclusionOption not set, defaulting to Day");

			if (!inclusionOptions.ends_with(";"))
				inclusionOptions += ";";

			inclusionOptions += "TOD=Day";
		}

		if (inclusionOptions.find("HostedMode=") == std::string::npos)
		{
			cc.push("HostedMode InclusionOption not set, defaulting to ServerHosted");

			if (!inclusionOptions.ends_with(";"))
				inclusionOptions += ";";

			inclusionOptions += "HostedMode=ServerHosted";
		}

		LevelSetup setup;
		setup.m_name = levelName;

#ifdef CYPRESS_GW2
		if (!levelName.starts_with("Levels/"))
		{
			setup.m_name = std::format("Levels/{}/{}", levelName, levelName);
		}
#endif

		setup.setInclusionOptions(inclusionOptions.c_str());

#ifdef CYPRESS_GW2
		if (!loadScreenGameMode.empty())
			setup.LoadScreen_GameMode = loadScreenGameMode;

		if (!loadScreenLevelName.empty())
			setup.LoadScreen_LevelName = loadScreenLevelName;

		if (!loadScreenLevelDescription.empty())
			setup.LoadScreen_LevelDescription = loadScreenLevelDescription;
		
		if (!loadScreenUIAssetPath.empty())
			setup.LoadScreen_UIAssetPath = loadScreenUIAssetPath;

#endif

		fb::PostServerLoadLevelMessage(&setup, true, false);
		cc.push("Loading {} {}", levelName, inclusionOptions);
	}

	void Server::ServerLoadNextPlaylistSetup(fb::ConsoleContext& cc)
	{
		if (!g_program->GetServer()->IsUsingPlaylist())
		{
			cc.push("Server is not using a playlist.");
			return;
		}

		const PlaylistLevelSetup* nextSetup = g_program->GetServer()->GetServerPlaylist()->GetNextSetup();
		g_program->GetServer()->LoadPlaylistSetup(nextSetup);
	}

	void Server::ServerKickPlayer(fb::ConsoleContext& cc)
	{
		auto stream = cc.stream();
		std::string playerName;
		std::string reason;

		stream >> playerName >> reason;

		ServerGameContext* gameContext = ServerGameContext::GetInstance();
		if (!gameContext) return;
		if (!gameContext->m_serverPlayerManager) return;

		ServerPlayer* player = gameContext->m_serverPlayerManager->findHumanByName(playerName.c_str());
		if (!player)
		{
			cc.push("Player {} not found!", playerName.c_str());
			return;
		}
		if (player->isAIOrPersistentAIPlayer())
		{
			cc.push("Player {} is an AI!", playerName.c_str());
			return;
		}

		eastl::string eastlReason = "Kicked by Admin";
		if (!reason.empty())
		{
			eastlReason = reason.c_str();
		}
		cc.push("Kicked {} ({})", player->m_name, eastlReason.c_str());
		player->disconnect(SecureReason_KickedOut, eastlReason);
	}

	void Server::ServerKickPlayerById(fb::ConsoleContext& cc)
	{
		auto stream = cc.stream();
		int playerIndex;
		std::string reason;

		stream >> playerIndex >> reason;

		ServerGameContext* gameContext = ServerGameContext::GetInstance();
		if (!gameContext) return;
		if (!gameContext->m_serverPlayerManager) return;

		ServerPlayer* player = gameContext->m_serverPlayerManager->getById(playerIndex);
		if (!player)
		{
			cc.push("No player found at index {}", playerIndex);
			return;
		}
		if (player->isAIOrPersistentAIPlayer())
		{
			cc.push("Player {} (index {}) is an AI!", player->m_name, playerIndex);
			return;
		}

		eastl::string eastlReason = "Kicked by Admin";
		if (!reason.empty())
		{
			eastlReason = reason.c_str();
		}
		cc.push("Kicked {} ({})", player->m_name, eastlReason.c_str());
		player->disconnect(SecureReason_KickedOut, eastlReason);
	}

	void Server::ServerBanPlayer(fb::ConsoleContext& cc)
	{
		auto stream = cc.stream();
		std::string playerName;
		std::string reason;

		stream >> playerName >> reason;

		ServerGameContext* gameContext = ServerGameContext::GetInstance();
		if (!gameContext) return;
		if (!gameContext->m_serverPlayerManager) return;

		ServerPlayer* player = gameContext->m_serverPlayerManager->findHumanByName(playerName.c_str());
		if (!player)
		{
			cc.push("Player {} not found!", playerName.c_str());
			return;
		}
		if (player->isAIOrPersistentAIPlayer())
		{
			cc.push("Player {} is an AI!", playerName.c_str());
			return;
		}

		ServerConnection* connection = gameContext->m_serverPeer->connectionForPlayer(player);
		if (!connection) return;

		eastl::string reasonText = "The Ban Hammer has spoken!";
		if (!reason.empty())
		{
			reasonText = reason.c_str();
		}

		g_program->GetServer()->GetServerBanlist()->AddToList(player->m_name, connection->m_machineId.c_str(), reasonText.c_str());
		gameContext->m_serverPeer->m_bannedMachines.push_back(connection->m_machineId);
		gameContext->m_serverPeer->m_bannedPlayers.push_back(player->m_name);
		player->disconnect(SecureReason_Banned, reasonText);
	}

	void Server::ServerBanPlayerById(fb::ConsoleContext& cc)
	{
		auto stream = cc.stream();
		int playerIndex;
		std::string reason;

		stream >> playerIndex >> reason;

		ServerGameContext* gameContext = ServerGameContext::GetInstance();
		if (!gameContext) return;
		if (!gameContext->m_serverPlayerManager) return;

		ServerPlayer* player = gameContext->m_serverPlayerManager->getById(playerIndex);
		if (!player)
		{
			cc.push("No player found at index {}", playerIndex);
			return;
		}
		if (player->isAIOrPersistentAIPlayer())
		{
			cc.push("Player {} (index {}) is an AI!", player->m_name, playerIndex);
			return;
		}

		ServerConnection* connection = gameContext->m_serverPeer->connectionForPlayer(player);
		if (!connection) return;

		eastl::string reasonText = "Banned by server admin";
		if (!reason.empty())
		{
			reasonText = reason.c_str();
		}
		g_program->GetServer()->GetServerBanlist()->AddToList(player->m_name, connection->m_machineId.c_str(), reasonText.c_str());
		gameContext->m_serverPeer->m_bannedMachines.push_back(connection->m_machineId);
		gameContext->m_serverPeer->m_bannedPlayers.push_back(player->m_name);
		player->disconnect(SecureReason_Banned, reasonText);
	}

	void Server::ServerUnbanPlayer(fb::ConsoleContext& cc)
	{
		auto stream = cc.stream();
		std::string playerName;

		stream >> playerName;

		ServerGameContext* gameContext = ServerGameContext::GetInstance();
		if (!gameContext) return;

		if (!g_program->GetServer()->GetServerBanlist()->IsBanned(playerName.c_str()))
		{
			cc.push("Player {} is not banned", playerName);
			return;
		}

		auto entry = g_program->GetServer()->GetServerBanlist()->GetPlayerEntry(playerName.c_str());
		gameContext->m_serverPeer->removeBannedMachine(entry.MachineId.c_str());
		gameContext->m_serverPeer->removeBannedPlayer(entry.Name.c_str());
		g_program->GetServer()->GetServerBanlist()->RemoveFromList(playerName.c_str());

		cc.push("Player {} has been unbanned", playerName);
	}

	void Server::ServerSay(fb::ConsoleContext& cc)
	{
		auto stream = cc.stream();
		std::string message;
		float duration;

		stream >> message >> duration;

		if (message.empty())
		{
			cc.push("Wrong parameters, missing message");
			return;
		}

#ifdef CYPRESS_GW1
		auto func = reinterpret_cast<void* (*)(__int64 arena)>(0x141508040);
		void* msg = func(0x141E7F750);
#elif defined(CYPRESS_GW2)
		auto func = reinterpret_cast<void* (*)(__int64 arena, int localPlayerId)>(0x141FCEE70);
		void* msg = func(0x1429386E0, 0);
#endif
		

		fb::String msgText = message.c_str();
		float messageDuration = std::clamp(duration, 1.0f, 10.0f);

		ptrset<fb::String>(msg, 0x48, msgText);
		ptrset<float>(msg, 0x50, messageDuration);

		ServerGameContext::GetInstance()->m_serverPeer->sendMessage(msg, nullptr);
	}

	void Server::ServerSayToPlayer(fb::ConsoleContext& cc)
	{
		auto stream = cc.stream();
		std::string playerName;
		std::string message;
		float duration;

		stream >> playerName >> message >> duration;

		ServerGameContext* gameContext = ServerGameContext::GetInstance();
		if (!gameContext) return;
		if (!gameContext->m_serverPlayerManager) return;

		ServerPlayer* player = gameContext->m_serverPlayerManager->findHumanByName(playerName.c_str());
		if (!player)
		{
			cc.push("Player {} not found!", playerName.c_str());
			return;
		}

		fb::ServerConnection* connection = gameContext->m_serverPeer->connectionForPlayer(player);
		if (!connection)
		{
			cc.push("Connection for player {} not found!", playerName.c_str());
			return;
		}

#ifdef CYPRESS_GW1
		auto func = reinterpret_cast<void* (*)(__int64 arena)>(0x141508040);
		void* msg = func(0x141E7F750);
#elif defined(CYPRESS_GW2)
		auto func = reinterpret_cast<void* (*)(__int64 arena, int localPlayerId)>(0x141FCEE70);
		void* msg = func(0x1429386E0, 0);
#endif

		fb::String msgText = message.c_str();
		float messageDuration = std::clamp(duration, 1.0f, 10.0f);

		ptrset<fb::String>(msg, 0x48, msgText);
		ptrset<float>(msg, 0x50, messageDuration);

		connection->sendMessage(msg);
	}

	Server::Server()
		: m_socketManager(new Kyber::SocketManager(Kyber::ProtocolDirection::Clientbound, Kyber::SocketSpawnInfo(false, "", "")))
		, m_fbServerInstance(nullptr)
		, m_mainWindow((HWND*)(OFFSET_MAINWND))
		, m_commandBox((HWND*)(OFFSET_COMMANDBOX))
		, m_toggleLogButtonBox((HWND*)(OFFSET_TOGGLELOGBUTTONBOX))
		, m_running(false)
		, m_statusUpdated(false)
		, m_serverLogEnabled(false)
		, m_usingPlaylist(false)
		, m_loadRequestFromLevelControl(false)
		, m_statusCol1()
		, m_statusCol2()
		, m_banlist()
		, m_playlist()
	{
	}

	Server::~Server()
	{
		StopTcpApi();
	}

	bool Server::ExecuteServerCommandLine(const std::string& commandLine)
	{
		const std::string trimmed = TrimString(commandLine);
		if (trimmed.empty())
			return false;

		size_t whitespacePos = trimmed.find_first_of(" \t");
		std::string commandName = whitespacePos == std::string::npos ? trimmed : trimmed.substr(0, whitespacePos);
		std::string commandArgs = whitespacePos == std::string::npos ? "" : trimmed.substr(whitespacePos + 1);

		const char* methodName = commandName.c_str();
		constexpr const char* groupName = "Server";
		if (commandName.rfind("Server.", 0) == 0)
		{
			methodName += 7;
		}

		for (const CommandMapEntry& entry : kServerCommandMap)
		{
			if (strcmp(methodName, entry.name) != 0)
				continue;

			fb::ConsoleContext cc{};
			cc.m_args = commandArgs.empty() ? const_cast<char*>("") : commandArgs.data();
			cc.m_groupName = const_cast<char*>(groupName);
			cc.m_method = const_cast<char*>(entry.name);
			entry.fn(cc);
			return true;
		}

		CYPRESS_LOGTOSERVER(LogLevel::Warning, "Unknown command: {}", trimmed.c_str());
		return false;
	}

	void Server::UpdateStatus(void* fbServerInstance, float deltaTime)
	{
		static unsigned int startSystemTime = GetSystemTime();
		unsigned int sec = (GetSystemTime() - startSystemTime) / 1000;
		unsigned int min = (sec / 60) % 60;
		unsigned int hour = (sec / (60 * 60));

		static unsigned int lastSec = 0;
		static float currentDeltaTime = 0;
		static float sumDeltaTime = 0;
		static unsigned int frameCount = 0;

		m_fbServerInstance = fbServerInstance;

		sumDeltaTime += deltaTime;
		++frameCount;

		if (lastSec != sec)
		{
			lastSec = sec;
			currentDeltaTime = sumDeltaTime / float(frameCount);
			sumDeltaTime = 0;
			frameCount = 0;
		}
			
		fb::ServerPlayerManager* playerMgr = ptrread<fb::ServerPlayerManager*>(fbServerInstance, CYPRESS_GW_SELECT(0x98, 0xA0));
		fb::ServerPeer* serverPeer = ptrread<fb::ServerPeer*>(fbServerInstance, CYPRESS_GW_SELECT(0x90, 0x98));
		
		fb::ServerGhostManager* ghostMgr = serverPeer->GetGhostManager();
		int ghostcount = ghostMgr->ghostCount();
		
		fb::SettingsManager* settingsManager = fb::SettingsManager::GetInstance();

		// +13 to cut off GamePlatform_
		const char* platformName = fb::toString(settingsManager->getContainer<fb::SystemSettings>("Game")->Platform) + 13;

		unsigned int maxPlayerCount = settingsManager->getContainer<fb::NetworkSettings>("Network")->MaxClientCount;
		unsigned int maxSpectatorCount = settingsManager->getContainer<fb::GameSettings>("Game")->MaxSpectatorCount;

		if (serverPeer)
			maxPlayerCount = std::min(maxPlayerCount, serverPeer->maxClientCount());

		std::string playerCountStr = std::format("{}/{} ({}/{}) [{}]",
			playerMgr->humanPlayerCount(),
			maxPlayerCount - maxSpectatorCount,
			playerMgr->spectatorCount(),
			maxSpectatorCount,
			maxPlayerCount);
		
		//not used, but might be useful i guess
		//std::string aiPlayerCountStr = std::format("{}/{} [{}]", playerMgr->aiPlayerCount(), 64, 64 - (playerMgr->humanPlayerCount() + playerMgr->aiPlayerCount()));
		
		g_program->GetServer()->SetStatusColumn1(
			std::format(
			"FPS: {} \t\t\t\t"
			"UpTime: {}:{}:{} \t\t\t"
			"PlayerCount: {} \t\t\t"
			"GhostCount: {} \t\t\t"
			"Memory (CPU): {} MB",
			int(1.0f / currentDeltaTime),
			hour,
			min,
			sec % 60,
			playerCountStr,
			ghostcount,
			GetMemoryUsage()
		));

		//Server restarting the level after the last player in it leaves. Might need for some instances, but is absolutely needed for GW2 because of the following below
		//If the last player in a GW2 server leaves, anyone who joins afterwards will experience issues where, whether it's by taunting, or using specific abilities, some inputs will lock up. The only workaround so far is that the level is reloaded in someway, whether through restarting or changing it.
		static int prevplayercount = playerMgr->humanPlayerCount();
		int curplayercount = playerMgr->humanPlayerCount();

		if (prevplayercount > 0 && curplayercount == 0)
		{
			//RestartLevel
			reinterpret_cast<void(__fastcall*)()>(CYPRESS_GW_SELECT(0x14078EDA0, 0x140674180))();
		}

		prevplayercount = curplayercount;
		
		fb::LevelSetup setup = ptrread<fb::LevelSetup>(fbServerInstance, CYPRESS_GW_SELECT(0x40, 0x30));
		if (setup.m_name.length() > 0)
		{
			//creating a server without any of these set will break the status columns, so we need to make sure they are set to something
			const char* levelName    = extractFileName(setup.m_name.c_str());
			const char* gameMode     = strlen(setup.getInclusionOption("GameMode")) > 1 ? setup.getInclusionOption("GameMode") : "\t";
			const char* hostedMode   = strlen(setup.getInclusionOption("HostedMode")) > 1 ? setup.getInclusionOption("HostedMode") : "\t";
			const char* timeOfDay    = strlen(setup.getInclusionOption("TOD")) > 1 ? setup.getInclusionOption("TOD") : "\t";

			g_program->GetServer()->SetStatusColumn2(
				std::format(
					"Level: {} \t\t"
					"GameMode: {} \t\t"
					"HostedMode: {} \t\t"
					"TOD: {}\t\t\t\t"
					"Platform: {}",
					levelName,
					gameMode,
					hostedMode,
					timeOfDay,
					platformName
				));
		}
		else
		{
			g_program->GetServer()->SetStatusColumn2("Level: No level");
		}
		
		static size_t tick = 0;
		size_t fps = int(1.0f / currentDeltaTime);
		if (tick != fps)
		{
			tick = fps;
			g_program->GetServer()->SetStatusUpdated(false);
		}

		g_tcpStatus.Running = GetRunning();
		g_tcpStatus.UptimeSec = sec;
		g_tcpStatus.Fps = (int)fps;
		g_tcpStatus.PlayerCount = playerCountStr;
		g_tcpStatus.GhostCount = ghostcount;
		g_tcpStatus.MemoryMb = GetMemoryUsage();
		g_tcpStatus.Level = setup.m_name.length() > 0 ? extractFileName(setup.m_name.c_str()) : "No level";
		{
			const char* gameModeOpt = setup.getInclusionOption("GameMode");
			const char* hostedModeOpt = setup.getInclusionOption("HostedMode");
			const char* todOpt = setup.getInclusionOption("TOD");
			g_tcpStatus.GameMode = gameModeOpt ? gameModeOpt : "";
			g_tcpStatus.HostedMode = hostedModeOpt ? hostedModeOpt : "";
			g_tcpStatus.Tod = todOpt ? todOpt : "";
		}
		g_tcpStatus.Platform = platformName ? platformName : "";
		g_tcpStatus.StatusLine1 = m_statusCol1;
		g_tcpStatus.StatusLine2 = m_statusCol2;
		PollTcpApi(this);
	}

	size_t Server::GetMemoryUsage()
	{
		PROCESS_MEMORY_COUNTERS_EX pmc;
		if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
			return pmc.WorkingSetSize / (1024 * 1024); //MegaBytes
		}
		return 0;
	}

	void Server::LoadPlaylistSetup(const PlaylistLevelSetup* nextSetup)
	{
		LevelSetup setup;
		LevelSetupFromPlaylistSetup(&setup, nextSetup);
		ApplySettingsFromPlaylistSetup(nextSetup);

		CYPRESS_LOGTOSERVER(LogLevel::Info, "Server is loading playlist setup ({} on {})", setup.m_name.c_str(), setup.getInclusionOption("GameMode"));
		fb::PostServerLoadLevelMessage(&setup, true, false);
	}

	void Server::LevelSetupFromPlaylistSetup(LevelSetup* setup, const PlaylistLevelSetup* playlistSetup)
	{
		setup->m_name = playlistSetup->LevelName.c_str();
		setup->setInlusionOption("GameMode", playlistSetup->GameMode.c_str());
		setup->setInlusionOption("HostedMode", "ServerHosted");
		setup->setInlusionOption("TOD", playlistSetup->TOD.c_str());
#ifdef CYPRESS_GW2
		if (!playlistSetup->Loadscreen_GamemodeName.empty())
			setup->LoadScreen_GameMode = playlistSetup->Loadscreen_GamemodeName.c_str();
		if (!playlistSetup->Loadscreen_LevelName.empty())
			setup->LoadScreen_LevelName = playlistSetup->Loadscreen_LevelName.c_str();
		if (!playlistSetup->Loadscreen_LevelDescription.empty())
			setup->LoadScreen_LevelDescription = playlistSetup->Loadscreen_LevelDescription.c_str();
		if (!playlistSetup->Loadscreen_UIAssetPath.empty())
		setup->LoadScreen_UIAssetPath = playlistSetup->Loadscreen_UIAssetPath.c_str();
#endif
	}

	void Server::ApplySettingsFromPlaylistSetup(const PlaylistLevelSetup* playlistSetup)
	{
		if (!playlistSetup->SettingsToApply.empty())
		{
			std::vector<std::string> settings = splitString(playlistSetup->SettingsToApply, '|');

			for (const auto& setting : settings)
			{
				std::vector<std::string> settingAndValue = splitString(setting, ' ');
				if (settingAndValue.size() != 2) continue;

				CYPRESS_LOGTOSERVER(LogLevel::Info, "Playlist is setting {} to {}", settingAndValue[0].c_str(), settingAndValue[1].c_str());
				if (settingAndValue[1].c_str()[0] == '^') // empty string
				{
					fb::SettingsManager::GetInstance()->set(settingAndValue[0].c_str(), "");
				}
				else
				{
					fb::SettingsManager::GetInstance()->set(settingAndValue[0].c_str(), settingAndValue[1].c_str());
				}
			}
		}
	}

	void Server::InitDedicatedServer(void* thisPtr)
	{
		g_program->GetServer()->SetRunning(true);

		auto fb_createWindow = reinterpret_cast<__int64(*)()>(CYPRESS_GW_SELECT(0x140008D90, 0x14012F7E0));

		if (!g_program->IsHeadless())
		{
			fb_createWindow();
		}

		LevelSetup initialLevelSetup;
		if (g_program->GetServer()->m_usingPlaylist)
		{
			CYPRESS_LOGTOSERVER(LogLevel::Info, "Loading first setup in playlist");
			const PlaylistLevelSetup* playlistSetup = g_program->GetServer()->m_playlist.GetSetup(0);

			if (g_program->GetServer()->m_playlist.IsMixedMode())
				playlistSetup = g_program->GetServer()->m_playlist.GetMixedLevelSetup(0);
			else
				playlistSetup = g_program->GetServer()->m_playlist.GetSetup(0);

			g_program->GetServer()->LevelSetupFromPlaylistSetup(&initialLevelSetup, playlistSetup);
			g_program->GetServer()->ApplySettingsFromPlaylistSetup(playlistSetup);
		}
		else
		{
			const char* initialLevel = fb::ExecutionContext::getOptionValue("level");
			const char* initialInclusion = fb::ExecutionContext::getOptionValue("inclusion");
			CYPRESS_ASSERT(initialLevel != nullptr, "Must provide a level name via the -level argument!");
			CYPRESS_ASSERT(initialInclusion != nullptr, "Must provide inclusion options via the -inclusion argument!");

			if (strstr(initialLevel, "Levels/") == 0)
			{
				// this is fine since Name is an fb::String, which copies the string its given
				initialLevelSetup.m_name = std::format("Levels/{}/{}", initialLevel, initialLevel);
			}
			else
			{
				initialLevelSetup.m_name = initialLevel;
			}
			initialLevelSetup.setInclusionOptions(initialInclusion);
		}

		ServerSpawnInfo* spawnInfo = new ServerSpawnInfo(&initialLevelSetup);

		auto fb_spawnServer = reinterpret_cast<void (*)(void* thisPtr, ServerSpawnInfo* info)>(OFFSET_FB_MAIN_SPAWNSERVER);
		fb_spawnServer(thisPtr, spawnInfo);

		g_program->GetGameModule()->RegisterCommands();

		bool enableTcpApi = fb::ExecutionContext::getOptionValue("enableApi") != nullptr
			|| fb::ExecutionContext::getOptionValue("apiPort") != nullptr;
		if (enableTcpApi)
		{
			const char* apiBind = fb::ExecutionContext::getOptionValue("apiBind", "127.0.0.1");
			const char* apiPortStr = fb::ExecutionContext::getOptionValue("apiPort", "8787");
			int apiPort = atoi(apiPortStr);
			if (apiPort <= 0 || apiPort > 65535)
			{
				CYPRESS_LOGMESSAGE(LogLevel::Warning, "Invalid -apiPort '{}', using 8787", apiPortStr);
				apiPort = 8787;
			}
			StartTcpApi(apiBind, apiPort);
		}

		g_program->GetServer()->m_banlist.LoadFromFile("bans.json");
		ServerPeer* peer = ServerGameContext::GetInstance()->m_serverPeer;
		for (const auto& player : g_program->GetServer()->m_banlist.GetBannedPlayers())
		{
			peer->m_bannedPlayers.push_back(player.Name.c_str());
			peer->m_bannedMachines.push_back(player.MachineId.c_str());
		}

		CYPRESS_LOGMESSAGE(LogLevel::Info, "Initialized Dedicated Server");
	}

	unsigned int Server::GetSystemTime()
	{
		static bool l_isInitialized = false;
		static LARGE_INTEGER    l_liPerformanceFrequency;
		static LARGE_INTEGER    l_liBaseTime;

		LARGE_INTEGER liEndTime;

		if (!l_isInitialized) {
			QueryPerformanceFrequency(&l_liPerformanceFrequency);
			QueryPerformanceCounter(&l_liBaseTime);
			l_isInitialized = true;

			return 0;
		}

		unsigned __int64 n;
		QueryPerformanceCounter(&liEndTime);
		n = liEndTime.QuadPart - l_liBaseTime.QuadPart;
		n = n * 1000 / l_liPerformanceFrequency.QuadPart;

		return (unsigned int)n;
	}
}
#endif
