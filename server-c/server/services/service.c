#include "./service.h"

// Sort users by score in ascending order.
static int rankingSortScore(const void* a, const void* b) {
  struct RankingModel* _a = (struct RankingModel*) (*(ld_data**) a)->value;
  struct RankingModel* _b = (struct RankingModel*) (*(ld_data**) b)->value;
  return strtol(_b->rankingScore, NULL, 10) - strtol(_a->rankingScore, NULL, 10);
}

static int _rankingSortScore(const void* a, const void* b) {
  struct RankingModel* _a = (struct RankingModel*) *(struct RankingModel**) a;
  struct RankingModel* _b = (struct RankingModel*) *(struct RankingModel**) b;
  return strtol(_b->rankingScore, NULL, 10) - strtol(_a->rankingScore, NULL, 10);
}

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
void GameEntry(struct ls_connection* socket, struct ls_http_request data) {
  // Parse request data.
  struct request {
    char userId[17];
    char userPassword[17];
  } request;
  memset(&request, 0, sizeof(request));
  ls_http_param(data.query, "id", request.userId, 17);
  ls_http_param(data.query, "pass", request.userPassword, 17);
  // Check if user exists.
  ld_data* users = ld_find_one(&databaseUsers, 0, request.userId, 17);
  struct UserModel* user = (struct UserModel*) users->value;
  if (!users->size_value) {
    // Register new user if allowed.
    if (options._register) {
      user = calloc(sizeof(struct UserModel), sizeof(char));
      memcpy(user->userId, request.userId, 17);
      memcpy(user->userPassword, request.userPassword, 17);
      user->userCount = 0;
      user->userRanking = 0;
      ld_save(&databaseUsers, 0, user->userId, 17, user, sizeof(*user));
    } else {
      free(users);
      // Return error: wrong id or password.
      ls_http_send(socket, 200, "1"); return;
    }
  }
  if (strcmp(request.userPassword, user->userPassword) != 0) {
    free(user);
    free(users);
    // Return error: wrong id or password.
    ls_http_send(socket, 200, "1"); return;
  }
  // Send response data.
  ls_http_send(socket, 200, "");
  // Clear allocated memory.
  free(user);
  free(users);
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
void _GetMessage(struct ls_connection* socket, struct ls_http_request data) {
  // Manage notice message text.
  char* message = calloc(1, sizeof(char));
  if (options.noticeMode && options.noticeText != NULL) {
    message = realloc(message, strlen(options.noticeText) + 2);
    sprintf(message, "%s\n", options.noticeText);
  }
  // Send response data.
  ls_http_send(socket, 200, message);
  // Clear allocated memory.
  free(message);
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
void GetName(struct ls_connection* socket, struct ls_http_request data) {
  // Send response data.
  ls_http_send(socket, 200, "");
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
void GetRanking(struct ls_connection* socket, struct ls_http_request data) {
  // Parse request data.
  struct request {
    char rankingUser[17];
    char rankingMode[2];
    char rankingView[3];
  } request;
  memset(&request, 0, sizeof(request));
  ls_http_param(data.query, "id", request.rankingUser, 17);
  ls_http_param(data.query, "mode", request.rankingMode, 2);
  ls_http_param(data.query, "view", request.rankingView, 3);
  // Manage personal rankings.
  if (strlen(request.rankingUser) && strcmp(request.rankingView, "0") == 0) {
    ld_data* users = ld_find_one(&databaseUsers, 0, request.rankingUser, 17);
    struct UserModel* user = (struct UserModel*) users->value;
    char* scores = calloc(user->userCount * 100, sizeof(char));
    char* _scores = scores;
    // Get user rankings matching the requested mode.
    for (int i = 0; i < user->userCount; i++) {
      struct RankingModel* ranking = (struct RankingModel*) &user->userRanking + i;
      if (strcmp(ranking->rankingMode, request.rankingMode) == 0) {
        // Format ranking data.
        if (*scores != '\0') {
          *_scores = '.';
          _scores++;
        }
        sprintf(_scores, "0\n%s\n%s\n%s\n0\n%s\n%d\n%s\n%s\n%d", user->userId, user->userId, ranking->rankingScore, ranking->rankingLevel, 0, ranking->rankingTime, ranking->rankingDate, 1);
        _scores = strrchr(_scores, '\0');
      }
    }
    // Send response data.
    ls_http_send(socket, 200, scores);
    // Clear allocated memory.
    free(scores);
    free(users);
  }
  // Manage global rankings.
  else {
    lv_vector* rankings = ld_find(&databaseRankings, 0, NULL, 0);
    if (rankings->count) {
      // Filter rankings for the selected mode.
      for (int i = 0; i < rankings->count;) {
        struct RankingModel* ranking = (struct RankingModel*) ((ld_data*) rankings->data[i])->value;
        if (strcmp(ranking->rankingMode, request.rankingMode) != 0) {
          lv_erase(rankings, i);
        } else {
          i++;
        }
      }
      // Sort rankings by score.
      qsort(rankings->data, rankings->count, sizeof(struct RankingModel*), rankingSortScore);
      // Set rankings table index.
      int index = strcmp(request.rankingView, "-1") == 0 ? 0 : (int) strtol(request.rankingView, NULL, 10);
      if (strlen(request.rankingUser)) {
        // Get user score position in table.
        for (int i = 0; i < rankings->count; i++) {
          struct RankingModel* ranking = (struct RankingModel*) ((ld_data*) rankings->data[i])->value;
          if (strcmp(ranking->rankingUser, request.rankingUser) == 0) {
            index = floor(i / 10);
            break;
          }
        }
      }
      // Build table score page.
      char* scores = calloc(10 * 100, sizeof(char));
      char* _scores = scores;
      for (int i = index * 10; i < index * 10 + 10; i++) {
        if (i >= rankings->count) {
          break;
        }
        struct RankingModel* ranking = (struct RankingModel*) ((ld_data*) rankings->data[i])->value;
        int accent = strcmp(ranking->rankingUser, request.rankingUser) == 0;
        int title = 0;
        switch (request.rankingMode[0]) {
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
        if (*scores != '\0') {
          *_scores = '.';
          _scores++;
        }
        if (!options.multiScores) {
          sprintf(_scores, "%d\n%s\n%s\n%s\n0\n%s\n%d\n%s\n%s\n%d", index, ranking->rankingUser, ranking->rankingUser, ranking->rankingScore, ranking->rankingLevel, title, ranking->rankingTime, ranking->rankingDate, accent);
        } else {
          sprintf(_scores, "%d\n%s_%s\n%s\n%s\n0\n%s\n%d\n%s\n%s\n%d", index, ((ld_data*) rankings->data[i])->key, ranking->rankingScore, ranking->rankingUser, ranking->rankingScore, ranking->rankingLevel, title, ranking->rankingTime, ranking->rankingDate, 0);
        }
        _scores = strrchr(_scores, '\0');
      }
      // Send response data.
      ls_http_send(socket, 200, scores);
      // Clear allocated memory.
      free(scores);
    } else {
      // Send response data.
      ls_http_send(socket, 200, "");
    }
    // Clear allocated memory.
    ld_free(rankings);
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
void GetReplay(struct ls_connection* socket, struct ls_http_request data) {
  if (!options.multiScores) {
    // Parse request data.
    struct request {
      char rankingUser[17];
      char rankingMode[2];
    } request;
    memset(&request, 0, sizeof(request));
    ls_http_param(data.query, "id", request.rankingUser, 17);
    ls_http_param(data.query, "mode", request.rankingMode, 2);
    // Send response data.
    char replayName[64];
    sprintf(replayName, "./server/rep/%s%s.rep", request.rankingUser, request.rankingMode);
    ls_http_serve(socket, replayName, NULL);
  } else {
    // Parse request data.
    struct request {
      char rankingId[32];
    } request;
    memset(&request, 0, sizeof(request));
    ls_http_param(data.query, "id", request.rankingId, 32);
    // Send response data.
    char replayName[64];
    sprintf(replayName, "./server/rep/%s.rep", request.rankingId);
    ls_http_serve(socket, replayName, NULL);
  }
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
void ScoreEntry(struct ls_connection* socket, struct ls_http_request data) {
  // Parse request data.
  struct request {
    char rankingUser[17];
    char rankingMode[2];
    char rankingScore[10];
    char rankingJewel[5];
    char rankingLevel[4];
    char rankingClass[4];
    char rankingTime[7];
  } request;
  memset(&request, 0, sizeof(request));
  ls_http_param(data.query, "id", request.rankingUser, 17);
  ls_http_param(data.query, "mode", request.rankingMode, 2);
  ls_http_param(data.query, "score", request.rankingScore, 10);
  ls_http_param(data.query, "jewel", request.rankingJewel, 5);
  ls_http_param(data.query, "level", request.rankingLevel, 4);
  ls_http_param(data.query, "class", request.rankingClass, 4);
  ls_http_param(data.query, "time", request.rankingTime, 7);
  // Generate unique key for ranking entry.
  char rankingKey[18];
  memset(rankingKey, 0, sizeof(rankingKey));
  sprintf(rankingKey, "%s%s", request.rankingUser, request.rankingMode);
  // Get current datetime.
  char rankingDate[20];
  time_t datetime = time(NULL);
  strftime(rankingDate, sizeof(rankingDate), "%Y/%m/%d %H:%M:%S", localtime(&datetime));
  // Manage global rankings and replays storage.
  ld_data* rankings = ld_find_one(&databaseRankings, 0, rankingKey, 18);
  struct RankingModel* ranking = (struct RankingModel*) rankings->value;
  // Update user score entry if already exists.
  if (rankings->size_value && !options.multiScores) {
    // Replace only if the new score is higher than the previous.
    if (strtol(request.rankingScore, NULL, 10) > strtol(ranking->rankingScore, NULL, 10)) {
      // Delete and replace previous replay file.
      char replayName[64];
      sprintf(replayName, "./server/rep/%s%s.rep", ranking->rankingUser, ranking->rankingMode);
      remove(replayName);
      sprintf(replayName, "./server/rep/%s%s.rep", request.rankingUser, request.rankingMode);
      struct ls_http_part part;
      ls_http_multipart(&data, 0, &part);
      FILE* replayFile = fopen(replayName, "w");
      fwrite(part.body, part.size, sizeof(char), replayFile);
      fclose(replayFile);
      // Update database entry.
      memcpy(ranking->rankingScore, request.rankingScore, 10);
      memcpy(ranking->rankingJewel, request.rankingJewel, 5);
      memcpy(ranking->rankingLevel, request.rankingLevel, 5);
      memcpy(ranking->rankingClass, request.rankingClass, 4);
      memcpy(ranking->rankingTime, request.rankingTime, 7);
      memcpy(ranking->rankingDate, rankingDate, 20);
      ld_save(&databaseRankings, 0, rankingKey, 18, ranking, sizeof(*ranking));
    }
  }
  // Add score entry if it's from a new user or multiple scores are enabled.
  else {
    // Store replay file.
    char replayName[64];
    if (!options.multiScores) {
      sprintf(replayName, "./server/rep/%s%s.rep", request.rankingUser, request.rankingMode);
    } else {
      sprintf(replayName, "./server/rep/%s%s_%s.rep", request.rankingUser, request.rankingMode, request.rankingScore);
    }
    struct ls_http_part part;
    ls_http_multipart(&data, 0, &part);
    FILE* replayFile = fopen(replayName, "w");
    fwrite(part.body, part.size, sizeof(char), replayFile);
    fclose(replayFile);
    // Store score entry in the rankings database.
    ranking = calloc(sizeof(struct RankingModel), sizeof(char));
    memcpy(ranking->rankingUser, request.rankingUser, 17);
    memcpy(ranking->rankingMode, request.rankingMode, 2);
    memcpy(ranking->rankingScore, request.rankingScore, 10);
    memcpy(ranking->rankingJewel, request.rankingJewel, 5);
    memcpy(ranking->rankingLevel, request.rankingLevel, 5);
    memcpy(ranking->rankingClass, request.rankingClass, 4);
    memcpy(ranking->rankingTime, request.rankingTime, 7);
    memcpy(ranking->rankingDate, rankingDate, 20);
    ld_save(&databaseRankings, 0, rankingKey, 18, ranking, sizeof(*ranking));
  }
  // Clear allocated memory.
  free(ranking);
  free(rankings);
  // Manage personal rankings.
  ld_data* users = ld_find_one(&databaseUsers, 0, request.rankingUser, 17);
  struct UserModel* user = (struct UserModel*) users->value;
  // Load user rankings and get total amount for the selected mode.
  int userCount = 0;
  lv_vector* userRanking = calloc(sizeof(lv_vector), sizeof(char));
  lv_init(userRanking, sizeof(struct RankingModel*));
  for (int i = 0; i < user->userCount; i++) {
    struct RankingModel* ranking = (struct RankingModel*) &user->userRanking + i;
    lv_push(userRanking, &ranking);
    if (strcmp(ranking->rankingMode, request.rankingMode) == 0) {
      userCount++;
    }
  }
  // Check if ranking slots are full for the selected mode.
  if (userCount == 10) {
    for (int i = userRanking->count - 1; i >= 0; i--) {
      struct RankingModel* ranking = userRanking->data[i];
      if (strcmp(ranking->rankingMode, request.rankingMode) == 0) {
        // Replace only if the new score is higher than the smallest one stored.
        if (strtol(request.rankingScore, NULL, 10) > strtol(ranking->rankingScore, NULL, 10)) {
          memcpy(ranking->rankingScore, request.rankingScore, 10);
          memcpy(ranking->rankingJewel, request.rankingJewel, 5);
          memcpy(ranking->rankingLevel, request.rankingLevel, 5);
          memcpy(ranking->rankingClass, request.rankingClass, 4);
          memcpy(ranking->rankingTime, request.rankingTime, 7);
          memcpy(ranking->rankingDate, rankingDate, 20);
          break;
        }
      }
    }
  }
  // Add new score entry for the selected user.
  else {
    struct RankingModel* ranking = calloc(sizeof(struct RankingModel), sizeof(char));
    memcpy(ranking->rankingUser, request.rankingUser, 17);
    memcpy(ranking->rankingMode, request.rankingMode, 2);
    memcpy(ranking->rankingScore, request.rankingScore, 10);
    memcpy(ranking->rankingJewel, request.rankingJewel, 5);
    memcpy(ranking->rankingLevel, request.rankingLevel, 5);
    memcpy(ranking->rankingClass, request.rankingClass, 4);
    memcpy(ranking->rankingTime, request.rankingTime, 7);
    memcpy(ranking->rankingDate, rankingDate, 20);
    lv_push(userRanking, &ranking);
  }
  // Sort rankings by score, to avoid sorting every time rankings are requested.
  qsort(userRanking->data, userRanking->count, sizeof(struct RankingModel*), _rankingSortScore);
  // Update user ranking data in database.
  struct _UserModel {
    char userId[17];
    char userPassword[17];
    int userCount;
    char userRanking[userRanking->count * sizeof(struct RankingModel)];
  } _user;
  memset(&_user, 0, sizeof(struct UserModel) - sizeof(char*) + userRanking->count * sizeof(struct RankingModel));
  memcpy(_user.userId, user->userId, 17);
  memcpy(_user.userPassword, user->userPassword, 17);
  memcpy(&_user.userCount, &userRanking->count, sizeof(int));
  for (int i = 0; i < userRanking->count; i++) {
    memcpy(&_user.userRanking[i * sizeof(struct RankingModel)], userRanking->data[i], sizeof(struct RankingModel));
  }
  ld_save(&databaseUsers, 0, user->userId, 17, &_user, sizeof(struct UserModel) - sizeof(char*) + userRanking->count * sizeof(struct RankingModel));
  // Clear allocated memory.
  for (int i = 0; i < userRanking->count; i++) {
    free(userRanking->data[i]);
  }
  lv_free(userRanking);
  free(userRanking);
  free(users);
  // Send response data.
  ls_http_send(socket, 200, "");
}