import fs from 'node:fs';
import path from 'path';
import { options } from '../options.js';
import { RankingModel } from '../models/ranking.js';
import { UserModel } from '../models/user.js';

/*
GET /JM_test/service/GameEntry: User authentication.

<<< REQUEST
-----------
Parameters:
  > game: ?
    > 0: ?
  > id: User ID
  > pass: User Password
  > ver: Game Version

>>> RESPONSE
------------
Content-Type: text/html
Body:
  > Result Code
    > Nothing: Connection Successful
    > 1: Authentication Error
    > 10: Connection Error
    > ?: Version Error
*/
export const GameEntry = (request, response) => {
  // Parse request data.
  const url = new URL(request.url, `http://${request.headers.host}`);
  const query = {
    userId: url.searchParams.get('id'),
    userPassword: url.searchParams.get('pass'),
  };
  // Check if user exists.
  UserModel.findOne({ userId: query.userId }).then((user) => {
    if (!user) {
      // Register new user if allowed.
      if (options.register) {
        user = new UserModel({
          userId: query.userId,
          userPassword: query.userPassword,
          userRanking: [],
        });
        user.save();
      } else {
        // Return error: wrong id or password.
        response.end('1'); return;
      }
    }
    if (query.userPassword != user.userPassword) {
      // Return error: wrong id or password.
      response.end('1'); return;
    }
    // Send response data.
    response.end();
  });
}

/*
GET /JM_test/service/GetMessage: Notice message text.

<<< REQUEST
-----------
> Nothing

>>> RESPONSE
------------
Content-Type: text/html
Body:
  > Message Text
*/
export const GetMessage = (request, response) => {
  // Manage notice message text.
  const mensaje = options.noticeMode ? options.noticeText : '';
  // Send response data.
  response.end(mensaje);
}

/*
GET /JM_test/service/GetName: Nothing.

<<< REQUEST
-----------
Parameters:
  > id : User ID

>>> RESPONSE
------------
> Nothing
*/
export const GetName = (request, response) => {
  // Send response data.
  response.end();
}

