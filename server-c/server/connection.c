#include "./connection.h"

// Manage server connection.
void serverConnection(struct ls_http_handler* handle) {
  // Manage received/sent connection data.
  if (handle->event == LS_EVENT_HTTP_MSG) {
    // Get HTTP request from packet.
    struct ls_http_request request;
    ls_http_parse(handle->data, handle->size, &request);
    // Output packet data.
    outputRequest(handle->connection, request);
    // Endpoint parse.
    if (strcmp(request.uri, "/") == 0) {
      ls_http_send(handle->connection, 200, "");
    }
    else if (strcmp(request.uri, "/JM_test/service/GameEntry") == 0) {
      GameEntry(handle->connection, request);
    }
    else if (strcmp(request.uri, "/JM_test/service/GetMessage") == 0) {
      _GetMessage(handle->connection, request);
    }
    else if (strcmp(request.uri, "/JM_test/service/GetName") == 0) {
      GetName(handle->connection, request);
    }
    else if (strcmp(request.uri, "/JM_test/service/GetRanking") == 0) {
      GetRanking(handle->connection, request);
    }
    else if (strcmp(request.uri, "/JM_test/service/GetReplay") == 0) {
      GetReplay(handle->connection, request);
    }
    else if (strcmp(request.uri, "/JM_test/service/ScoreEntry") == 0) {
      ScoreEntry(handle->connection, request);
    }
    else {
      // Empty response on unidentified endpoint.
      ls_http_send(handle->connection, 200, "");
    }
    // Clear allocated memory.
    free(handle);
  }
}