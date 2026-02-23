#pragma once
#include <IGameModule.h>

#ifdef CYPRESS_GW1
namespace Cypress
{

	class GW1Module : public IGameModule
	{
	public:
		void InitGameHooks() override;
		void InitMemPatches() override;
		void InitDedicatedServerPatches(class Cypress::Server* pServer) override;
		void RegisterCommands() override;
	};

}
#endif