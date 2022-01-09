// dinput8hook.cpp : Defines the exported functions for the DLL.
//

#include "pch.h"
#include "framework.h"
#include "dinput8hook.h"

static HMODULE               hDinput8dll{ nullptr };
static DirectInput8Create_t  DirectInput8CreateOrig{ nullptr };
static DllCanUnloadNow_t     DllCanUnloadNowOrig{ nullptr };
static DllGetClassObject_t   DllGetClassObjectOrig{ nullptr };
static DllRegisterServer_t   DllRegisterServerOrig{ nullptr };
static DllUnregisterServer_t DllUnregisterServerOrig{ nullptr };
static GetdfDIJoystick_t     GetdfDIJoystickOrig{ nullptr };

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

struct DummyTable {
    LPFUNC* table;
};

static BOOL EnsureLoadOriginalDll() {
    // already loaded?
    if (hDinput8dll) {
        return TRUE;
    }

    // need to load...
    NikWaitForDebugger();
    WCHAR pBuff[4096]{ L'\0' };
    PWSTR pStr{ nullptr };
    BOOL ret{ FALSE };
    HRESULT hr{ SHGetKnownFolderPath(FOLDERID_System, KF_FLAG_DEFAULT, nullptr, &pStr) };
    HMODULE hm{ nullptr };

    if (SUCCEEDED(hr)) {
        wcscat_s(pBuff, pStr);
        wcscat_s(pBuff, L"\\dinput8.dll");
        hm = LoadLibraryW(pBuff);
        if (hm) {
            auto p1 = GetProcAddress(hm, "DirectInput8Create");
            auto p2 = GetProcAddress(hm, "DllCanUnloadNow");
            auto p3 = GetProcAddress(hm, "DllGetClassObject");
            auto p4 = GetProcAddress(hm, "DllRegisterServer");
            auto p5 = GetProcAddress(hm, "DllUnregisterServer");
            auto p6 = GetProcAddress(hm, "GetdfDIJoystick");
            if (p1 && p2 && p3 && p4 && p5) {
                hDinput8dll             = hm;
                DirectInput8CreateOrig  = reinterpret_cast<DirectInput8Create_t> (p1);
                DllCanUnloadNowOrig     = reinterpret_cast<DllCanUnloadNow_t>    (p2);
                DllGetClassObjectOrig   = reinterpret_cast<DllGetClassObject_t>  (p3);
                DllRegisterServerOrig   = reinterpret_cast<DllRegisterServer_t>  (p4);
                DllUnregisterServerOrig = reinterpret_cast<DllUnregisterServer_t>(p5);
                GetdfDIJoystickOrig     = reinterpret_cast<GetdfDIJoystick_t>    (p6);
                ret                     = TRUE;
            }
        }
    }

    if (pStr) {
        CoTaskMemFree(pStr);
        pStr = nullptr;
    }

    return ret;
}

static HRESULT DllLoadFailed() {
    MessageBoxW(nullptr, L"Failed to load the original dinput8.dll.", L"dinput8hook: Error in " __FUNCTIONW__, MB_OK | MB_ICONERROR);

#ifdef _DEBUG
    DebugBreak();
#endif

    return OLE_E_FIRST;
}

static LPFUNC OldCreateDevice{};
static LPFUNC OldSetCooperativeLevel{};

HRESULT WINAPI SetCooperativeLevel(LPDIRECTINPUTDEVICE8W self, HWND hwnd, DWORD dwflags) {
    SIZE_T wrote{ 0 };
    HRESULT hr{};
    DummyTable* aaa{ reinterpret_cast<DummyTable*>(self) };
    LPFUNC* funcaddrptr{ &aaa->table[13] };

    // new flags.
    DWORD dwcopy{ DISCL_BACKGROUND | DISCL_NONEXCLUSIVE };

    // need to restore the original method.
    auto mymt{ OldSetCooperativeLevel };
    if (mymt)
    hr = reinterpret_cast<HRESULT(WINAPI*)(LPDIRECTINPUTDEVICE8W, HWND, DWORD)>(mymt)(self, hwnd, dwcopy);

    // pass-through
    return hr;
}

HRESULT WINAPI MyCreateDevice(LPDIRECTINPUT8W self, REFGUID refguid, LPDIRECTINPUTDEVICE8W* lpddi, LPUNKNOWN lpunk) {

    // need to restore the original method.
    SIZE_T wrote{ 0 };
    DummyTable* aaa{ reinterpret_cast<DummyTable*>(self) };
    LPFUNC* funcaddrptr{ &(aaa->table[3]) };
    HRESULT hr{};

    auto mymt{ OldCreateDevice };
    if (mymt)
    hr = reinterpret_cast<HRESULT(WINAPI*)(LPDIRECTINPUT8W, REFGUID, LPDIRECTINPUTDEVICE8W*, LPUNKNOWN)>(mymt)(self, refguid, lpddi, lpunk);

    if (SUCCEEDED(hr)) {
        // can modify the device :3
        LPFUNC scltt{ reinterpret_cast<LPFUNC>(&SetCooperativeLevel) };
        DummyTable* aaa{ reinterpret_cast<DummyTable*>(*lpddi) };
        LPFUNC* dvfuncptr{ &aaa->table[13] };
        if (!OldSetCooperativeLevel) OldSetCooperativeLevel = *dvfuncptr;
        WriteProcessMemory(GetCurrentProcess(), reinterpret_cast<LPVOID>(dvfuncptr), &scltt, sizeof(scltt), &wrote);
    }

    return hr;
}

extern "C" {


    BOOL TryUnloadDll() {
        if (hDinput8dll) {
            BOOL r{ FreeLibrary(hDinput8dll) };
            hDinput8dll = nullptr;
            return r;
        }

        /* was not even loaded? */
        return FALSE;
    }

    HRESULT WINAPI DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter) {
        if (!EnsureLoadOriginalDll()) return DllLoadFailed();
        
        HRESULT hr{ DirectInput8CreateOrig(hinst, dwVersion, riidltf, ppvOut, punkOuter) };
        if (FAILED(hr)) return hr;

        LPDIRECTINPUT8W self{ reinterpret_cast<LPDIRECTINPUT8W>(*ppvOut) };
        DummyTable* aaa{ reinterpret_cast<DummyTable*>(self) };
        /* the fun: */
        /* we know gamemaker will always allocate a IID_IDirectInput8W interface... */

        // [0] - QueryInterface, [1] - AddRef, [2] - Release, [3] - CreateDevice, [4] - EnumDevices.
        // ржака
        LPFUNC* funcaddrptr{ &(aaa->table[3]) };
        if (!OldCreateDevice) OldCreateDevice = *funcaddrptr;
        // need to write new function ptr at that addr.

        LPFUNC tt{ reinterpret_cast<LPFUNC>(&MyCreateDevice) };
        SIZE_T wrote{ 0 };
        BOOL wok{ WriteProcessMemory(GetCurrentProcess(), reinterpret_cast<LPVOID>(funcaddrptr), &tt, sizeof(tt), &wrote) };

        return hr;
    }

    HRESULT WINAPI DllCanUnloadNow() {
        if (!EnsureLoadOriginalDll()) return DllLoadFailed();

        return DllCanUnloadNowOrig();
    }

    _Check_return_
    HRESULT WINAPI DllGetClassObject(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ LPVOID FAR* ppv) {
        if (!EnsureLoadOriginalDll()) return DllLoadFailed();

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
}

