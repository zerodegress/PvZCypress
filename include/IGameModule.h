#pragma once

namespace Cypress
{
	class Server;

	class IGameModule
	{
	public:
		virtual void InitGameHooks() = 0;
		virtual void InitMemPatches() = 0;
		virtual void InitDedicatedServerPatches(class Cypress::Server* pServer) = 0;
		virtual void RegisterCommands() = 0;
	};

}