// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here
#include "framework.h"
#include <cstdint>
// begin Cypress includes
#include <MemUtil.h>
#include <MinHook/MinHook.h>
#include <Core/VersionInfo.h>
#include <Core/Logging.h>
#include <Core/Assert.h>
#include <Core/Config.h>
#include <ServerBanlist.h>
#include <ServerPlaylist.h>
#include <StringUtil.h>

// Configure EASTL
#define EASTL_SIZE_T_32BIT 1
#define EASTL_CUSTOM_FLOAT_CONSTANTS_REQUIRED 1
#define EA_HAVE_CPP11_INITIALIZER_LIST 1
#define CHAR8_T_DEFINED 1

#include "eastl_arena_allocator.h"

#define EASTLAllocatorType fb::eastl_arena_allocator
#define EASTLAllocatorDefault fb::GetDefaultAllocator
// end EASTL configuration
// end Cypress includes
#endif //PCH_H
