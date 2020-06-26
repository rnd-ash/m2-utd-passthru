// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "Logger.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{   
    LOGGER.logDebug("APIENTRY", "DLL entry called - HMOUDLE: %p", hModule);
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        LOGGER.logDebug("APIENTRY", "Process attached");
        break;
    case DLL_THREAD_ATTACH:
        LOGGER.logDebug("APIENTRY", "Thread attached");
        break;
    case DLL_THREAD_DETACH:
        LOGGER.logDebug("APIENTRY", "Thread detached");
        break;
    case DLL_PROCESS_DETACH:
        LOGGER.logDebug("APIENTRY", "Process detached");
        break;
    }
    return TRUE;
}