/*
GET /JM_test/service/GetRanking: Users ranking list.

<<< REQUEST
-----------
Parameters:
  > view : Ranking Type
    > 0: Personal
    > -1: Global
  > id : User ID
  > mode : Ranking Difficulty
    > 0: Normal
    > 1: Hard
    > 2: Death

>>> RESPONSE
------------
Content-Type: text/html
Body:
  > Rankings List
    > Table Index
    > User ID
    > Ranking Name
    > Ranking Score
    > Nothing
    > Ranking Level
    > Ranking Title
      > 101: Knight Of Masters
      > 102: Knight
      > 201: King Of Masters
      > 202: King
      > 301: Death Master
      > 302: Death Knight Of Masters
      > 303: Death Knight
    > Ranking Time
    > Ranking Date
    > Ranking Highlight
*/
export const GetRanking = (request, response) => {
  // Parse request data.
  const url = new URL(request.url, `http://${request.headers.host}`);
  const query = {
    rankingUser: url.searchParams.get('id'),
    rankingMode: url.searchParams.get('mode'),
    rankingView: url.searchParams.get('view'),
  };
  // Manage personal rankings.
  if (query.rankingUser && query.rankingView == '0') {
    UserModel.findOne({ userId: query.rankingUser }).then((user) => {
      let scores = '';
      for (const ranking of user.userRanking) {
        if (ranking.rankingMode == query.rankingMode) {
          if (scores) {
            scores += '.';
          }
          scores += `0\n${user.userId}\n${user.userId}\n${ranking.rankingScore}\n0\n${ranking.rankingLevel}\n0\n${ranking.rankingTime}\n${ranking.rankingDate}\n1`;
        }
      }
      // Send response data.
      response.end(scores);
    });
  }
  // Manage global rankings.
  else {
    // Filter rankings for the selected mode and sort by score.
    RankingModel.find({ rankingMode: query.rankingMode }).sort({ rankingScore: -1 }).then((rankings) => {
      if (rankings.length) {
        // Set rankings table index.
        let index = query.rankingView == '-1' ? 0 : parseInt(query.rankingView);
        if (query.rankingUser) {
          // Get user score position in table.
          for (let i = 0; i < rankings.length; i++) {
            if (rankings[i].rankingUser == query.rankingUser) {
              index = Math.floor(i / 10);
              break;
            }
          }
        }
        // Build table score page.
        let scores = '';
        for (let i = index * 10; i < index * 10 + 10; i++) {
          if (i >= rankings.length) {
            break;
          }
          const ranking = rankings[i];
          const accent = +(ranking.rankingUser == query.rankingUser);
          let title = 0;
          switch (query.rankingMode) {
            case '0': {
              title = index == 0 ? (i == 0 ? 101 : 102) : (index == 1 && i < 15 ? 102 : 0);
              break;
            }
            case '1': {
              title = index == 0 ? (i == 0 ? 201 : 202) : (index == 1 && i < 15 ? 202 : 0);
              break;
            }
            case '2': {
              title = index == 0 ? (i == 0 ? 301 : i == 1 ? 302 : 303) : (index == 1 && i < 15 ? 303 : 0);
              break;
            }
          }
          // Format ranking data.
          if (scores) {
            scores += '.';
          }
          if (!options.multiScores) {
            scores += `${index}\n${ranking.rankingUser}\n${ranking.rankingUser}\n${ranking.rankingScore}\n0\n${ranking.rankingLevel}\n${title}\n${ranking.rankingTime}\n${ranking.rankingDate}\n${accent}`;
          } else {
            scores += `${index}\n${ranking.rankingUser}${ranking.rankingMode}_${ranking.rankingScore}\n${ranking.rankingUser}\n${ranking.rankingScore}\n0\n${ranking.rankingLevel}\n${title}\n${ranking.rankingTime}\n${ranking.rankingDate}\n0`;
          }
        }
        // Send response data.
        response.end(scores);
      } else {
        // Send response data.
        response.end();
      }
    });
  }
}

/*
GET /JM_test/service/GetReplay: Get user replay data.

<<< REQUEST
-----------
Parameters:
  > view : Ranking Type
    > 0: Personal
    > -1: Global
  > id : User ID
  > mode : Ranking Difficulty
    > 0: Normal
    > 1: Hard
    > 2: Death

>>> RESPONSE
------------
Content-Type: application/octet-stream
Body:
  > Replay Data
*/
export const GetReplay = (request, response) => {
  // Parse request data.
  const url = new URL(request.url, `http://${request.headers.host}`);
  const query = {
    rankingId: url.searchParams.get('id'),
    rankingMode: url.searchParams.get('mode'),
  };
  // Send response data.
  const replayName = `${path.resolve()}/server/rep/${options.multiScores ? query.rankingId : query.rankingId + query.rankingMode}.rep`;
  response.setHeader('Content-Type', 'application/octet-stream');
  response.setHeader('Content-Length', fs.statSync(replayName).size);
  fs.createReadStream(replayName).pipe(response);
}

