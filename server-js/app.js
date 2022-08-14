// Initialize Express.js
import express from 'express';
const app = express();

// Server constants.
const hostname = '127.0.0.1';
const hostport = '8081';

// Manage database initialization.
import mongoose from 'mongoose';
const mongoDB = `mongodb://${hostname}/jm`;
mongoose.connect(mongoDB, { useNewUrlParser: true, useUnifiedTopology: true });
mongoose.connection.on('error', (e) => { console.error.bind(console, e) });

// Setup server routing.
import * as Router from './routes.js';
app.use('/', Router.router);

// Start Jewelry Master server.
import fs from 'node:fs';
import path from 'path';
app.listen(hostport, () => {
  console.log(`Server for Jewelry Master created on http://${hostname}:${hostport}/`);
  // Create directory for replays storage.
  if (!fs.existsSync(path.resolve() + '/rep')) {
    fs.mkdir(path.resolve() + '/rep', (e) => { if (e) { throw e; } });
  }
});