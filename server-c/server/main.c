#include "./main.h"

// Define server variables.
static struct ls_manager server;
static struct lt_thread thread;

// Manage server termination.
static volatile int stop = 0;
static void serverStop() { stop = 1; }

// Polling server on main thread.
static void serverPoll(struct ls_connection* connection, ls_http_event event, void* data, int size) {
  // Prepare connection data.
  struct ls_http_handler* handle = (struct ls_http_handler*) malloc(sizeof(struct ls_http_handler));
  handle->connection = connection;
  handle->event = event;
  handle->data = data;
  handle->size = size;
  // Create and send task to worker thread.
  lt_insert(&thread, (void*) serverConnection, handle);
}

int main() {
  // Initialize options.
  optionsInit();
  // Initialize loader.
  if (options.serverMode != 1) {
    loaderInit();
  }
  // Initialize server.
  if (options.serverMode != 2) {
    // Initialize databases.
    databasesInit();
    // Initialize worker thread.
    lt_init(&thread);
    // Initialize connections.
    ls_init(&server);
    ls_http_listen(&server, "8081", serverPoll);
    // Initialize logging.
    outputInit();
    // Start server polling.
    signal(SIGINT, serverStop);
    while (!stop) {
      ls_poll(&server, 200);
      // Check if game process is still running.
      if (options.serverMode != 1 && loaderState()) {
        serverStop();
      }
    }
    // Close databases.
    databasesClose();
    // Close connections.
    ls_close(&server);
    // Close worker thread.
    lt_close(&thread);
  }
  // Close loader.
  if (options.serverMode != 1) {
    loaderClose();
  }
  // Close options.
  optionsClose();
  return 0;
}

/*
Requests
--------
GET
 > /JM_test/service/
   > GameEntry: User authentication.
   > GetMessage: Notice message text.
   > GetName: Nothing.
   > GetRanking: Users ranking list.
   > GetReplay: Get user replay data.
POST
 > /JM_test/service/
   > ScoreEntry: Send user replay data.
*/