#include "pch.h"
#include "eastl_arena_allocator.h"
#include <stdio.h>

using namespace fb;

fb::eastl_arena_allocator::eastl_arena_allocator(const char* pName)
{
	this->mpArena = NULL;
}

fb::eastl_arena_allocator::eastl_arena_allocator(void* memoryArena)
{
	this->mpArena = memoryArena;
}

fb::eastl_arena_allocator::eastl_arena_allocator(const eastl_arena_allocator& x)
{
	this->mpArena = x.mpArena;
}

fb::eastl_arena_allocator::eastl_arena_allocator(const eastl_arena_allocator& x, const char* pName)
{
	this->mpArena = x.mpArena;
}

fb::eastl_arena_allocator& eastl_arena_allocator::operator=(const eastl_arena_allocator& x)
{
	this->mpArena = x.mpArena;
	return *this;
}

bool fb::eastl_arena_allocator::operator==(const eastl_arena_allocator& x)
{
	return this->mpArena == x.mpArena;
}

void* fb::eastl_arena_allocator::allocate(size_t n, int flags)
{
	auto arenaAlloc = reinterpret_cast<void* (*)(void*, __int64 size, __int64 alignment)>(FB_MEMORYARENA_ALLOC_ADDRESS);
	if (!this->mpArena)
	{
		auto findArena = reinterpret_cast<void* (*)(void*, bool)>(FB_MEMORYARENA_FINDARENAFOROBJECT_ADDRESS);
		this->mpArena = findArena(this, true);
	}
	void* allocMem = arenaAlloc(this->mpArena, n, sizeof(void*));
	return allocMem;
}

void* fb::eastl_arena_allocator::allocate(size_t n, size_t alignment, size_t offset, int flags)
{
	auto arenaAlloc = reinterpret_cast<void* (*)(void*, __int64 size, __int64 alignment)>(FB_MEMORYARENA_ALLOC_ADDRESS);
	if (!this->mpArena)
	{
		auto findArena = reinterpret_cast<void* (*)(void*, bool)>(FB_MEMORYARENA_FINDARENAFOROBJECT_ADDRESS);
		this->mpArena = findArena(this, true);
	}
	void* allocMem = arenaAlloc(this->mpArena, n, alignment);
	return allocMem;
}

void fb::eastl_arena_allocator::deallocate(void* p, size_t n)
{
	auto arenaFree = reinterpret_cast<void(*)(void*, void*)>(FB_MEMORYARENA_FREE_ADDRESS);
	arenaFree(this->mpArena, p);
}

const char* fb::eastl_arena_allocator::get_name() const
{
	return "CYP_EASTL";
}

void fb::eastl_arena_allocator::set_name(const char* pName)
{
}

const void* fb::eastl_arena_allocator::get_arena() const
{
	return this->mpArena;
}

void fb::eastl_arena_allocator::set_arena(void* pArena)
{
	this->mpArena = pArena;
}

eastl_arena_allocator gDefaultAllocator;
eastl_arena_allocator* gpDefaultAllocator = &gDefaultAllocator;

eastl_arena_allocator* fb::GetDefaultAllocator()
{
	return gpDefaultAllocator;
}

eastl_arena_allocator* fb::SetDefaultAllocator(eastl_arena_allocator* pAllocator)
{
	eastl_arena_allocator* const pPrevAllocator = gpDefaultAllocator;
	gpDefaultAllocator = pAllocator;
	return pPrevAllocator;
}
