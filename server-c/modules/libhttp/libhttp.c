// libhttp: libsocket HTTP extension library.
// Version 1.0 by Renzo Pigliacampo.
// libsocket 1.1

#include "libhttp.h"

// Get header value.
static struct ls_http_header ls_http_header(char* data, char* name) {
  // Find header position.
  struct ls_http_header header = { NULL, 0 };
  int length = strlen(name);
  for (int i = 0; i + length <= LS_HTTP_HEADER; i++) {
    if (memcmp(data + i, name, length) == 0) {
      header.data = data + (i + length + 1);
      // Skip space character if present.
      if (*header.data == ' ') {
        header.data++;
      }
      // Get header value length.
      header.size = strchr(header.data, '\r') - header.data;
      break;
    }
  }
  return header;
}

// Internal handler for HTTP messages.
static void ls_http_internal(struct ls_connection* connection, ls_event event, void* data, int size, void* userdata) {
  // Skip connection initialization.
  if (event == LS_EVENT_INIT) { return; }
  // Define connection state data.
  struct state {
      ls_http_handler handler;
      struct buffer {
        char* data;
        int size;
        int max;
        int length;
      } buffer;
      struct headers {
        int length;
        int parsed;
      } headers;
  } *state = connection->userdata;
  // Handle connection accept event.
  if (event == LS_EVENT_ACCEPT) {
    // Initialize and persist state structure on connection.
    state = (struct state*) calloc(1, sizeof(struct state));
    state->buffer.data = (char*) calloc(LS_HTTP_BUFFER, sizeof(char));
    state->buffer.max = state->buffer.data ? LS_HTTP_BUFFER : 0;
    state->handler = (ls_http_handler) connection->listener->userdata;
    connection->userdata = state;
    return;
  }
  // Return if state is not defined.
  if (!state || !state->handler) { return; }
  // Handle connection close event.
  if (event == LS_EVENT_CLOSE) {
    state->handler(connection, LS_EVENT_HTTP_CLOSE, NULL, 0);
    // Clear allocated memory.
    free(state->buffer.data);
    free(state);
    connection->userdata = NULL;
  }
  // Handle data reception event.
  if (event == LS_EVENT_RECV) {
    // Extend buffer size.
    if (state->buffer.size + size > state->buffer.max) {
      int max = state->buffer.max * 2;
      state->buffer.data = (char*) realloc(state->buffer.data, max);
      state->buffer.max = max;
    }
    // Append new data chunk.
    memcpy(state->buffer.data + state->buffer.size, data, size);
    state->buffer.size += size;
    // Parse HTTP headers.
    if (!state->headers.parsed) {
      for (int i = 0; i + 4 <= state->buffer.size; i++) {
        if (memcmp(state->buffer.data + i, "\r\n\r\n", 4) == 0) {
          state->headers.length = (state->buffer.data + i + 4) - state->buffer.data;
          state->headers.parsed = 1;
          // Get the 'Content-Length' header.
          struct ls_http_header header = ls_http_header(state->buffer.data, "Content-Length");
          char length[header.size];
          memcpy(length, header.data, header.size);
          state->buffer.length = strtol(length, NULL, 10);
          break;
        }
      }
    }
    // Return buffer once all data has been read.
    if (state->headers.parsed) {
      if (state->buffer.size - state->headers.length >= state->buffer.length) {
        state->handler(connection, LS_EVENT_HTTP_MSG, state->buffer.data, state->buffer.size);
      }
    }
  }
}

// Create connection listener.
struct ls_listener* ls_http_listen(struct ls_manager* manager, const char* port, ls_http_handler handler) {
  return ls_listen(manager, port, ls_http_internal, handler);
}

// Convert hex characters to integer values.
static int ls_http_hex(char byte) {
  if (byte >= '0' && byte <= '9') { return byte - '0'; }
  if (byte >= 'a' && byte <= 'f') { return byte - 'a' + 10; }
  if (byte >= 'A' && byte <= 'F') { return byte - 'A' + 10; }
  return -1;
}

// Get query param value for a given key.
void ls_http_param(char* query, char* key, char* data, int size) {
  // Get key and value sizes.
  int key_size = strlen(key);
  int value_size = size;
  data[0] = '\0';
  // Loop through the query params string.
  while (query && *query) {
    // Find the start of the next key-value pair.
    char* start = query;
    query = strchr(query, '&');
    if (query) {
      *query++ = '\0';
    }
    // Check if the key matches.
    if (strncmp(start, key, key_size) == 0 && start[key_size] == '=') {
      char* value = start + key_size + 1;
      char* destination = data;
      int pointer = 0;
      // Copy value data.
      while (*value && pointer < value_size - 1) {
        if (*value == '%') {
          if (isxdigit(value[1]) && isxdigit(value[2])) {
            int value_1 = ls_http_hex(value[1]);
            int value_2 = ls_http_hex(value[2]);
            *destination++ = (char) (value_1 * 16 + value_2);
            value += 3;
          } else {
            *destination++ = '?';
            value++;
          }
        } else if (*value == '+') {
          *destination++ = ' ';
          value++;
        } else {
          *destination++ = *value++;
        }
        pointer++;
      }
      *destination = '\0';
      // Restore '&' character.
      if (query) {
        *(query - 1) = '&';
      }
      return;
    }
    // Restore '&' character.
    if (query) {
      *(query - 1) = '&';
    }
  }
}

