#include "./converter.h"

// Convert users database.
void converterUsers() {
  // Get list of users.
  lv_vector* users = ld_find(&database, 0, NULL, 0);
  for (int i = 0; i < users->count; i++) {
    char* cursor = ((ld_data*) users->data[i])->value;
    // Get user ranking count.
    int count = 0;
    sscanf(strstr(cursor, "\"count\":") + 8, "%d", &count);
    int size = count == 0 ? sizeof(char*) : count * sizeof(struct RankingModel);
    // Build user entry.
    struct _UserModel {
      char userId[17];
      char userPassword[17];
      int userCount;
      char userRanking[size];
    } _user;
    memset(&_user, 0, sizeof(struct UserModel) - sizeof(char*) + size);
    sscanf(strstr(cursor, "\"id\":") + 6, "%[^\"]", _user.userId);
    sscanf(strstr(cursor, "\"pass\":") + 8, "%[^\"]", _user.userPassword);
    memcpy(&_user.userCount, &count, sizeof(int));
    // Build user ranking entries.
    cursor = strstr(cursor, "\"rankings\":[");
    for (int k = 0; k < count; k++) {
      cursor = strchr(cursor, '{');
      if (!cursor) { break; }
      sscanf(strstr(cursor, "\"id\":") + 6, "%[^\"]", _user.userRanking + sizeof(struct RankingModel) * k + 0);
      sscanf(strstr(cursor, "\"mode\":") + 7, "%[^,}]", _user.userRanking + sizeof(struct RankingModel) * k + 17);
      sscanf(strstr(cursor, "\"score\":") + 8, "%[^,}]", _user.userRanking + sizeof(struct RankingModel) * k + 19);
      sscanf(strstr(cursor, "\"jewel\":") + 8, "%[^,}]", _user.userRanking + sizeof(struct RankingModel) * k + 29);
      sscanf(strstr(cursor, "\"level\":") + 8, "%[^,}]", _user.userRanking + sizeof(struct RankingModel) * k + 34);
      sscanf(strstr(cursor, "\"class\":") + 8, "%[^,}]", _user.userRanking + sizeof(struct RankingModel) * k + 38);
      sscanf(strstr(cursor, "\"time\":") + 7, "%[^,}]", _user.userRanking + sizeof(struct RankingModel) * k + 42);
      _user.userRanking[sizeof(struct RankingModel) * k + 49] = '\x20';
      cursor++;
    }
    // Save user entry in database.
    ld_save(&databaseUsers, 0, _user.userId, 17, &_user, sizeof(struct UserModel) - sizeof(char*) + size);
  }
  ld_free(users);
}

// Convert rankings database and replay files.
void converterRankings(int argc, char** argv) {
  // Get list of rankings.
  lv_vector* rankings = ld_find(&database, 1, NULL, 0);
  for (int i = 0; i < rankings->count; i++) {
    char* cursor = ((ld_data*) rankings->data[i])->value;
    // Build ranking entry.
    struct RankingModel* ranking = calloc(sizeof(struct RankingModel), sizeof(char));
    char rankingId[17];
    memset(rankingId, 0, sizeof(rankingId));
    sscanf(strstr(cursor, "\"_id\":") + 7, "%[^\"]", rankingId);
    sscanf(strstr(cursor, "\"id\":") + 6, "%[^\"]", ranking->rankingUser);
    sscanf(strstr(cursor, "\"mode\":") + 7, "%[^,}]", ranking->rankingMode);
    sscanf(strstr(cursor, "\"score\":") + 8, "%[^,}]", ranking->rankingScore);
    sscanf(strstr(cursor, "\"jewel\":") + 8, "%[^,}]", ranking->rankingJewel);
    sscanf(strstr(cursor, "\"level\":") + 8, "%[^,}]", ranking->rankingLevel);
    sscanf(strstr(cursor, "\"class\":") + 8, "%[^,}]", ranking->rankingClass);
    sscanf(strstr(cursor, "\"time\":") + 7, "%[^,}]", ranking->rankingTime);
    *ranking->rankingDate = '\x20';
    // Generate unique key for ranking entry.
    char rankingKey[18];
    memset(rankingKey, 0, sizeof(rankingKey));
    sprintf(rankingKey, "%s%s", ranking->rankingUser, ranking->rankingMode);
    // Save ranking entry in database.
    ld_save(&databaseRankings, 0, rankingKey, 18, ranking, sizeof(*ranking));
    // Move replay file.
    char input[64];
    char output[64];
    sprintf(input, "./server.bak/rep/%s.rep", rankingId);
    if (argc > 1 && stricmp(argv[1], "-MultiScores") == 0) {
      sprintf(output, "./server/rep/%s%s_%s.rep", ranking->rankingUser, ranking->rankingMode, ranking->rankingScore);
    } else {
      sprintf(output, "./server/rep/%s%s.rep", ranking->rankingUser, ranking->rankingMode);
    }
    MoveFile(input, output);
    free(ranking);
  }
  ld_free(rankings);
}