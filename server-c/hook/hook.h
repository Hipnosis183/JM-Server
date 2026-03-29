#ifndef HOOK_HOOK_H
#define HOOK_HOOK_H

#include <string.h>
#include <windows.h>
#include <wininet.h>
#include "minhook/minhook.h"
#include "./options.h"

// Define function types.
typedef HINTERNET (WINAPI* INTERNETCONNECTA) (HINTERNET hInternet, LPCSTR lpszServerName, INTERNET_PORT nServerPort, LPCSTR lpszUserName, LPCSTR lpszPassword, DWORD dwService, DWORD dwFlags, DWORD_PTR dwContext);
typedef HINTERNET (WINAPI* INTERNETOPENURLA) (HINTERNET hInternet, LPCSTR lpszUrl, LPCSTR lpszHeaders, DWORD dwHeadersLength, DWORD dwFlags, DWORD_PTR dwContext);

// Initialize hooking process.
void hookInit();

// Close hooking process.
void hookClose();

#endif