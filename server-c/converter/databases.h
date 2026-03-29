#ifndef CONVERTER_DATABASES_H
#define CONVERTER_DATABASES_H

#include <windows.h>
#include <shellapi.h>
#include "libdb/libdb.h"

// Define databases.
extern struct ld_db database;
extern struct ld_db databaseUsers;
extern struct ld_db databaseRankings;

// Initialize database connections.
void databasesInit();

// Close database connections.
void databasesClose();

#endif