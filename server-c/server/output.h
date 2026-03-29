#ifndef SERVER_OUTPUT_H
#define SERVER_OUTPUT_H

#include <stdio.h>
#include <time.h>
#include <winsock2.h>
#include "libsocket/libsocket.h"
#include "libhttp/libhttp.h"

// Print common output data.
static void outputHeader(char* address);

// Print server initialization data.
void outputInit();

// Print received request data.
void outputRequest(struct ls_connection* socket, struct ls_http_request request);

#endif