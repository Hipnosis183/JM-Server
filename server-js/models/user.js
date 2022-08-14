import mongoose from 'mongoose';
const { model, Schema } = mongoose;

const UserSchema = new Schema({
  id: { type: String, required: true, maxlength: 16 },
  pass: { type: String, maxlength: 16 },
  rankings: [{ type: Object }],
});

export const User = model('user', UserSchema);