#ifndef HOOK_OPTIONS_H
#define HOOK_OPTIONS_H

#include <stdlib.h>
#include "libini/libini.h"

// Define server options.
extern struct Options {
  // Define server behavior.
  // 0. Server + Client: Runs the server and starts the game client.
  // 1. Server: Runs the server without client execution.
  // 2. Client: Disables server emulation and starts the game client.
  int serverMode;
  // Define server host.
  // Localhost: Connect to a local server. Ideal for single-player.
  // Private: Connect to a server over a LAN connection. Ideal for local play.
  // Public: Connect to a server over a WAN connection. Ideal for online play.
  char* serverHost;
} options;

// Initialize server options.
void optionsInit();

// Close server options.
void optionsClose();

#endif