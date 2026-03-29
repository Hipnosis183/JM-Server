#include "./hook.h"

// Define function pointers.
static INTERNETCONNECTA InternetConnectAPointer = NULL;
static INTERNETOPENURLA InternetOpenUrlAPointer = NULL;

// Detour 'InternetConnectA' function.
static HINTERNET WINAPI InternetConnectADetour(HINTERNET hInternet, LPCSTR lpszServerName, INTERNET_PORT nServerPort, LPCSTR lpszUserName, LPCSTR lpszPassword, DWORD dwService, DWORD dwFlags, DWORD_PTR dwContext) {
  // Load custom server host.
  return InternetConnectAPointer(hInternet, (LPCSTR) options.serverHost, nServerPort, lpszUserName, lpszPassword, dwService, dwFlags, dwContext);
}

// Detour 'InternetOpenUrlA' function.
static HINTERNET WINAPI InternetOpenUrlADetour(HINTERNET hInternet, LPCSTR lpszUrl, LPCSTR lpszHeaders, DWORD dwHeadersLength, DWORD dwFlags, DWORD_PTR dwContext) {
  // Load custom server host.
  char serverHost[1024];
  snprintf(serverHost, 1024, "http://%s%s", options.serverHost, lpszUrl + 21);
  return InternetOpenUrlAPointer(hInternet, serverHost, lpszHeaders, dwHeadersLength, dwFlags, dwContext);
}

// Initialize hooking process.
void hookInit() {
  // Initialize MinHook.
  MH_Initialize();
  // Hook functions.
  MH_CreateHookApiEx(L"wininet", "InternetConnectA", &InternetConnectADetour, (LPVOID*) &InternetConnectAPointer, NULL);
  MH_CreateHookApiEx(L"wininet", "InternetOpenUrlA", &InternetOpenUrlADetour, (LPVOID*) &InternetOpenUrlAPointer, NULL);
  MH_EnableHook(&InternetConnectA);
  MH_EnableHook(&InternetOpenUrlA);
}

// Close hooking process.
void hookClose() {
  // Disable hooks and close MinHook.
  MH_DisableHook(&InternetConnectA);
  MH_DisableHook(&InternetOpenUrlA);
  MH_Uninitialize();
}