#include "pch.h"
#include "fbServerHooks.h"

#include <string>
#include <sstream>
#include <Core/Program.h>
#include <Core/Console/ConsoleFunctions.h>
#include <Core/Logging.h>
#include <fb/Engine/Server.h>
#include <fb/Engine/ServerGameContext.h>
#include <json.hpp>

#if(HAS_DEDICATED_SERVER)
DEFINE_HOOK(
	fb_Server_start,
	__fastcall,
	__int64,

	void* thisPtr,
	fb::ServerSpawnInfo* info,
	Kyber::ServerSpawnOverrides* spawnOverrides
)
{
#if(HAS_DEDICATED_SERVER)
	if (!g_program->GetInitialized())
	{
		bool wsaInit = g_program->InitWSA();
		CYPRESS_ASSERT(wsaInit, "WSA failed to initialize!");
		g_program->SetInitialized(true);
	}
#endif
	return Orig_fb_Server_start(thisPtr, info, spawnOverrides);
}

DEFINE_HOOK(
	fb_Server_update,
	__fastcall,
	bool,

	void* thisPtr,
	void* params
)
{
#if(HAS_DEDICATED_SERVER)
	bool updated = Orig_fb_Server_update(thisPtr, params);
	if (g_program->IsServer())
	{
			Cypress::Server* server = g_program->GetServer();
			server->ProcessExternalCommands();
			server->UpdateStatus(thisPtr, ptrread<float>(params, CYPRESS_GW_SELECT(0x18, 0x28)));

		bool statusUpdated = !server->GetStatusUpdated();
		server->SetStatusUpdated(true);
		if (statusUpdated)
		{
			PostMessageA(*server->GetMainWindow(), WM_APP_UPDATESTATUS, 0, 0);
		}
	}
	return updated;
#else
	Orig_fb_Server_update(thisPtr, params);
#endif
}

std::string g_commandBoxCommand;

DEFINE_HOOK(
	fb_editBoxWndProcProxy,
	__cdecl,
	LRESULT,

	HWND hWnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam
)
{
	switch (msg)
	{
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_RETURN:
		case VK_TAB:
			char buf[1024];
			int cmdLen = GetWindowTextA(hWnd, buf, sizeof(buf));
			g_commandBoxCommand = buf;
			break;
		}
	}

	LRESULT ret = Orig_fb_editBoxWndProcProxy(hWnd, msg, wParam, lParam);

	switch (msg)
	{
		case WM_KEYDOWN:
			switch (wParam)
			{
			case VK_RETURN:
				CYPRESS_LOGTOSERVER(LogLevel::Info, "{}", g_commandBoxCommand.c_str());
				Cypress_PublishServerEvent("command.console_submitted", "hook.fb_editBoxWndProcProxy",
					nlohmann::json({ {"cmd", g_commandBoxCommand} }).dump());
				break;
			}
		}

	return ret;
}

DEFINE_HOOK(
	fb_windowProcedure,
	__stdcall,
	__int64,

	HWND hWnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam
)
{
	if (g_program->IsServer() && !g_program->IsHeadless())
	{
		HWND commandBox = *g_program->GetServer()->GetCommandBox();
		if (msg == WM_APP_UPDATESTATUS && commandBox)
		{
			HWND* g_statusBox = (HWND*)OFFSET_G_STATUSBOX;
			SetWindowTextA(*g_statusBox, g_program->GetServer()->GetStatusColumn1().c_str());
			SetWindowTextA(*++g_statusBox, g_program->GetServer()->GetStatusColumn2().c_str());
			return DefWindowProcA(hWnd, msg, wParam, lParam);
		}
		else if (msg == WM_COMMAND)
		{
			if (HIWORD(wParam) == BN_CLICKED && (HWND)lParam == *g_program->GetServer()->GetToggleLogButtonBox())
			{
				if (!g_program->GetServer()->GetServerLogEnabled())
				{
					g_program->GetServer()->SetServerLogEnabled(true);
					CYPRESS_LOGTOSERVER(LogLevel::Info, "Log window enabled.");
					SetWindowTextA(*g_program->GetServer()->GetToggleLogButtonBox(), "Disable Logs");
					PostMessageA(*g_program->GetServer()->GetMainWindow(), WM_APP_UPDATESTRINGLIST, 0, 0);
				}
				else
				{
					CYPRESS_LOGTOSERVER(LogLevel::Info, "Log window disabled.");
					g_program->GetServer()->SetServerLogEnabled(false);
					SetWindowTextA(*g_program->GetServer()->GetToggleLogButtonBox(), "Enable Logs");
				}
				return DefWindowProcA(hWnd, msg, wParam, lParam);
			}
		}
	}

	return Orig_fb_windowProcedure(hWnd, msg, wParam, lParam);
}

DEFINE_HOOK(
	fb_ServerPVZLevelControlEntity_loadLevel,
	__fastcall,
	void,

	void* thisPtr,
	const char* level,
	const char* inclusion
)
{
	g_program->GetServer()->SetLoadRequestFromLevelControl(true);
	Orig_fb_ServerPVZLevelControlEntity_loadLevel(thisPtr, level, inclusion);
	g_program->GetServer()->SetLoadRequestFromLevelControl(false);
}

DEFINE_HOOK(
	fb_ServerLevelControlEntity_loadLevel,
	__fastcall,
	void,

	void* thisPtr,
	bool notifyLevelComplete
)
{
	g_program->GetServer()->SetLoadRequestFromLevelControl(true);
	Orig_fb_ServerLevelControlEntity_loadLevel(thisPtr, notifyLevelComplete);
	g_program->GetServer()->SetLoadRequestFromLevelControl(false);
}

