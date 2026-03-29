#include "./options.h"

// Define server options.
struct Options options = { 0, NULL };

// Define options file instance.
static li_handler* optionsFile;

// Initialize server options.
void optionsInit() {
  // Load options file.
  optionsFile = li_init("server.ini");
  // Load options values.
  options = (struct Options) {
    strtol(li_find_one(optionsFile, "Connection", "ServerMode"), NULL, 10),
    li_find_one(optionsFile, "Connection", "ServerHost"),
  };
}

// Close server options.
void optionsClose() {
  // Close options file.
  li_close(optionsFile);
}