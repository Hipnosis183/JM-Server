// Jewelry Master Server Emulator by Renzo Pigliacampo (Hipnosis), 2022.
#include <stdio.h>
#include <windows.h>
#include <wininet.h>
#include "ini/ini.h"
#include "minhook/minhook.h"

// 0. Local Mode: Runs the server on 127.0.0.1 and starts the game client. Doesn't work for LAN connections.
// 1. Online Mode: Disables the local server emulation and connects to the 'HostName' address.
// 2. Host Mode A: Runs the server on 'HostName' without client execution. Opens a console with server information.
// 3. Host Mode B: Runs the server on 'HostName' with client execution. Ideal for hosting LAN servers.
static int SERVERMODE = 0;
// Set server host for connection.
static char HOSTNAME[16] = "127.0.0.1";

typedef int (WINAPI *INTERNETCONNECTA)(HINTERNET, LPCSTR, INTERNET_PORT, LPCSTR, LPCSTR, DWORD, DWORD, DWORD_PTR);
typedef int (WINAPI *INTERNETOPENURLA)(HINTERNET, LPCSTR, LPCSTR, DWORD, DWORD, DWORD_PTR);
// Define pointers to the original functions.
INTERNETCONNECTA fpInternetConnectA = NULL;
INTERNETOPENURLA fpInternetOpenUrlA = NULL;
// Define functions to overwrite the originals.
int WINAPI dInternetConnectA(HINTERNET hInternet, LPCSTR lpszServerName, INTERNET_PORT nServerPort, LPCSTR lpszUserName, LPCSTR lpszPassword, DWORD dwService, DWORD dwFlags, DWORD_PTR dwContext)
{
  // Change the original host with the configured IP address.
  return fpInternetConnectA(hInternet, (LPCSTR)HOSTNAME, nServerPort, lpszUserName, lpszPassword, dwService, dwFlags, dwContext);
}
int WINAPI dInternetOpenUrlA(HINTERNET hInternet, LPCSTR lpszUrl, LPCSTR lpszHeaders, DWORD dwHeadersLength, DWORD dwFlags, DWORD_PTR dwContext)
{
  // Change the original host with the configured IP address.
  char buf[200]; int ofs = 21; snprintf(buf, 200, "http://%s%s", HOSTNAME, lpszUrl + ofs);
  return fpInternetOpenUrlA(hInternet, buf, lpszHeaders, dwHeadersLength, dwFlags, dwContext);
}

BOOL WINAPI DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved)
{
  switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
      // Initialize MinHook.
      MH_Initialize();
      // Hook InternetConnect() and InternetOpenUrl() to redirect server calls to localhost.
      MH_CreateHookApiEx(L"wininet", "InternetConnectA", &dInternetConnectA, (LPVOID *)&fpInternetConnectA, NULL);
      MH_CreateHookApiEx(L"wininet", "InternetOpenUrlA", &dInternetOpenUrlA, (LPVOID *)&fpInternetOpenUrlA, NULL);
      MH_EnableHook(&InternetConnectA);
      MH_EnableHook(&InternetOpenUrlA);
      // Load configuration options from file.
      char dir[MAX_PATH], ini[MAX_PATH];
      GetCurrentDirectory(MAX_PATH, dir);
      snprintf(ini, MAX_PATH, "%s\\server.ini", dir);
      ini_t *config = ini_load(ini);
      if (config) { char *svr_p;
        const char *svr = ini_get(config, "Connection", "ServerMode");
        const char *hst = ini_get(config, "Connection", "HostName");
        if (svr) { SERVERMODE = strtol(svr, &svr_p, 10); }
        if (hst && SERVERMODE != 0) { snprintf(HOSTNAME, 16, hst); }
        ini_free(config);
      } break;
    case DLL_THREAD_ATTACH: break;
    case DLL_THREAD_DETACH: break;
    case DLL_PROCESS_DETACH:
      // Disable hooks and close MinHook.
      MH_DisableHook(&InternetConnectA);
      MH_DisableHook(&InternetOpenUrlA);
      MH_Uninitialize(); break;
  } return TRUE;
}