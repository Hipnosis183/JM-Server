import mongoose from 'mongoose';
const { model, Schema } = mongoose;

const RankingSchema = new Schema({
  id: { type: String, required: true, maxlength: 16 },
  // 0: Normal | 1: Hard | 2: Death
  mode: { type: Number, required: true, maxlength: 1 },
  score: { type: Number, required: true, maxlength: 10 },
  jewel: { type: Number, required: true, maxlength: 4 },
  level: { type: Number, required: true, maxlength: 3 },
  // 101(0x65): Knight Of Masters | 102(0x66): Knight
  // 201(0xC9): King Of Masters | 202(0xCA): King
  // 301(0x12D): Death Master | 302(0x12E): Death Knight Of Masters | 303(0x12F): Death Knight
  class: { type: Number, required: true, maxlength: 3 }, // Assuming class is title.
  time: { type: Number, required: true, maxlength: 16 },
});

export const Ranking = model('ranking', RankingSchema);