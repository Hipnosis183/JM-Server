#include "./databases.h"

// Define databases.
struct ld_db databaseUsers;
struct ld_db databaseRankings;

// Initialize database connections.
void databasesInit() {
  CreateDirectory("./server", NULL);
  CreateDirectory("./server/db", NULL);
  CreateDirectory("./server/rep", NULL);
  char* _databaseUsers[] = { "users" };
  char* _databaseRankings[] = { "rankings" };
  ld_init(&databaseUsers, "./server/db/users", 1, _databaseUsers, 0);
  ld_init(&databaseRankings, "./server/db/rankings", 1, _databaseRankings, options.multiScores ? (MDB_CREATE | MDB_DUPSORT) : MDB_CREATE);
}

// Close database connections.
void databasesClose() {
  ld_close(&databaseUsers);
  ld_close(&databaseRankings);
}