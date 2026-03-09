#ifndef NETWORKSETTINGS_H
#define NETWORKSETTINGS_H

namespace fb
{
	class NetworkSettings // size = 0x10
	{
	public:
		char pad_0x00[0x10];
#ifdef CYPRESS_GW1
		unsigned int ProtocolVersion; //0x10
		char pad_0x14[0x4];
		const char* TitleId; //0x18
		unsigned int ClientPort; //0x20
		unsigned int ServerPort; //0x24
		unsigned int MaxGhostCount; //0x28
		unsigned int MaxClientToServerGhostCount; //0x2C
		unsigned int MaxClientCount; //0x30
		unsigned int MaxClientFrameSize; //0x34
		unsigned int MaxServerFrameSize; //0x38
		char pad_0x3C[0x4];
		const char* XlspAddress; //0x40
		const char* ServerAddress; //0x48
		const char* ClientConnectionDebugFilePrefix; //0x50
		const char* ServerConnectionDebugFilePrefix; //0x58
		float TimeNudgeGhostFrequencyFactor; //0x60
		float TimeNudgeBias; //0x64
		float ConnectTimeout; //0x68
		float PacketLossLogInterval; //0x6C
		unsigned int MaxLocalPlayerCount; //0x70
		bool IncrementServerPortOnFail; //0x74
		bool UseFrameManager; //0x75
		bool TimeSyncEnabled; //0x76
		char pad_0x77[0x1];
#elif defined(CYPRESS_GW2)
		unsigned int ProtocolVersion; //0x10
		char pad_0x14[0x4];
		const char* TitleId; //0x18
		unsigned int ClientPort; //0x20
		unsigned int ServerPort; //0x24
		unsigned int MaxGhostCount; //0x28
		unsigned int MaxClientToServerGhostCount; //0x2C
		unsigned int MaxClientCount; //0x30
		unsigned int MaxClientFrameSize; //0x34
		unsigned int MaxServerFrameSize; //0x38
		char pad_0x3C[0x4];
		const char* XlspAddress; //0x40
		const char* ServerAddress; //0x48
		const char* ClientConnectionDebugFilePrefix; //0x50
		const char* ServerConnectionDebugFilePrefix; //0x58
		float SinglePlayerTimeNudgeGhostFrequencyFactor; //0x60
		float SinglePlayerTimeNudgeBias; //0x64
		float SinglePlayerTimeNudge; //0x68
		float SinglePlayerTimeNudgeSmoothingTime; //0x6C
		float SinglePlayerLatencyFactor; //0x70
		float MemorySocketTimeNudgeGhostFrequencyFactor; //0x74
		float MemorySocketTimeNudgeBias; //0x78
		float MemorySocketTimeNudge; //0x7C
		float MemorySocketTimeNudgeSmoothingTime; //0x80
		float MemorySocketLatencyFactor; //0x84
		float LocalHostTimeNudgeGhostFrequencyFactor; //0x88
		float LocalHostTimeNudgeBias; //0x8C
		float LocalHostTimeNudge; //0x90
		float LocalHostTimeNudgeSmoothingTime; //0x94
		float LocalHostLatencyFactor; //0x98
		float DefaultTimeNudgeGhostFrequencyFactor; //0x9C
		float DefaultTimeNudgeBias; //0xA0
		float DefaultTimeNudge; //0xA4
		float DefaultTimeNudgeSmoothingTime; //0xA8
		float DefaultLatencyFactor; //0xAC
		float ConnectTimeout; //0xB0
		float PacketLossLogInterval; //0xB4
		unsigned int ValidLocalPlayersMask; //0xB8
		unsigned int DesiredLocalPlayersMask; //0xBC
		unsigned int PersistentLocalPlayersMask; //0xC0
		bool SinglePlayerUserFrequencyFactorOverride; //0xC4
		bool SinglePlayerAutomaticTimeNudge; //0xC5
		bool MemorySocketUserFrequencyFactorOverride; //0xC6
		bool MemorySocketAutomaticTimeNudge; //0xC7
		bool LocalHostUserFrequencyFactorOverride; //0xC8
		bool LocalHostAutomaticTimeNudge; //0xC9
		bool DefaultUserFrequencyFactorOverride; //0xCA
		bool DefaultAutomaticTimeNudge; //0xCB
		bool IncrementServerPortOnFail; //0xCC
		bool UseFrameManager; //0xCD
		bool TimeSyncEnabled; //0xCE
		bool MLUREnabled; //0xCF
#endif
	}; //size = 0x78
} //namespace fb

#endif