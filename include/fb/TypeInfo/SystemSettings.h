#ifndef SYSTEMSETTINGS_H
#define SYSTEMSETTINGS_H

#include "GamePlatform.h"

namespace fb
{
	class SystemSettings
	{
	public:
		char pad_0x0[0x10];
		const char* Name; //0x10
		fb::GamePlatform Platform; //0x18
		char pad_0x1C[0x4];
	}; //size = 0x20
} //namespace fb

#endif