#ifndef SERVER_OPTIONS_H
#define SERVER_OPTIONS_H

#include <stdlib.h>
#include "libini/libini.h"

// Define server options.
extern struct Options {
  // Define server behavior.
  // 0. Server + Client: Runs the server and starts the game client.
  // 1. Server: Runs the server without client execution.
  // 2. Client: Disables server emulation and starts the game client.
  int serverMode;
  // Allow unregistered users to be registered at the login screen.
  int _register;
  // Allow users to have mutiple scores (and replays) in the global rankings.
  // Don't change once the database has already been created.
  int multiScores;
  // Define notice message text.
  // 0. No message text.
  // 1. Custom message text.
  int noticeMode;
  char* noticeText;
} options;

// Initialize server options.
void optionsInit();

// Close server options.
void optionsClose();

#endif