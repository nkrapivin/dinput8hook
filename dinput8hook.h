// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the DINPUT8HOOK_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// DINPUT8HOOK_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#include "pch.h"
#include "framework.h"
#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION (0x0800)
#endif
#include <combaseapi.h>
#include <dinput.h>
#include <ShlObj.h>

#ifdef DINPUT8HOOK_EXPORTS
#define DINPUT8HOOK_API __declspec(dllexport)
#else
#define DINPUT8HOOK_API __declspec(dllimport)
#endif

typedef HRESULT(WINAPI * DirectInput8Create_t)(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter);
typedef HRESULT(WINAPI * DllCanUnloadNow_t)();
typedef HRESULT(WINAPI * DllGetClassObject_t)(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ LPVOID FAR* ppv);
typedef HRESULT(WINAPI * DllRegisterServer_t)();
typedef HRESULT(WINAPI * DllUnregisterServer_t)();
typedef LPCDIDATAFORMAT(WINAPI * GetdfDIJoystick_t)();
typedef HRESULT(WINAPI * SetCooperativeLevel_t)(LPDIRECTINPUTDEVICE8W self, HWND hwnd, DWORD dwflags);
typedef HRESULT(WINAPI * CreateDevice_t)(LPDIRECTINPUT8W self, REFGUID rguid, LPDIRECTINPUTDEVICE8W* lpdid, LPUNKNOWN lpunk);

BOOL TryUnloadDll();

/* hooks: */
HRESULT         WINAPI DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter);
__control_entrypoint(DllExport)
STDAPI                 DllCanUnloadNow();
_Check_return_
STDAPI                 DllGetClassObject(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ LPVOID FAR* ppv);
HRESULT         WINAPI DllRegisterServer();
HRESULT         WINAPI DllUnregisterServer();
LPCDIDATAFORMAT WINAPI GetdfDIJoystick();