/*
POST /JM_test/service/ScoreEntry: Send user replay data.

<<< REQUEST
-----------
Parameters:
  > id : User ID
  > mode : Ranking Difficulty
    > 0: Normal
    > 1: Hard
    > 2: Death
  > score: Ranking Score
  > jewel: Ranking Jewels
  > level: Ranking Level
  > class: ?
  > time: Ranking Time
  > noInfo: ?

>>> RESPONSE
------------
> Nothing
*/
export const ScoreEntry = (request, response) => {
  // Parse request data.
  const url = new URL(request.url, `http://${request.headers.host}`);
  const query = {
    rankingUser: url.searchParams.get('id'),
    rankingMode: url.searchParams.get('mode'),
    rankingScore: parseInt(url.searchParams.get('score')),
    rankingJewel: url.searchParams.get('jewel'),
    rankingLevel: url.searchParams.get('level'),
    rankingClass: url.searchParams.get('class'),
    rankingTime: url.searchParams.get('time'),
  };
  // Get current datetime.
  const rankingDate = new Date().toISOString().replace('T', ' ').substring(0, 19).replace(/-/g, '/');
  // Manage global rankings and replays storage.
  RankingModel.findOne({ rankingUser: query.rankingUser, rankingMode: query.rankingMode }).then((ranking) => {
    // Update user score entry if already exists.
    if (ranking && !options.multiScores) {
      // Replace only if the new score is higher than the previous.
      if (query.rankingScore > ranking.rankingScore) {
        // Delete and replace previous replay file.
        fs.writeFile(`${path.resolve()}/server/rep/${ranking.rankingUser}${ranking.rankingMode}.rep`, request.file.body, () => {});
        // Update database entry.
        ranking.rankingScore = query.rankingScore;
        ranking.rankingJewel = query.rankingJewel;
        ranking.rankingLevel = query.rankingLevel;
        ranking.rankingClass = query.rankingClass;
        ranking.rankingTime = query.rankingTime;
        ranking.rankingDate = rankingDate;
        ranking.save();
      }
    }
    // Add score entry if it's from a new user or multiple scores are enabled.
    else {
      // Store replay file.
      if (!options.multiScores) {
        fs.writeFile(`${path.resolve()}/server/rep/${query.rankingUser}${query.rankingMode}.rep`, request.file.body, () => {});
      } else {
        fs.writeFile(`${path.resolve()}/server/rep/${query.rankingUser}${query.rankingMode}_${query.rankingScore}.rep`, request.file.body, () => {});
      }
      // Store score entry in the rankings database.
      ranking = new RankingModel({
        rankingUser: query.rankingUser,
        rankingMode: query.rankingMode,
        rankingScore: query.rankingScore,
        rankingJewel: query.rankingJewel,
        rankingLevel: query.rankingLevel,
        rankingClass: query.rankingClass,
        rankingTime: query.rankingTime,
        rankingDate: rankingDate,
      });
      ranking.save();
    }
    // Manage personal rankings.
    UserModel.findOne({ userId: query.rankingUser }).then((user) => {
      // Load user rankings and get total amount for the selected mode.
      const userRanking = user.userRanking.filter((ranking) => ranking.rankingMode == query.rankingMode);
      // Check if ranking slots are full for the selected mode.
      if (userRanking.length == 10) {
        for (let i = userRanking.length - 1; i >= 0; i--) {
          const ranking = userRanking[i];
          if (ranking.rankingMode == query.rankingMode) {
            // Replace only if the new score is higher than the smallest one stored.
            if (query.rankingScore > ranking.rankingScore) {
              ranking.rankingScore = query.rankingScore;
              ranking.rankingJewel = query.rankingJewel;
              ranking.rankingLevel = query.rankingLevel;
              ranking.rankingClass = query.rankingClass;
              ranking.rankingTime = query.rankingTime;
              ranking.rankingDate = rankingDate;
              break;
            }
          }
        }
      }
      // Add new score entry for the selected user.
      else {
        const ranking = new RankingModel({
          rankingUser: query.rankingUser,
          rankingMode: query.rankingMode,
          rankingScore: query.rankingScore,
          rankingJewel: query.rankingJewel,
          rankingLevel: query.rankingLevel,
          rankingClass: query.rankingClass,
          rankingTime: query.rankingTime,
          rankingDate: rankingDate,
        });
        user.userRanking.push(ranking);
      }
      // Sort rankings by score, to avoid sorting every time rankings are requested.
      user.userRanking.sort((a, b) => { return b.rankingScore - a.rankingScore; });
      // Update user ranking data in database.
      user.save();
      // Send response data.
      response.end();
    });
  });
}