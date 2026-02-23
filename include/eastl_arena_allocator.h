#pragma once
#include <EASTL/internal/config.h>

#define FB_MEMORYARENA_ALLOC_ADDRESS CYPRESS_GW_SELECT(0x140389540, 0x140155630)
#define FB_MEMORYARENA_FREE_ADDRESS CYPRESS_GW_SELECT(0x140389FC0, 0x140155740)
#define FB_MEMORYARENA_FINDARENAFOROBJECT_ADDRESS CYPRESS_GW_SELECT(0x140389E70, 0x1401CA550)

namespace fb
{

	class eastl_arena_allocator
	{
	public:
		//EASTL_ALLOCATOR_EXPLICIT eastl_arena_allocator(const char* pName = EASTL_NAME_VAL(EASTL_DEFAULT_NAME_PREFIX));
		EASTL_ALLOCATOR_EXPLICIT eastl_arena_allocator(const char* pName = EASTL_NAME_VAL("CYP_EASTL"));
		eastl_arena_allocator(void* memoryArena);
		eastl_arena_allocator(const eastl_arena_allocator& x);
		eastl_arena_allocator(const eastl_arena_allocator& x, const char* pName);

		eastl_arena_allocator& operator=(const eastl_arena_allocator& x);
		bool operator==(const eastl_arena_allocator& x);

		void* allocate(size_t n, int flags = 0);
		void* allocate(size_t n, size_t alignment, size_t offset, int flags = 0);
		void  deallocate(void* p, size_t n);

		const char* get_name() const;
		void        set_name(const char* pName);

		const void* get_arena() const;
		void		set_arena(void* pArena);

	protected:
		void* mpArena;
	};

	eastl_arena_allocator* GetDefaultAllocator();
	eastl_arena_allocator* SetDefaultAllocator(eastl_arena_allocator* pAllocator);

} // namespace fb