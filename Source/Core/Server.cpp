#include "pch.h"
#include "Psapi.h"
#include "Server.h"
#include <string>
#include <sstream>
#include <algorithm>
#include <array>
#include <atomic>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>
#include <cctype>
#include <cstdlib>
#include <Core/Program.h>
#include <Core/Settings.h>
#include <Core/Logging.h>
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
#define _WINSOCKAPI_
#include <ws2tcpip.h>
#include <json.hpp>

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

	std::unique_ptr<class ApiControlServer> g_apiControlServer;

	bool TryExecuteServerCommandLineInternal(const std::string& commandLine)
	{
		size_t whitespacePos = commandLine.find_first_of(" \t");
		std::string commandName = whitespacePos == std::string::npos ? commandLine : commandLine.substr(0, whitespacePos);
		std::string commandArgs = whitespacePos == std::string::npos ? "" : commandLine.substr(whitespacePos + 1);

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

		return false;
	}

	static std::string TrimString(std::string value)
	{
		size_t start = value.find_first_not_of(" \t\r\n");
		if (start == std::string::npos)
			return "";
		size_t end = value.find_last_not_of(" \t\r\n");
		return value.substr(start, end - start + 1);
	}

	class ApiControlServer
	{
	public:
		ApiControlServer(std::string bindAddress, int port, std::string token)
			: m_bindAddress(std::move(bindAddress))
			, m_port(port)
			, m_token(std::move(token))
			, m_running(false)
			, m_listenSocket(INVALID_SOCKET)
		{
		}

		~ApiControlServer()
		{
			Stop();
		}

		void Start()
		{
			if (m_running.exchange(true))
				return;
			m_thread = std::thread(&ApiControlServer::Run, this);
		}

		void Stop()
		{
			if (!m_running.exchange(false))
				return;

			SOCKET listenSocket = m_listenSocket.exchange(INVALID_SOCKET);
			if (listenSocket != INVALID_SOCKET)
			{
				closesocket(listenSocket);
			}

			if (m_thread.joinable())
			{
				m_thread.join();
			}
		}

	private:
		static std::unordered_map<std::string, std::string> ParseHeaders(const std::string& headerSection)
		{
			std::unordered_map<std::string, std::string> headers;
			size_t start = 0;
			while (start < headerSection.size())
			{
				size_t end = headerSection.find("\r\n", start);
				if (end == std::string::npos)
					end = headerSection.size();
				std::string line = headerSection.substr(start, end - start);
				size_t colon = line.find(':');
				if (colon != std::string::npos)
				{
					std::string key = line.substr(0, colon);
					std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return (char)std::tolower(c); });
					std::string value = TrimString(line.substr(colon + 1));
					headers[key] = value;
				}
				start = end + 2;
			}
			return headers;
		}

		static std::string BuildHttpResponse(int status, const char* statusText, const std::string& body)
		{
			return std::format(
				"HTTP/1.1 {} {}\r\n"
				"Content-Type: application/json\r\n"
				"Connection: close\r\n"
				"Content-Length: {}\r\n\r\n{}",
				status,
				statusText,
				body.size(),
				body);
		}

		void SendJson(SOCKET clientSocket, int status, const char* statusText, const nlohmann::json& body)
		{
			const std::string payload = body.dump();
			const std::string response = BuildHttpResponse(status, statusText, payload);
			send(clientSocket, response.c_str(), (int)response.size(), 0);
		}

		bool IsAuthorized(const std::unordered_map<std::string, std::string>& headers)
		{
			if (m_token.empty())
				return true;

			auto it = headers.find("authorization");
			if (it == headers.end())
				return false;
			const std::string expected = std::format("Bearer {}", m_token);
			return it->second == expected;
		}

		static std::string GetPathOnly(const std::string& target)
		{
			size_t q = target.find('?');
			return q == std::string::npos ? target : target.substr(0, q);
		}

		static std::unordered_map<std::string, std::string> ParseQuery(const std::string& target)
		{
			std::unordered_map<std::string, std::string> query;
			size_t q = target.find('?');
			if (q == std::string::npos || q + 1 >= target.size())
				return query;
			std::string queryString = target.substr(q + 1);
			size_t start = 0;
			while (start < queryString.size())
			{
				size_t amp = queryString.find('&', start);
				if (amp == std::string::npos)
					amp = queryString.size();
				std::string part = queryString.substr(start, amp - start);
				size_t eq = part.find('=');
				if (eq != std::string::npos)
					query[part.substr(0, eq)] = part.substr(eq + 1);
				else
					query[part] = "";
				start = amp + 1;
			}
			return query;
		}

		void HandleClient(SOCKET clientSocket)
		{
			std::string request;
			std::array<char, 4096> buf{};

			while (request.find("\r\n\r\n") == std::string::npos)
			{
				int received = recv(clientSocket, buf.data(), (int)buf.size(), 0);
				if (received <= 0)
					return;
				request.append(buf.data(), received);
				if (request.size() > 1024 * 128)
					return;
			}

			size_t headerEnd = request.find("\r\n\r\n");
			std::string header = request.substr(0, headerEnd);
			std::string body = request.substr(headerEnd + 4);

			size_t lineEnd = header.find("\r\n");
			if (lineEnd == std::string::npos)
			{
				SendJson(clientSocket, 400, "Bad Request", { {"ok", false}, {"error", "Malformed request line"} });
				return;
			}

			std::string requestLine = header.substr(0, lineEnd);
			size_t m1 = requestLine.find(' ');
			size_t m2 = requestLine.find(' ', m1 + 1);
			if (m1 == std::string::npos || m2 == std::string::npos)
			{
				SendJson(clientSocket, 400, "Bad Request", { {"ok", false}, {"error", "Malformed request line"} });
				return;
			}

			std::string method = requestLine.substr(0, m1);
			std::string target = requestLine.substr(m1 + 1, m2 - m1 - 1);
			auto headers = ParseHeaders(header.substr(lineEnd + 2));

			size_t contentLength = 0;
			auto cl = headers.find("content-length");
			if (cl != headers.end())
			{
				contentLength = (size_t)strtoull(cl->second.c_str(), nullptr, 10);
			}

			while (body.size() < contentLength)
			{
				int received = recv(clientSocket, buf.data(), (int)buf.size(), 0);
				if (received <= 0)
					return;
				body.append(buf.data(), received);
			}

			if (!IsAuthorized(headers))
			{
				SendJson(clientSocket, 401, "Unauthorized", { {"ok", false}, {"error", "Unauthorized"} });
				return;
			}

			const std::string path = GetPathOnly(target);
			const auto query = ParseQuery(target);
			Cypress::Server* server = g_program->GetServer();

			if (method == "GET" && path == "/health")
			{
				SendJson(clientSocket, 200, "OK", { {"ok", true} });
				return;
			}

		if (method == "GET" && path == "/v1/status")
		{
			nlohmann::json result =
			{
				{"ok", true},
				{"running", server && server->GetRunning()},
				{"queuedCommands", server ? server->GetExternalCommandQueueSize() : 0},
				{"latestLogId", Cypress_GetLatestServerLogId()},
				{"latestEventId", Cypress_GetLatestServerEventId()}
			};
			SendJson(clientSocket, 200, "OK", result);
			return;
		}

			if (method == "GET" && path == "/v1/messages")
			{
				uint64_t since = 0;
				size_t limit = 100;

				auto sinceIt = query.find("since");
				if (sinceIt != query.end())
					since = strtoull(sinceIt->second.c_str(), nullptr, 10);
				auto limitIt = query.find("limit");
				if (limitIt != query.end())
					limit = (size_t)strtoull(limitIt->second.c_str(), nullptr, 10);

				limit = std::clamp(limit, (size_t)1, (size_t)500);
				auto logs = Cypress_GetServerLogsSince(since, limit);
				nlohmann::json messages = nlohmann::json::array();
				for (const auto& entry : logs)
				{
					messages.push_back({
						{"id", entry.first},
						{"text", entry.second}
						});
				}

			SendJson(clientSocket, 200, "OK", {
				{"ok", true},
				{"messages", messages},
				{"latestLogId", Cypress_GetLatestServerLogId()}
				});
			return;
		}

		if (method == "GET" && path == "/v1/events")
		{
			uint64_t since = 0;
			size_t limit = 100;
			std::string type;

			auto sinceIt = query.find("since");
			if (sinceIt != query.end())
				since = strtoull(sinceIt->second.c_str(), nullptr, 10);
			auto limitIt = query.find("limit");
			if (limitIt != query.end())
				limit = (size_t)strtoull(limitIt->second.c_str(), nullptr, 10);
			auto typeIt = query.find("type");
			if (typeIt != query.end())
				type = typeIt->second;

			limit = std::clamp(limit, (size_t)1, (size_t)500);
			auto events = Cypress_GetServerEventsSince(since, limit, type);
			nlohmann::json resultEvents = nlohmann::json::array();
			for (const auto& evt : events)
			{
				nlohmann::json payload = nlohmann::json::object();
				try
				{
					if (!evt.PayloadJson.empty())
						payload = nlohmann::json::parse(evt.PayloadJson);
				}
				catch (...)
				{
				}

				resultEvents.push_back({
					{"id", evt.Id},
					{"ts", evt.TimestampMs},
					{"type", evt.Type},
					{"source", evt.Source},
					{"payload", payload}
					});
			}

			SendJson(clientSocket, 200, "OK", {
				{"ok", true},
				{"events", resultEvents},
				{"latestEventId", Cypress_GetLatestServerEventId()}
				});
			return;
		}

			if (method == "POST" && path == "/v1/command")
			{
				std::string cmd;
				try
				{
					nlohmann::json input = nlohmann::json::parse(body);
					if (input.contains("cmd") && input["cmd"].is_string())
						cmd = input["cmd"].get<std::string>();
				}
				catch (...)
				{
					cmd.clear();
				}

				cmd = TrimString(cmd);
				if (cmd.empty())
				{
					SendJson(clientSocket, 400, "Bad Request", { {"ok", false}, {"error", "Missing cmd"} });
					return;
				}

				if (!server)
				{
					SendJson(clientSocket, 503, "Service Unavailable", { {"ok", false}, {"error", "Server not ready"} });
					return;
				}

		server->QueueExternalCommand(cmd);
		Cypress_PublishServerEvent("command.api_queued", "webapi", nlohmann::json({ {"cmd", cmd} }).dump());
		SendJson(clientSocket, 202, "Accepted", { {"ok", true}, {"queued", true}, {"cmd", cmd} });
		return;
	}

			SendJson(clientSocket, 404, "Not Found", { {"ok", false}, {"error", "Unknown endpoint"} });
		}

		void Run()
		{
			SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (listenSocket == INVALID_SOCKET)
			{
				CYPRESS_LOGMESSAGE(LogLevel::Error, "API server socket create failed ({})", WSAGetLastError());
				m_running = false;
				return;
			}

			sockaddr_in addr{};
			addr.sin_family = AF_INET;
			addr.sin_port = htons((u_short)m_port);
			if (inet_pton(AF_INET, m_bindAddress.c_str(), &addr.sin_addr) != 1)
			{
				CYPRESS_LOGMESSAGE(LogLevel::Error, "Invalid API bind address '{}'", m_bindAddress);
				closesocket(listenSocket);
				m_running = false;
				return;
			}

			int opt = 1;
			setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
			if (bind(listenSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
			{
				CYPRESS_LOGMESSAGE(LogLevel::Error, "API server bind failed on {}:{} ({})", m_bindAddress, m_port, WSAGetLastError());
				closesocket(listenSocket);
				m_running = false;
				return;
			}

			if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
			{
				CYPRESS_LOGMESSAGE(LogLevel::Error, "API server listen failed ({})", WSAGetLastError());
				closesocket(listenSocket);
				m_running = false;
				return;
			}

			m_listenSocket = listenSocket;
			CYPRESS_LOGMESSAGE(LogLevel::Info, "Control API listening on http://{}:{}", m_bindAddress, m_port);

			while (m_running)
			{
				sockaddr_in clientAddr{};
				int clientAddrLen = sizeof(clientAddr);
				SOCKET clientSocket = accept(listenSocket, (sockaddr*)&clientAddr, &clientAddrLen);
				if (clientSocket == INVALID_SOCKET)
				{
					if (m_running)
						CYPRESS_LOGMESSAGE(LogLevel::Warning, "API accept failed ({})", WSAGetLastError());
					break;
				}

				HandleClient(clientSocket);
				closesocket(clientSocket);
			}

			SOCKET old = m_listenSocket.exchange(INVALID_SOCKET);
			if (old != INVALID_SOCKET)
				closesocket(old);
			CYPRESS_LOGMESSAGE(LogLevel::Info, "Control API stopped");
			m_running = false;
		}

		std::string m_bindAddress;
		int m_port;
		std::string m_token;
		std::atomic<bool> m_running;
		std::atomic<SOCKET> m_listenSocket;
		std::thread m_thread;
	};
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
		if (g_apiControlServer)
		{
			g_apiControlServer->Stop();
			g_apiControlServer.reset();
		}
	}

	bool Server::ExecuteServerCommandLine(const std::string& commandLine)
	{
		const std::string trimmed = TrimString(commandLine);
		if (trimmed.empty())
			return false;

		const bool executed = TryExecuteServerCommandLineInternal(trimmed);
		if (!executed)
		{
			Cypress_PublishServerEvent("command.unknown", "server.command", nlohmann::json({ {"cmd", trimmed} }).dump());
			CYPRESS_LOGTOSERVER(LogLevel::Warning, "Unknown command: {}", trimmed.c_str());
			return false;
		}
		Cypress_PublishServerEvent("command.executed", "server.command", nlohmann::json({ {"cmd", trimmed} }).dump());
		return true;
	}

	void Server::QueueExternalCommand(const std::string& commandLine)
	{
		const std::string trimmed = TrimString(commandLine);
		if (trimmed.empty())
			return;
		std::scoped_lock lock(m_externalCommandQueueMutex);
		m_externalCommandQueue.push_back(trimmed);
	}

	size_t Server::GetExternalCommandQueueSize()
	{
		std::scoped_lock lock(m_externalCommandQueueMutex);
		return m_externalCommandQueue.size();
	}

	void Server::ProcessExternalCommands()
	{
		std::deque<std::string> commands;
		{
			std::scoped_lock lock(m_externalCommandQueueMutex);
			if (m_externalCommandQueue.empty())
				return;
			commands.swap(m_externalCommandQueue);
		}

		for (const auto& command : commands)
		{
			CYPRESS_LOGTOSERVER(LogLevel::Info, "[API] {}", command.c_str());
			const bool ok = ExecuteServerCommandLine(command);
			Cypress_PublishServerEvent("command.api_executed", "webapi", nlohmann::json({
				{"cmd", command},
				{"ok", ok}
				}).dump());
		}
	}

	void Server::UpdateStatus(void* fbServerInstance, float deltaTime)
	{
		if (!fbServerInstance)
			return;

		static unsigned int startSystemTime = GetSystemTime();
		unsigned int sec = (GetSystemTime() - startSystemTime) / 1000;
		unsigned int min = (sec / 60) % 60;
		unsigned int hour = (sec / (60 * 60));

		static unsigned int lastSec = 0;
		static float currentDeltaTime = 0;
		static float sumDeltaTime = 0;
		static unsigned int frameCount = 0;

		m_fbServerInstance = fbServerInstance;

		if (deltaTime > 0.0f)
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
		if (!playerMgr || !serverPeer)
			return;

		fb::ServerGhostManager* ghostMgr = serverPeer->GetGhostManager();
		int ghostcount = ghostMgr ? ghostMgr->ghostCount() : 0;

		fb::SettingsManager* settingsManager = fb::SettingsManager::GetInstance();
		if (!settingsManager)
			return;

		auto* systemSettings = settingsManager->getContainer<fb::SystemSettings>("Game");
		auto* networkSettings = settingsManager->getContainer<fb::NetworkSettings>("Network");
		auto* gameSettings = settingsManager->getContainer<fb::GameSettings>("Game");
		if (!systemSettings || !networkSettings || !gameSettings)
			return;

		// +13 to cut off GamePlatform_
		const char* platformNameRaw = fb::toString(systemSettings->Platform);
		const char* platformName = platformNameRaw ? platformNameRaw : "Unknown";
		if (strlen(platformName) >= 13)
			platformName += 13;

		unsigned int maxPlayerCount = networkSettings->MaxClientCount;
		unsigned int maxSpectatorCount = gameSettings->MaxSpectatorCount;

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
		
		int fpsInt = currentDeltaTime > 0.0f ? int(1.0f / currentDeltaTime) : 0;

		g_program->GetServer()->SetStatusColumn1(
			std::format(
			"FPS: {} \t\t\t\t"
			"UpTime: {}:{}:{} \t\t\t"
			"PlayerCount: {} \t\t\t"
			"GhostCount: {} \t\t\t"
			"Memory (CPU): {} MB",
			fpsInt,
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
				const char* gameModeOpt = setup.getInclusionOption("GameMode");
				const char* hostedModeOpt = setup.getInclusionOption("HostedMode");
				const char* todOpt = setup.getInclusionOption("TOD");
				const char* gameMode = (gameModeOpt && strlen(gameModeOpt) > 1) ? gameModeOpt : "\t";
				const char* hostedMode = (hostedModeOpt && strlen(hostedModeOpt) > 1) ? hostedModeOpt : "\t";
				const char* timeOfDay = (todOpt && strlen(todOpt) > 1) ? todOpt : "\t";

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
		size_t fps = (size_t)fpsInt;
		if (tick != fps)
		{
			tick = fps;
			g_program->GetServer()->SetStatusUpdated(false);
		}
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
		const char* startupCommand = fb::ExecutionContext::getOptionValue("startupCommand");
		if (startupCommand && startupCommand[0] != '\0')
		{
			Cypress_PublishServerEvent("command.startup_requested", "startup", nlohmann::json({ {"cmd", startupCommand} }).dump());
			ExecuteServerCommandLine(startupCommand);
		}

		bool enableApi = fb::ExecutionContext::getOptionValue("enableApi") != nullptr
			|| fb::ExecutionContext::getOptionValue("apiPort") != nullptr;
		if (enableApi && !g_apiControlServer)
		{
			const char* apiBind = fb::ExecutionContext::getOptionValue("apiBind", "127.0.0.1");
			const char* apiPortStr = fb::ExecutionContext::getOptionValue("apiPort", "8787");
			const char* apiToken = fb::ExecutionContext::getOptionValue("apiToken", "");
			int apiPort = atoi(apiPortStr);
			if (apiPort <= 0 || apiPort > 65535)
			{
				CYPRESS_LOGMESSAGE(LogLevel::Warning, "Invalid -apiPort '{}', using 8787", apiPortStr);
				apiPort = 8787;
			}

			g_apiControlServer = std::make_unique<ApiControlServer>(apiBind, apiPort, apiToken);
			g_apiControlServer->Start();
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
