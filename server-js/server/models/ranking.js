import mongoose from 'mongoose';

// Define database model.
export const RankingModel = mongoose.model('ranking', new mongoose.Schema({
  rankingUser: String,
  rankingMode: String,
  rankingScore: Number,
  rankingJewel: String,
  rankingMode: String,
  rankingLevel: String,
  rankingClass: String,
  rankingTime: String,
  rankingDate: String,
}));