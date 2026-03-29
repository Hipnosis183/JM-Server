#ifndef SERVICES_SERVICE_H
#define SERVICES_SERVICE_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "libsocket/libsocket.h"
#include "libhttp/libhttp.h"
#include "libdb/libdb.h"
#include "../databases.h"
#include "../options.h"
#include "../models/ranking.h"
#include "../models/user.h"

// GET /JM_test/service/GameEntry: User authentication.
void GameEntry(struct ls_connection* socket, struct ls_http_request);

// GET /JM_test/service/GetMessage: Notice message text.
void _GetMessage(struct ls_connection* socket, struct ls_http_request data);

// GET /JM_test/service/GetName: Nothing.
void GetName(struct ls_connection* socket, struct ls_http_request data);

// GET /JM_test/service/GetRanking: Users ranking list.
void GetRanking(struct ls_connection* socket, struct ls_http_request data);

// GET /JM_test/service/GetReplay: Get user replay data.
void GetReplay(struct ls_connection* socket, struct ls_http_request data);

// POST /JM_test/service/ScoreEntry: Send user replay data.
void ScoreEntry(struct ls_connection* socket, struct ls_http_request data);

#endif