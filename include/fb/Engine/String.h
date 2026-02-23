#pragma once
#include <EASTL/internal/config.h>
#include <eastl_arena_allocator.h>
#include <string.h>
#include <EASTL/string.h>

#define FB_STRING_EMPTYCHARS CYPRESS_GW_SELECT(0x141C710E0, 0x14294ED54)
#define FB_STRING_SET CYPRESS_GW_SELECT(0x14036FFC0, 0x1401CE220)

namespace fb
{
    // recreation of fb's string class based on the bf3 pdb
    class String
    {
    public:
        char* m_chars;

        String() { m_chars = (char*)FB_STRING_EMPTYCHARS; }

        String(const char* str)                         { construct(str, (unsigned int)strlen(str)); }
        String(const char* str, eastl_size_t strLen)    { construct(str, strLen); }
        String(const std::string& str)                  { construct(str.c_str(), str.size()); }
        String(const eastl::string& str)                { construct(str.data(), str.size()); }
        String(const String& str)                       { construct(str.c_str(), str.size()); }
        String(String&& str)
        {
            m_chars = str.m_chars;
            str.m_chars = (char*)FB_STRING_EMPTYCHARS;
        }

        // causes crashes somewhere, fix later and hope any potential memory leaks aren't that bad
        ~String() { /* freeRep(); */ }

        const char* c_str() const { return m_chars; }
        int size()          const { return strlen(m_chars); }
        int length()        const { return strlen(m_chars); }
        bool empty()        const { return m_chars[0] == '\0'; }


        inline String& operator=(String&& str) noexcept
        {
            if (this != &str)
            {
                freeRep();
                m_chars = str.m_chars;
                str.m_chars = (char*)FB_STRING_EMPTYCHARS;
            }
            return *this;
        }


        void construct(const char* str, unsigned int length)
        {
            m_chars = (char*)FB_STRING_EMPTYCHARS;
            if (length)
            {
                set(str, length);
            }
        }

        String& set(const char* str)
        {
            auto fbStringSet_o = reinterpret_cast<fb::String * (*)(fb::String * a1, const char* str, unsigned int length)>(FB_STRING_SET);
            return *fbStringSet_o(this, str, strlen(str));
        }

        String& set(const char* str, unsigned int length)
        {
            auto fbStringSet_o = reinterpret_cast<fb::String * (*)(fb::String * a1, const char* str, unsigned int length)>(FB_STRING_SET);
            return *fbStringSet_o(this, str, length);
        }

        String& operator=(const String& str)
        {
            return (this == &str)
                ? *this
                : set(str.c_str(), str.size());
        }

        String& operator=(const char* str)
        {
            return set(str, (unsigned int)strlen(str));
        }

        void freeRep()
        {
            if ((uintptr_t)m_chars != (uintptr_t)FB_STRING_EMPTYCHARS)
            {
                auto findArena = reinterpret_cast<void* (*)(void*, bool)>(FB_MEMORYARENA_FINDARENAFOROBJECT_ADDRESS);
                void* arena = findArena(m_chars, true);
                auto arenaFree = reinterpret_cast<void(*)(void*, void*)>(FB_MEMORYARENA_FREE_ADDRESS);
                arenaFree(arena, m_chars);
            }
        }
    };
}