#ifndef SERVER_LOADER_H
#define SERVER_LOADER_H

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "./options.h"

// Inject custom library.
static void loaderHook(HANDLE handle, char* filepath);

// Initialize game process loader.
void loaderInit();

// Close game process loader.
void loaderClose();

// Get game process running state.
int loaderState();

#endif