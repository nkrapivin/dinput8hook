// dinput8hook.cpp : Defines the exported functions for the DLL.
//

#include "pch.h"
#include "framework.h"
#include "dinput8hook.h"

static HMODULE               hDinput8dll;
static DirectInput8Create_t  DirectInput8CreateOrig;
static DllCanUnloadNow_t     DllCanUnloadNowOrig;
static DllGetClassObject_t   DllGetClassObjectOrig;
static DllRegisterServer_t   DllRegisterServerOrig;
static DllUnregisterServer_t DllUnregisterServerOrig;
static GetdfDIJoystick_t     GetdfDIJoystickOrig;
static CreateDevice_t        OldCreateDevice;
static SetCooperativeLevel_t OldSetCooperativeLevel;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReasonForCall, LPVOID lpReserved) {
    switch (dwReasonForCall) {
        case DLL_PROCESS_ATTACH: {
            hDinput8dll = NULL;
            DirectInput8CreateOrig = NULL;
            DllCanUnloadNowOrig = NULL;
            DllGetClassObjectOrig = NULL;
            DllRegisterServerOrig = NULL;
            DllUnregisterServerOrig = NULL;
            GetdfDIJoystickOrig = NULL;
            OldCreateDevice = NULL;
            OldSetCooperativeLevel = NULL;
            break;
        }

        case DLL_PROCESS_DETACH: {
            return TryUnloadDll();
        }

        default: {
            break;
        }
    }

    return TRUE;
}

static BOOL NikWaitForDebugger() {
#ifdef _DEBUG
    while (!IsDebuggerPresent()) {
        Sleep(1);
    }

    return TRUE;
#else
    return FALSE;
#endif
}

void mystrcat(LPWSTR str, LPCWSTR src) {
    while (*str) str++;
    while (*src) *(str++) = *(src++);
    *(str++) = L'\0';
}

static BOOL EnsureLoadOriginalDll() {
    // need to load...
    WCHAR pBuff[1024];
    PWSTR pStr;
    BOOL ret;
    HRESULT hr;
    HMODULE hm;
    FARPROC p1, p2, p3, p4, p5, p6;

    NikWaitForDebugger();

    // already loaded?
    if (hDinput8dll) {
        return TRUE;
    }

    pStr = NULL;
    ret = FALSE;
    hm = NULL;
    p1 = NULL; p2 = NULL; p3 = NULL; p4 = NULL; p5 = NULL; p6 = NULL;
    // place a breakpoint here:
    SecureZeroMemory(&pBuff[0], sizeof(pBuff));
    hr = SHGetKnownFolderPath(&FOLDERID_System, KF_FLAG_DEFAULT, NULL, &pStr);

    if (SUCCEEDED(hr)) {
        mystrcat(&pBuff[0], pStr);
        mystrcat(&pBuff[0], L"\\dinput8.dll");

        hm = LoadLibraryW(pBuff);
        if (hm) {
            p1 = GetProcAddress(hm, "DirectInput8Create");
            p2 = GetProcAddress(hm, "DllCanUnloadNow");
            p3 = GetProcAddress(hm, "DllGetClassObject");
            p4 = GetProcAddress(hm, "DllRegisterServer");
            p5 = GetProcAddress(hm, "DllUnregisterServer");
            p6 = GetProcAddress(hm, "GetdfDIJoystick");
            if (p1 && p2 && p3 && p4 && p5) {
                hDinput8dll             = hm;
                DirectInput8CreateOrig  = (DirectInput8Create_t) (p1);
                DllCanUnloadNowOrig     = (DllCanUnloadNow_t)    (p2);
                DllGetClassObjectOrig   = (DllGetClassObject_t)  (p3);
                DllRegisterServerOrig   = (DllRegisterServer_t)  (p4);
                DllUnregisterServerOrig = (DllUnregisterServer_t)(p5);
                GetdfDIJoystickOrig     = (GetdfDIJoystick_t)    (p6);
                ret                     = TRUE;
            }
        }
    }

    if (pStr) {
        CoTaskMemFree(pStr);
        pStr = NULL;
    }

    return ret;
}

