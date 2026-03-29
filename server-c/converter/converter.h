#ifndef CONVERTER_CONVERTER_H
#define CONVERTER_CONVERTER_H

#include <windows.h>
#include "libdb/libdb.h"
#include "./databases.h"
#include "../server/models/ranking.h"
#include "../server/models/user.h"

// Convert users database.
void converterUsers();

// Convert rankings database and replay files.
void converterRankings(int argc, char** argv);

#endif