DEFINE_HOOK(
	fb_ServerLoadLevelMessage_post,
	__cdecl,
	void,

	fb::LevelSetup* levelSetup,
	bool fadeOut,
	bool forceReloadResources
)
	{
		Cypress::Server* server = g_program->GetServer();
		if (server->GetIsLoadRequestFromLevelControl() && server->IsUsingPlaylist())
	{
		const auto nextSetup = server->GetServerPlaylist()->GetNextSetup();
		server->LevelSetupFromPlaylistSetup(levelSetup, nextSetup);
		server->ApplySettingsFromPlaylistSetup(nextSetup);
		}
	Cypress_PublishServerEvent("level.load_requested", "hook.fb_ServerLoadLevelMessage_post",
		nlohmann::json({
			{"level", levelSetup && levelSetup->m_name.length() > 0 ? levelSetup->m_name.c_str() : ""},
			{"gameMode", levelSetup ? levelSetup->getInclusionOption("GameMode") : ""},
			{"hostedMode", levelSetup ? levelSetup->getInclusionOption("HostedMode") : ""},
			{"tod", levelSetup ? levelSetup->getInclusionOption("TOD") : ""},
			{"fadeOut", fadeOut},
			{"forceReloadResources", forceReloadResources}
			}).dump());
		Orig_fb_ServerLoadLevelMessage_post(levelSetup, fadeOut, forceReloadResources);
	}

DEFINE_HOOK(
	fb_ServerConnection_onCreatePlayerMsg,
	__fastcall,
	void*,

	fb::ServerConnection* thisPtr,
	void* msg
)
{
	const char* playerName = ptrread<const char*>(msg, 0x48);
	int nameLen = strlen(playerName);
	if (nameLen < 3 || nameLen > 32)
	{
		thisPtr->m_shouldDisconnect = true;
		thisPtr->m_disconnectReason = 0x4;
		thisPtr->m_reasonText = "Invalid Username Length";
	}

	for (const char* p = playerName; *p != '\0'; ++p)
	{
		if (iscntrl(static_cast<unsigned char>(*p)))
		{
			thisPtr->m_shouldDisconnect = true;
			thisPtr->m_disconnectReason = 0x4;
			thisPtr->m_reasonText = "Invalid Characters in Username";
			break;
		}
	}

	CYPRESS_LOGTOSERVER(LogLevel::Info, "{} is trying to join from machine {}", playerName, thisPtr->m_machineId.c_str());
	Cypress_PublishServerEvent("player.connect_attempt", "hook.fb_ServerConnection_onCreatePlayerMsg",
		nlohmann::json({
			{"playerName", playerName ? playerName : ""},
			{"machineId", thisPtr->m_machineId.c_str()},
			{"shouldDisconnect", thisPtr->m_shouldDisconnect},
			{"disconnectReason", thisPtr->m_disconnectReason},
			{"disconnectReasonText", thisPtr->m_reasonText.c_str()}
			}).dump());
	return Orig_fb_ServerConnection_onCreatePlayerMsg(thisPtr, msg);
}

DEFINE_HOOK(
	fb_ServerPlayerManager_addPlayer,
	__fastcall,
	void*,

	fb::ServerPlayerManager* thisPtr,
	fb::ServerPlayer* player,
	const char* nickname
	)
	{
		if (!player->isAIPlayer())
		{
			const char* joinedName = nickname ? nickname : (player->m_name ? player->m_name : "");
			CYPRESS_LOGTOSERVER(LogLevel::Info, "[Id: {}] {} has joined the server", player->getPlayerId(), joinedName);
			Cypress_PublishServerEvent("player.joined", "hook.fb_ServerPlayerManager_addPlayer",
				nlohmann::json({
					{"playerId", player->getPlayerId()},
					{"name", joinedName}
					}).dump());
		}
		return Orig_fb_ServerPlayerManager_addPlayer(thisPtr, player, nickname);
	}

DEFINE_HOOK(
	fb_ServerPlayer_disconnect,
	__fastcall,
	void,

	fb::ServerPlayer* thisPtr,
	fb::SecureReason reason,
	eastl::string& reasonText
)
{
	CYPRESS_LOGTOSERVER(LogLevel::Info, "[Id: {}] {} has left the server (Reason: {}, {})",
		thisPtr->getPlayerId(),
		thisPtr->m_name,
		reasonText.empty() ? "None provided" : reasonText.c_str(),
		fb::SecureReason_toString[reason]);
	Cypress_PublishServerEvent("player.left", "hook.fb_ServerPlayer_disconnect",
		nlohmann::json({
			{"playerId", thisPtr->getPlayerId()},
			{"name", thisPtr->m_name.c_str()},
			{"reasonCode", (int)reason},
			{"reasonName", fb::SecureReason_toString[reason]},
			{"reasonText", reasonText.empty() ? "" : reasonText.c_str()}
			}).dump());

	Orig_fb_ServerPlayer_disconnect(thisPtr, reason, reasonText);
}

DEFINE_HOOK(
	fb_sub_140112AA0,
	__fastcall,
	__int64,

	void* a1,
	int a2
)
{
	if (g_program->IsServer())
		a2 = 0;
	return Orig_fb_sub_140112AA0(a1, a2);
}
#endif
