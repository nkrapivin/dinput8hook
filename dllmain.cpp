// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "framework.h"
#include "dinput8hook.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReasonForCall, LPVOID lpReserved) {
    switch (dwReasonForCall) {
        case DLL_PROCESS_DETACH: {
            return TryUnloadDll();
        }

        default: {
            break;
        }
    }

    return TRUE;
}

