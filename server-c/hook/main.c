#include "./main.h"

BOOL WINAPI DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved) {
  switch (fdwReason) {
    case DLL_PROCESS_ATTACH: {
      // Initialize options.
      optionsInit();
      // Initialize hooking process.
      hookInit();
      break;
    }
    case DLL_PROCESS_DETACH: {
      // Close options.
      optionsClose();
      // Close hooking process.
      hookClose();
      break;
    }
  }
  return TRUE;
}