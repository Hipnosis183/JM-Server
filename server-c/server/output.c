#include "./output.h"

// Print common output data.
static void outputHeader(char* address) {
  char datetime[20];
  time_t _datetime = time(NULL);
  strftime(datetime, sizeof(datetime), "%Y/%m/%d %H:%M:%S", localtime(&_datetime));
  printf("\n%-20s %-15s > ", datetime, address);
}

// Print server initialization data.
void outputInit() {
  outputHeader("0.0.0.0");
  printf("INIT\n");
}

// Print received request data.
void outputRequest(struct ls_connection* socket, struct ls_http_request request) {
  outputHeader(inet_ntoa(socket->address.sin_addr));
  printf("%-6s %s\n", request.method, request.uri);
  if (request.query) {
    printf("%38s %-6s %s\n", ">", "QUERY", request.query);
  }
}