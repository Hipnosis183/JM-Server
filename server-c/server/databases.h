#ifndef SERVER_DATABASES_H
#define SERVER_DATABASES_H

#include <windows.h>
#include "libdb/libdb.h"
#include "./options.h"

// Define databases.
extern struct ld_db databaseUsers;
extern struct ld_db databaseRankings;

// Initialize database connections.
void databasesInit();

// Close database connections.
void databasesClose();

#endif