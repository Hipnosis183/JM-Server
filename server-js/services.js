import fs from 'node:fs';
import path from 'path';

// Import database models.
import { Ranking } from './models/ranking.js';
import { User } from './models/user.js';

// Server connection request.
export const index = (req, res) => {
  res.statusCode = 200;
  res.send();
}

const options = {
  // Allow unregistered users to be registered at the login screen.
  register: true,
  // Allow users to have mutiple scores (and replays) in the global rankings.
  // Don't change once the database has already been created, will break scores.
  // Won't work if a score doesn't make it to the player's top 10 ranking.
  multiScores: false,
  // Disable scores and replays saving.
  noScores: false
};

// Authenticate user. Used for login, getting rankings and starting games.
// Response: '1': Auth error | '10': Connection error | ?: Version error.
// Params: 'game', 'id', 'pass', 'ver'.
export const gameEntry = (req, res) => {
  console.log('GameEntry: ', req.query);
  res.statusCode = 200;
  User.findOne({ id: req.query.id, pass: req.query.pass }, (e, user) => {
    // Check if user exists and the credentials are correct.
    if (user) { res.send(); }
    // Check for users with the same id and create a new one if allowed.
    else if (req.query.id.length > 0 && options.register) {
      User.findOne({ id: req.query.id }, (e, exists) => {
        if (!exists) {
          // Store new user into the database.
          const user = new User({ id: req.query.id, pass: req.query.pass, rankings: [] });
          user.save(); res.send();
        // An user with this id already exists.
        } else { res.send('1'); }
      });
    // Wrong user id or password.
    } else { res.send('1'); }
  });
};

// Get main menu message.
export const getMessage = (req, res) => {
  res.statusCode = 200;
  res.send('Jewelry Master Server Emulator by Hipnosis, 2022');
};

// Unkwnown usage. Probably unused?
// Params: 'id'.
export const getName = (req, res) => {
  res.statusCode = 200;
  res.send();
};

// Get rankings/leaderboards data.
// Params: 'id', 'mode', 'view'.
export const getRanking = (req, res) => {
  console.log('GetRanking:', req.query);
  res.statusCode = 200;
  // Manage personal rankings.
  if (req.query.id && req.query.view == '0') {
    User.findOne({ id: req.query.id }, (e, u) => {
      let ranks = [];
      // Sort rankings in descending order.
      let rankings = u.rankings.sort((a, b) => b.score - a.score);
      for (let r of rankings) {
        // Build response string.
        let lit = (r == 0) ? 1 : 0;
        let rank = `0\n0\n${r.id}\n${r.score}\n0\n${r.level}\n0\n${r.time}\n${r.jewel}\n${lit}`;
        // Add rankings for the selected mode.
        if (req.query.mode == r.mode) { ranks.push(rank); }
      }
      // Return concatenated strings.
      res.send(ranks.join('.'));
    });
  // Manage global rankings.
  } else {
    // Sort rankings in descending order.
    Ranking.find({ mode: req.query.mode }).sort({ score: -1 }).exec((e, r) => {
      if (r.length > 0) {
        let ranks = [], f = -1;
        // Set rankings table index.
        let index = req.query.view == '-1' ? 0 : req.query.view;
        if (req.query.id) {
          // Get user score position table index.
          f = r.findIndex((v) => v.id == req.query.id);
          if (f != -1) { index = Math.floor(f / 10); }
        }
        // Fill the 10-slots scores table.
        for (let i = (index * 10); i < (index * 10 + 10); i++) {
          if (!r[i]) { break; }
          // Build response string.
          let lit = (i == f) ? 1 : 0;
          ranks.push(`${index}\n${r[i]._id}\n${r[i].id}\n${r[i].score}\n0\n${r[i].level}\n${r[i].class}\n${r[i].time}\n${r[i].jewel}\n${lit}`);
        } res.send(ranks.join('.'));
      } else { res.send(); }
    });
  }
};

// Get replay for the selected score.
// Params: 'id', 'mode', 'view'.
export const getReplay = (req, res) => {
  console.log('GetReplay: ', req.query);
  res.download(path.resolve() + '/rep/' + req.query.id + '.rep');
};

// Send user score to rankings/leaderboards and replay data.
// Params: 'id', 'mode', 'score', 'jewel', 'level', 'class', 'time'.
export const scoreEntry = (req, res) => {
  console.log('ScoreEntry: ', req.query);
  res.statusCode = 200;
  // Cancel score entry.
  if (options.noScores) {
    fs.unlink(path.resolve() + '/rep/temp', (e) => { if (e) { throw e; } });
    res.send(); return;
  }
  // Define new score entry object.
  let entry = {
    id: req.query.id,
    mode: req.query.mode,
    score: req.query.score,
    jewel: req.query.jewel,
    level: req.query.level,
    class: req.query.class,
    time: req.query.time,
  };
  // Manage global rankings database and replays storage.
  Ranking.findOne({ id: req.query.id, mode: req.query.mode }, (e, r) => {
    // Update user score entry if already present.
    if (r && !options.multiScores) {
      // Replace only if the score is higher than the already stored.
      if (req.query.score > r.score) {
        Ranking.findOneAndUpdate({ id: req.query.id, mode: req.query.mode }, entry, {}, (e, u) => {
          // Delete previous replay file and replace it with the new one.
          fs.unlink(path.resolve() + '/rep/' + r._id + '.rep', (e) => { if (e) { throw e; } });
          fs.rename(path.resolve() + '/rep/temp', path.resolve() + '/rep/' + r._id + '.rep', (e) => { if (e) { throw e; } });
        });
      } else {
        // Delete unused replay file.
        fs.unlink(path.resolve() + '/rep/temp', (e) => { if (e) { throw e; } });
      }
    // Add score entry if it's from a new user or multiple scores are enabled.
    } else {
      // Create rank object from model.
      const rank = new Ranking({
        id: req.query.id,
        mode: req.query.mode,
        score: req.query.score,
        jewel: req.query.jewel,
        level: req.query.level,
        class: req.query.class,
        time: req.query.time,
      });
      // Store new score entry in the rankings database.
      rank.save((e, r) => {
        // Rename replay file with the newly created id.
        fs.rename(path.resolve() + '/rep/temp', path.resolve() + '/rep/' + r._id + '.rep', (e) => { if (e) { throw e; } });
      });
    }
  });
  // Manage personal rankings from the users database.
  User.findOne({ id: req.query.id }, (e, u) => {
    // Check if the user rankings slots are full.
    let ranking = u.rankings.filter((r) => r.mode == req.query.mode);
    if (ranking.length == 10) {
      // Sort rankings to get the smallest score for the selected mode.
      let rankings = u.rankings.sort((a, b) => a.score - b.score);
      let index = rankings.findIndex((r) => r.mode == req.query.mode);
      // Replace only if the score is higher than the smallest stored.
      if (req.query.score > rankings[index].score) {
        // Update score entry with the new stats.
        rankings[index] = entry;
        User.findOneAndUpdate({ id: req.query.id }, { rankings: rankings }, {}, () => { res.send(); });
      } else { res.send(); }
    } else {
      // Add new score entry to the user personal ranking.
      u.rankings.push(entry);
      User.findOneAndUpdate({ id: req.query.id }, { rankings: u.rankings }, {}, () => { res.send(); });
    }
  });
};