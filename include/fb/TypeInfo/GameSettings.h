#ifndef GAMESETTINGS_H
#define GAMESETTINGS_H

#include "SystemSettings.h"

namespace fb
{
	class GameSettings : public SystemSettings // size = 0x20
	{
	public:
		unsigned int MaxPlayerCount; //0x20
		unsigned int MaxSpectatorCount; //0x24
		unsigned int MinPlayerCountElimination; //0x28
		int LogFileCollisionMode; //0x2C
		unsigned int LogFileRotationHistoryLength; //0x30
		char pad_0x34[0x4];
		const char* Level; //0x38
		const char* StartPoint; //0x40
		void* InputConfiguration; //0x48
		const char* ActiveGameModeViewDefinition; //0x50
		void* GameModeViewDefinitions; //0x58
		int DefaultTeamId; //0x60
		unsigned int PS3ContentRatingAge; //0x64
		unsigned int LogHistory; //0x68
		char pad_0x6C[0x4];
		void* Version; //0x70
		void* LayerInclusionTable; //0x78
		const char* DefaultLayerInclusion; //0x80
		float TimeBeforeSpawnIsAllowed; //0x88
		float LevelWarmUpTime; //0x8C
		float TimeToWaitForQuitTaskCompletion; //0x90
		char pad_0x94[0x4];
		void* Player; //0x98
		void* DifficultySettings; //0xA0
		int DifficultyIndex; //0xA8
		int CurrentSKU; //0xAC
		void* GameSettingsComponents; //0xB0
		bool LogFileEnable; //0xB8
		bool ResourceRefreshAlwaysAllowed; //0xB9
		bool SpawnMaxLocalPlayersOnStartup; //0xBA
		bool UseSpeedBasedDetailedCollision; //0xBB
		bool UseSingleWeaponSelector; //0xBC
		bool AutoAimEnabled; //0xBD
		bool HasUnlimitedAmmo; //0xBE
		bool HasUnlimitedMags; //0xBF
		bool RotateLogs; //0xC0
		bool AdjustVehicleCenterOfMass; //0xC1
		bool AimAssistEnabled; //0xC2
		bool AimAssistUsePolynomials; //0xC3
		bool ForceFreeStreaming; //0xC4
		bool ForceDisableFreeStreaming; //0xC5
		bool IsGodMode; //0xC6
		bool IsJesusMode; //0xC7
		bool IsJesusModeAi; //0xC8
		bool GameAdministrationEnabled; //0xC9
		bool AllowDestructionOutsideCombatArea; //0xCA
		char pad_0xCB[0x5];
	}; //size = 0xD0
} //namespace fb

#endif