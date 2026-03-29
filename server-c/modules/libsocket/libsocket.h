// libsocket: Sockets management library.
// Version 1.1 by Renzo Pigliacampo.

#ifndef LIBSOCKET_H
#define LIBSOCKET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

// Define data structures.
typedef enum {
  LS_EVENT_INIT,
  LS_EVENT_ACCEPT,
  LS_EVENT_RECV,
  LS_EVENT_CLOSE
} ls_event;

struct ls_manager;
struct ls_connection;
struct ls_listener;

typedef void(*ls_handler) (struct ls_connection* connection, ls_event event, void* data, int size, void* userdata);

struct ls_handler {
  struct ls_connection* connection;
  ls_event event;
  int size;
  char data[4096];
  void* userdata;
};

struct ls_recv {
  int size;
  char data[4096];
};

struct ls_listener {
  SOCKET socket;
  ls_handler handler;
  struct ls_manager* manager;
  struct ls_listener* next;
  void* userdata;
};

struct ls_connection {
  SOCKET socket;
  struct sockaddr_in address;
  struct ls_manager* manager;
  struct ls_listener* listener;
  struct ls_recv recv;
  struct ls_connection* next;
  void* userdata;
};

struct ls_manager {
  int startup;
  struct ls_listener* listeners;
  struct ls_connection* connections;
};

// Initialize connection manager.
void ls_init(struct ls_manager* manager);

// Create connection listener.
struct ls_listener* ls_listen(struct ls_manager* manager, const char* port, ls_handler handler, void* userdata);

// Process connection events.
void ls_poll(struct ls_manager* manager, int interval);

// Send data to client.
int ls_send(struct ls_connection* connection, const void* data, int size);

// Print buffer data formatted as hex string.
void ls_print(const void* data, int size);

// Close connection manager.
void ls_close(struct ls_manager* manager);

#endif