static HRESULT DllLoadFailed() {
    MessageBoxW(NULL, L"Failed to load the original dinput8.dll.", L"dinput8hook: Error.", MB_OK | MB_ICONERROR);

#ifdef _DEBUG
    DebugBreak();
#endif

    return OLE_E_FIRST;
}

HRESULT WINAPI SetCooperativeLevel(LPDIRECTINPUTDEVICE8W self, HWND hwnd, DWORD dwflags) {
    DWORD newflags;
    HRESULT hr;

    // inspect me in the debugger.
    newflags = DISCL_BACKGROUND | DISCL_NONEXCLUSIVE;
    hr = OldSetCooperativeLevel(self, hwnd, newflags);

    // pass-through
    return hr;
}

HRESULT WINAPI MyCreateDevice(LPDIRECTINPUT8W self, REFGUID refguid, LPDIRECTINPUTDEVICE8W* lpddi, LPUNKNOWN lpunk) {
    SIZE_T wrote;
    SetCooperativeLevel_t f;
    SetCooperativeLevel_t* fptr;
    HRESULT hr;

    wrote = 0;
    f = &SetCooperativeLevel;
    fptr = NULL;
    hr = OldCreateDevice(self, refguid, lpddi, lpunk);

    if (SUCCEEDED(hr)) {
        // can modify the device :3
        fptr = &((*((LPDIRECTINPUTDEVICE8W*)lpddi))->lpVtbl->SetCooperativeLevel);
        if (!OldSetCooperativeLevel) {
            OldSetCooperativeLevel = *fptr;
        }

        WriteProcessMemory(GetCurrentProcess(), fptr, &f, sizeof(f), &wrote);
    }

    return hr;
}

BOOL TryUnloadDll() {
    BOOL r;
    if (hDinput8dll) {
        r = FreeLibrary(hDinput8dll);
        hDinput8dll = NULL;
        return r;
    }

    /* was not even loaded? */
    return FALSE;
}

HRESULT WINAPI DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter) {
    SIZE_T wrote;
    CreateDevice_t tt;
    CreateDevice_t* funcaddrptr;
    HRESULT hr;

    if (!EnsureLoadOriginalDll()) {
        return DllLoadFailed();
    }

    wrote = 0;
    tt = &MyCreateDevice;
    funcaddrptr = NULL;
    hr = DirectInput8CreateOrig(hinst, dwVersion, riidltf, ppvOut, punkOuter);
    if (FAILED(hr)) {
        return hr;
    }

    /* the fun: */
    funcaddrptr = &((*((LPDIRECTINPUT8W*)ppvOut))->lpVtbl->CreateDevice);
    if (!OldCreateDevice) {
        OldCreateDevice = *funcaddrptr;
    }
    
    // need to write new function ptr at that addr.
    WriteProcessMemory(GetCurrentProcess(), funcaddrptr, &tt, sizeof(tt), &wrote);

    return hr;
}

__control_entrypoint(DllExport)
STDAPI DllCanUnloadNow() {
    if (!EnsureLoadOriginalDll()) return DllLoadFailed();

    return DllCanUnloadNowOrig();
}

_Check_return_
STDAPI DllGetClassObject(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ LPVOID FAR* ppv) {
    if (!EnsureLoadOriginalDll()) {
        //*ppv = NULL;
        return DllLoadFailed();
    }

    return DllGetClassObjectOrig(rclsid, riid, ppv);
}

HRESULT WINAPI DllRegisterServer() {
    if (!EnsureLoadOriginalDll()) return DllLoadFailed();

    return DllRegisterServerOrig();
}

HRESULT WINAPI DllUnregisterServer() {
    if (!EnsureLoadOriginalDll()) return DllLoadFailed();

    return DllUnregisterServerOrig();
}

LPCDIDATAFORMAT WINAPI GetdfDIJoystick() {
    if (!EnsureLoadOriginalDll()) DllLoadFailed();

    return GetdfDIJoystickOrig();
}