// Parse received request.
void ls_http_parse(char* data, int size, struct ls_http_request* request) {
  // Initialize request data.
  memset(request, 0, sizeof(struct ls_http_request));
  // Parse request method.
  request->method = data;
  char* pointer = (char*) memchr(data, ' ', size);
  *pointer++ = '\0';
  // Parse request URI and query params.
  request->uri = pointer;
  char* query_start = (char*) memchr(pointer, '?', pointer - data < size ? size - (pointer - data) : 0);
  char* uri_end = (char*) memchr(pointer, ' ', pointer - data < size ? size - (pointer - data) : 0);
  *uri_end = '\0';
  if (query_start && query_start < uri_end) {
    *query_start++ = '\0';
    request->query = query_start;
  } else {
    request->query = NULL;
  }
  // Parse request body contents.
  char* body_start = strstr(uri_end + 1, "\r\n\r\n");
  if (body_start) {
    request->body = body_start + 4;
    request->size = size - (request->body - data);
  } else {
    request->body = NULL;
    request->size = 0;
  }
}

// Get header property value.
static struct ls_http_header ls_http_header_value(char* data, int size, char* name) {
  // Find property position.
  struct ls_http_header header = { NULL, 0 };
  for (int i = 0; strlen(name) > 0 && i + strlen(name) + 2 < size; i++) {
    if (data[i + strlen(name)] == '=' && memcmp(&data[i], name, strlen(name)) == 0) {
      char* p = &data[i + strlen(name) + 1], *b = p, *x = &data[size];
      int q = p < x && *p == '"' ? 1 : 0;
      while (p < x && (q ? p == b || *p != '"' : *p != ';' && *p != ' ' && *p != ',')) {
        p++;
      }
      // Remove quotes from string.
      int l = p - b + q;
      if (l > 1 && b[0] == '"' && b[l - 1] == '"') {
        b = b + 1;
        l = l - 2;
      }
      // Set property data.
      header.data = b;
      header.size = l;
      break;
    }
  }
  return header;
}

// Parse multipart chunks.
int ls_http_multipart(struct ls_http_request* request, int offset, struct ls_http_part* part) {
  char* body = request->body;
  int pointer = offset, start, end, max = request->size;
  // Initialize part structure.
  if (part != NULL) {
    memset(part, 0, sizeof(struct ls_http_part));
  }
  // Skip boundary section.
  while (pointer + 2 < max && body[pointer] != '\r' && body[pointer + 1] != '\n') {
    pointer++;
  }
  if (pointer <= offset || pointer + 2 > max) { return 0; }
  // Get the 'Content-Disposition' header and properties.
  struct ls_http_header header = ls_http_header(body, "Content-Disposition");
  part->name = ls_http_header_value(header.data, header.size, "name");
  part->filename = ls_http_header_value(header.data, header.size, "filename");
  // Skip header section.
  for (int i = 0; i + 4 <= max; i++) {
    if (memcmp(body + i, "\r\n\r\n", 4) == 0) {
      start = end = i + 4;
      break;
    }
  }
  // Find end of body contents.
  while (end + 2 + (pointer - offset) + 2 < max && !(body[end] == '\r' && body[end + 1] == '\n' && memcmp(&body[end + 2], body, pointer - offset) == 0)) {
    end++;
  }
  if (end + 2 > max) { return 0; }
  // Set body data pointer and size.
  if (part != NULL) {
    part->body = &body[start];
    part->size = end - start;
  }
  // Return offset to next chunk.
  return end + 2;
}

// Send response data.
void ls_http_send(struct ls_connection* connection, int status, char* data) {
  // Define response status text.
  char* text = "Unknown Status";
  switch (status) {
    case 200: { text = "OK"; break; }
    case 201: { text = "Created"; break; }
    case 204: { text = "No Content"; break; }
    case 400: { text = "Bad Request"; break; }
    case 404: { text = "Not Found"; break; }
    case 500: { text = "Internal Server Error"; break; }
  }
  // Define response data size.
  int data_size = data ? strlen(data) : 0;
  // Define response header data.
  char header[256];
  int header_size = snprintf(header, sizeof(header), "HTTP/1.1 %d %s\r\n" "Content-Type: text/html\r\n" "Content-Length: %d\r\n" "Connection: close\r\n\r\n", status, text, data_size);
  // Send HTTP header packet.
  ls_send(connection, header, header_size);
  if (data_size > 0) {
    // Send HTTP data packet.
    ls_send(connection, data, data_size);
  }
}

// Send file as response.
void ls_http_serve(struct ls_connection* connection, char* filepath, char* type) {
  FILE *file = fopen(filepath, "rb");
  // Get file size.
  fseek(file, 0, SEEK_END);
  unsigned int size = ftell(file);
  fseek(file, 0, SEEK_SET);
  // Define and send headers.
  char headers[256];
  int n = snprintf(headers, sizeof(headers), "HTTP/1.1 200 OK\r\n" "Content-Type: %s\r\n" "Content-Length: %d\r\n" "Connection: close\r\n\r\n", type ? type : "application/octet-stream", size);
  ls_send(connection, headers, n);
  // Send file content in chunks.
  char data[LS_HTTP_BUFFER];
  int read;
  while ((read = fread(data, 1, sizeof(data), file)) > 0) {
    ls_send(connection, data, read);
  }
  fclose(file);
}