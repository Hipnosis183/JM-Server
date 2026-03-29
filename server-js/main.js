import http from 'http';
import { serverConnection } from './server/connection.js';
import { databasesInit } from './server/databases.js';
import { optionsInit } from './server/options.js';
import { outputInit } from './server/output.js';

(() => {
  // Initialize options.
  optionsInit();
  // Initialize databases.
  databasesInit();
  // Initialize connections.
  http.createServer(serverConnection).listen(8081);
  // Initialize logging.
  outputInit();
  // Don't crash server on exceptions.
  process.on('uncaughtException', (error) => { console.error(error); });
  // Manage server termination.
  process.on('SIGINT', () => { process.exit(0); });
})();

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