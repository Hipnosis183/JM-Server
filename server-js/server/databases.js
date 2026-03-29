import fs from 'node:fs';
import path from 'path';
import mongoose from 'mongoose';

// Define databases.
const database = 'mongodb://127.0.0.1/jm';

// Initialize database connections.
export const databasesInit = () => {
  mongoose.connect(database);
  // Create directory for replays storage.
  fs.mkdir(path.resolve() + '/server/rep', { recursive: true }, () => {});
}