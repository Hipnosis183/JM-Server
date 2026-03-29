import mongoose from 'mongoose';

// Define database model.
export const UserModel = mongoose.model('user', new mongoose.Schema({
  userId: String,
  userPassword: String,
  userRanking: [],
}));