// libhttp: libsocket HTTP extension library.
// Version 1.0 by Renzo Pigliacampo.
// libsocket 1.1

#ifndef LIBHTTP_H
#define LIBHTTP_H

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../libsocket/libsocket.h"

// Define constants.
#define LS_HTTP_HEADER 4096
#define LS_HTTP_BUFFER 8192

// Define data structures.
typedef enum {
  LS_EVENT_HTTP_MSG,
  LS_EVENT_HTTP_CLOSE
} ls_http_event;

typedef void(*ls_http_handler) (struct ls_connection* connection, ls_http_event event, void* data, int size);

struct ls_http_handler {
  struct ls_connection* connection;
  ls_http_event event;
  char* data;
  int size;
};

struct ls_http_request {
  char* method;
  char* uri;
  char* query;
  char* body;
  int size;
};

struct ls_http_header {
  char* data;
  int size;
};

struct ls_http_part {
  struct ls_http_header name;
  struct ls_http_header filename;
  char* body;
  int size;
};

// Create connection listener.
struct ls_listener* ls_http_listen(struct ls_manager* manager, const char* port, ls_http_handler handler);

// Get query param value for a given key.
void ls_http_param(char* query, char* key, char* data, int size);

// Parse received request.
void ls_http_parse(char* data, int size, struct ls_http_request* request);

// Parse multipart chunks.
int ls_http_multipart(struct ls_http_request* request, int offset, struct ls_http_part* part);

// Send response data.
void ls_http_send(struct ls_connection* connection, int status, char* data);

// Send file as response.
void ls_http_serve(struct ls_connection* connection, char* file, char* type);

#endif