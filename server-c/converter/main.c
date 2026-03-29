#include "./main.h"

int main(int argc, char** argv) {
  // Initialize databases.
  databasesInit();
  // Convert users database.
  converterUsers();
  // Convert rankings database and replay files.
  converterRankings(argc, argv);
  // Close databases.
  databasesClose();
  return 0;
}