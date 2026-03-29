#ifndef SERVER_CONNECTION_H
#define SERVER_CONNECTION_H

#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include "libsocket/libsocket.h"
#include "libhttp/libhttp.h"
#include "libvector/libvector.h"
#include "./output.h"
#include "./services/service.h"

// Manage server connection.
void serverConnection(struct ls_http_handler* handle);

#endif