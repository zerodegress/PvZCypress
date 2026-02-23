// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <Core/Program.h>

#pragma comment(lib, "Ws2_32.lib")

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  dwCallReason,
                       LPVOID lpReserved
                     )
{
    if (dwCallReason == DLL_PROCESS_ATTACH)
    {
        g_program = new Cypress::Program(hModule);
    }
    else if (dwCallReason == DLL_PROCESS_DETACH)
    {
        delete g_program;
    }
    return TRUE;
}