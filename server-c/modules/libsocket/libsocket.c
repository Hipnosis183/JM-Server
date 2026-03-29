// libsocket: Sockets management library.
// Version 1.1 by Renzo Pigliacampo.

#include "libsocket.h"

// Initialize connection manager.
void ls_init(struct ls_manager* manager) {
  manager->startup = 0;
  manager->listeners = NULL;
  manager->connections = NULL;
}

// Create connection listener.
struct ls_listener* ls_listen(struct ls_manager* manager, const char* port, ls_handler handler, void* userdata) {
  // Initialize Winsock.
  if (!manager->startup) {
    WSADATA data;
    WSAStartup(MAKEWORD(2, 2), &data);
    manager->startup = 1;
  }
  // Get address and port data.
  struct addrinfo hints;
  struct addrinfo* address = NULL;
  ZeroMemory(&hints, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags = AI_PASSIVE;
  getaddrinfo(NULL, port, &hints, &address);
  // Create, bind and listen connection socket.
  SOCKET _socket = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
  bind(_socket, address->ai_addr, (int) address->ai_addrlen);
  listen(_socket, SOMAXCONN);
  freeaddrinfo(address);
  // Create listener object.
  struct ls_listener* listener = (struct ls_listener*) malloc(sizeof(struct ls_listener));
  listener->socket = _socket;
  listener->handler = handler;
  listener->manager = manager;
  listener->next = manager->listeners;
  listener->userdata = userdata;
  manager->listeners = listener;
  // Send event to handler.
  if (listener->handler) {
    listener->handler(NULL, LS_EVENT_INIT, listener, 0, NULL);
  }
  return listener;
}

// Process connection events.
void ls_poll(struct ls_manager* manager, int interval) {
  fd_set readfds;
  FD_ZERO(&readfds);
  // Add listener file descriptor.
  struct ls_listener* listener;
  for (listener = manager->listeners; listener != NULL; listener = listener->next) {
    FD_SET(listener->socket, &readfds);
  }
  // Add connection file descriptor.
  struct ls_connection* connection;
  for (connection = manager->connections; connection != NULL; connection = connection->next) {
    FD_SET(connection->socket, &readfds);
  }
  // Define connection status.
  select(0, &readfds, NULL, NULL, &(const TIMEVAL) { interval / 1000, (interval % 1000) * 1000 });
  // Check for incoming connections on listeners.
  for (listener = manager->listeners; listener != NULL; listener = listener->next) {
    if (FD_ISSET(listener->socket, &readfds)) {
      struct sockaddr_in address;
      int addresslen = sizeof(address);
      SOCKET socket = accept(listener->socket, (struct sockaddr*) &address, &addresslen);
      if (socket != INVALID_SOCKET) {
        if (listener->handler) {
          struct ls_connection* _connection = (struct ls_connection*) malloc(sizeof(struct ls_connection));
          if (_connection) {
            _connection->socket = socket;
            _connection->address = address;
            _connection->manager = manager;
            _connection->listener = listener;
            _connection->userdata = listener->userdata;
            _connection->recv.size = 0;
            _connection->next = manager->connections;
            manager->connections = _connection;
            // Disable Nagle's algorithm.
            int option = 1;
            setsockopt(_connection->socket, IPPROTO_TCP, TCP_NODELAY, (char*) &option, sizeof(option));
            // Send event to handler.
            listener->handler(_connection, LS_EVENT_ACCEPT, NULL, 0, _connection->userdata);
          } else {
            // Connection error.
            closesocket(socket);
          }
        } else {
          // Accept connection and close socket immediately after.
          closesocket(socket);
        }
      }
    }
  }
  // Check for I/O on existing connections.
  struct ls_connection* prev = NULL;
  connection = manager->connections;
  while (connection != NULL) {
    struct ls_connection* next = connection->next;
    if (FD_ISSET(connection->socket, &readfds)) {
      // Read received data into buffer.
      int buffer = recv(connection->socket, connection->recv.data, sizeof(connection->recv.data), 0);
      if (buffer > 0) {
        connection->recv.size = buffer;
        // Send event to handler.
        connection->listener->handler(connection, LS_EVENT_RECV, connection->recv.data, connection->recv.size, connection->userdata);
      } else {
        // Send event to handler.
        connection->listener->handler(connection, LS_EVENT_CLOSE, NULL, 0, connection->userdata);
        // Connection closed.
        closesocket(connection->socket);
        if (prev == NULL) {
          manager->connections = next;
        } else {
          prev->next = next;
        }
        // Free connection memory automatically.
        // Disable to access the connection data inside 'LS_EVENT_CLOSE'.
        // Disabling requires to free and nullify manually.
        free(connection); connection = NULL;
      }
    }
    if (connection != NULL) {
      prev = connection;
    }
    connection = next;
  }
}

// Send data to client.
int ls_send(struct ls_connection* connection, const void* data, int size) {
  return send(connection->socket, (const char*) data, size, 0);
}

// Print buffer data formatted as hex string.
void ls_print(const void* data, int size) {
  const unsigned char* _data = (const unsigned char*) data;
  for (int i = 0; i < size; i++) {
    printf("%02X ", _data[i]);
  }
  printf("\n");
}

// Close connection manager.
void ls_close(struct ls_manager* manager) {
  // Clear connections.
  struct ls_connection* connection = manager->connections;
  while (connection != NULL) {
    struct ls_connection* next = connection->next;
    closesocket(connection->socket);
    free(connection);
    connection = next;
  }
  manager->connections = NULL;
  // Clear listeners.
  struct ls_listener* listener = manager->listeners;
  while (listener != NULL) {
    struct ls_listener* next = listener->next;
    closesocket(listener->socket);
    free(listener);
    listener = next;
  }
  manager->listeners = NULL;
  // Close Winsock.
  if (manager->startup) {
    WSACleanup();
    manager->startup = 0;
  }
}