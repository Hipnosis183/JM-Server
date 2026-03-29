#include "./databases.h"

// Define databases.
struct ld_db database;
struct ld_db databaseUsers;
struct ld_db databaseRankings;

// Initialize database connections.
void databasesInit() {
  SHFileOperation(&(SHFILEOPSTRUCT) { 0, FO_MOVE, "server\0", "server.bak\0", FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT });
  CreateDirectory("./server", NULL);
  CreateDirectory("./server/db", NULL);
  CreateDirectory("./server/rep", NULL);
  char* _database[] = { "user", "ranking" };
  char* _databaseUsers[] = { "users" };
  char* _databaseRankings[] = { "rankings" };
  ld_init(&database, "./server.bak/db", 2, _database, 0);
  ld_init(&databaseUsers, "./server/db/users", 1, _databaseUsers, 0);
  ld_init(&databaseRankings, "./server/db/rankings", 1, _databaseRankings, MDB_CREATE | MDB_DUPSORT);
}

// Close database connections.
void databasesClose() {
  ld_close(&database);
  ld_close(&databaseUsers);
  ld_close(&databaseRankings);
  SHFileOperation(&(SHFILEOPSTRUCT) { 0, FO_DELETE, "server.bak\0", NULL, FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT });
}