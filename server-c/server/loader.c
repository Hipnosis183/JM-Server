#include "./main.h"

// Define process variables.
static STARTUPINFO startupInfo;
static PROCESS_INFORMATION processInfo;
static HANDLE processHandle;
static DWORD processCode;

// Inject custom library.
static void loaderHook(HANDLE handle, char* filepath) {
  int size = strlen(filepath) + 1;
  LPVOID* buffer = VirtualAllocEx(handle, NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  WriteProcessMemory(handle, buffer, filepath, size, NULL);
  CreateRemoteThread(handle, NULL, 0, (LPTHREAD_START_ROUTINE) (LoadLibrary), buffer, 0, NULL);
}

// Initialize game process loader.
void loaderInit() {
  // Close console window.
  ShowWindow(GetConsoleWindow(), SW_HIDE);
  // Create process.
  ZeroMemory(&startupInfo, sizeof(startupInfo));
  ZeroMemory(&processInfo, sizeof(processInfo));
  startupInfo.cb = sizeof(startupInfo);
  CreateProcess("JM.exe", NULL, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &startupInfo, &processInfo);
  Sleep(500);
  // Inject custom library.
  loaderHook(processInfo.hProcess, "server.dll");
  Sleep(500);
  // Resume process.
  ResumeThread(processInfo.hThread);
  Sleep(500);
  HANDLE processHandle = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION, FALSE, processInfo.dwProcessId);
}

// Close game process loader.
void loaderClose() {
  // Wait for game process to finish.
  if (options.serverMode == 2) {
    WaitForSingleObject(processInfo.hProcess, INFINITE);
  }
  CloseHandle(processHandle);
  CloseHandle(processInfo.hProcess);
  CloseHandle(processInfo.hThread);
}

// Get game process running state.
int loaderState() {
  return GetExitCodeProcess(processInfo.hProcess, &processCode) && processCode != STATUS_PENDING;
}