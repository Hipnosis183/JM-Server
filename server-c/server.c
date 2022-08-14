// Jewelry Master Server Emulator by Renzo Pigliacampo (Hipnosis), 2022.
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include "ini/ini.h"
#include "lmdb/lmdb.h"
#include "mjson/mjson.h"
#include "mongoose/mongoose.h"

// 0. Local Mode: Runs the server on 127.0.0.1 and starts the game client. Doesn't work for LAN connections.
// 1. Online Mode: Disables the local server emulation and connects to the 'HostName' address.
// 2. Host Mode A: Runs the server on 'HostName' without client execution. Opens a console with server information.
// 3. Host Mode B: Runs the server on 'HostName' with client execution. Ideal for hosting LAN servers.
static int SERVERMODE = 0;
// Set server host for connection.
static char HOSTNAME[16] = "127.0.0.1";
// Enable network traffic hooking. If disabled, the hosts file should be edited manually.
static int HOOKDLL = 1;
// Allow unregistered users to be registered at the login screen.
static int REGISTER = 1;
// Allow users to have mutiple scores (and replays) in the global rankings.
// Don't change once the database has already been created, will break scores.
// Won't work if a score doesn't make it to the player's top 10 ranking.
static int MULTISCORES = 1;
// Disable scores and replays saving.
static int NOSCORES = 0;
// Set game process state.
static int RUN = 1;

// Declare game process variables.
static DWORD code;
static STARTUPINFO si;
static PROCESS_INFORMATION pi;

// Database global variables.
static MDB_env *env;
static MDB_txn *txn;
static MDB_dbi dbi_user;
static MDB_dbi dbi_ranking;

void db_init()
{
  // Initialize environment.
  mdb_env_create(&env);
  mdb_env_set_maxdbs(env, 5);
  mdb_env_open(env, "./server/db", 0, 0664);

  // Initialize databases.
  mdb_txn_begin(env, NULL, 0, &txn);
  mdb_dbi_open(txn, "user", MDB_CREATE, &dbi_user);
  mdb_dbi_open(txn, "ranking", MULTISCORES ? (MDB_CREATE | MDB_DUPSORT) : MDB_CREATE, &dbi_ranking);
  mdb_txn_commit(txn);
}

void db_close()
{
  // Close database connections and environment.
  mdb_dbi_close(env, dbi_user);
  mdb_dbi_close(env, dbi_ranking);
  mdb_env_close(env);
}

// Get the elements from the database matching the given key, or all of them if none is specified.
// Returns a pointer to the array of char arrays. Remember to free the result value and its contents.
void db_get(MDB_dbi dbi, char *_key, char ***res, int *len)
{
  // Initialize entry values.
  MDB_cursor *cur;
  MDB_val key, val;
  val.mv_size = 0;
  val.mv_data = NULL;
  if (strlen(_key) > 0) {
    key.mv_size = strlen(_key);
    key.mv_data = _key;
  } else {
    key.mv_size = 0;
    key.mv_data = NULL;
  }

  // Get items from the selected database matching the query key.
  // Stores the results values and length back to the param adresses.
  int i = 0;
  mdb_txn_begin(env, NULL, 0, &txn);
  mdb_cursor_open(txn, dbi, &cur);
  while ((mdb_cursor_get(cur, &key, &val, MDB_NEXT)) == 0) {
    // Allocate space for the new value.
    *res = realloc(*res, (i + 2) * sizeof(char *));
    (*res)[i] = calloc(((int)val.mv_size + 4), sizeof(char));
    strcpy((*res)[i], (char *)val.mv_data);
    // Secure that value is a valid string.
    (*res)[i][(int)val.mv_size] = '\0';
    i++;
  } *len = i;
  mdb_cursor_close(cur);
  mdb_txn_abort(txn);
}

// Get a single (first) element from the database matching the given key.
// Returns a pointer to the char array. Remember to free the return value.
char *db_get_one(MDB_dbi dbi, char *_key)
{
  // Initialize entry values.
  MDB_val key, val;
  key.mv_size = strlen(_key);
  key.mv_data = _key;
  val.mv_size = 0;
  val.mv_data = NULL;

  // Get an item from the selected database.
  mdb_txn_begin(env, NULL, 0, &txn);
  mdb_get(txn, dbi, &key, &val);
  mdb_txn_abort(txn);

  // Make sure the value is a valid string.
  char *buf = malloc((int)val.mv_size + 2);
  memcpy(buf, val.mv_data, (int)val.mv_size);
  buf[(int)val.mv_size] = '\0';
  return buf;
}

// Store a new key/value entry into the database, or update an already existing one.
void db_put(MDB_dbi dbi, char *_key, char *_val)
{
  // Initialize entry values.
  MDB_val key, val;
  key.mv_size = strlen(_key);
  key.mv_data = _key;
  val.mv_size = strlen(_val);
  val.mv_data = _val;

  // Store/update entry in database.
  mdb_txn_begin(env, NULL, 0, &txn);
  mdb_put(txn, dbi, &key, &val, 0);
  mdb_txn_commit(txn);
}

// Sort given elements in ascending order.
int cmp_asc(const void *a, const void *b)
{
  const char *_a = *(const char **)a;
  const char *_b = *(const char **)b;

  double buf_a_d, buf_b_d;
  mjson_get_number(_a, strlen(_a), "$.score", &buf_a_d);
  mjson_get_number(_b, strlen(_b), "$.score", &buf_b_d);

  int buf_a_i = (int)buf_a_d, buf_b_i = (int)buf_b_d;
  return (buf_b_i - buf_a_i);
}

// Sort given elements in descending order.
int cmp_des(const void *a, const void *b)
{
  const char *_a = *(const char **)a;
  const char *_b = *(const char **)b;

  double buf_a_d, buf_b_d;
  mjson_get_number(_a, strlen(_a), "$.score", &buf_a_d);
  mjson_get_number(_b, strlen(_b), "$.score", &buf_b_d);

  int buf_a_i = (int)buf_a_d, buf_b_i = (int)buf_b_d;
  return (buf_a_i - buf_b_i);
}

// Get fixed length random number.
char *random_num(char *buf)
{
  for (int i = 0; i < 16; i++) {
    buf[i] = (rand() % 10) + '0';
  } buf[16] = '\0'; return buf;
}

// Authenticate user. Used for login, getting rankings and starting games.
// Response: '1': Auth error | '10': Connection error | ?: Version error.
// Params: 'game', 'id', 'pass', 'ver'.
void game_entry(struct mg_connection *c, struct mg_http_message *hm)
{
  // Get query param values.
  char q_id[18], q_pass[18];
  mg_http_get_var(&hm->query, "id", q_id, sizeof(q_id));
  mg_http_get_var(&hm->query, "pass", q_pass, sizeof(q_pass));

  // Get selected user from database.
  char *user = db_get_one(dbi_user, q_id);
  // Check if user exists and the credentials are correct.
  if (strlen(user) > 0) {
    // Get password value from user object.
    char u_pass[18];
    mjson_get_string(user, strlen(user), "$.pass", u_pass, sizeof(u_pass));
    mg_http_reply(c, 200, NULL, strcmp(q_pass, u_pass) == 0 ? "" : "1");
  // Check for users with the same id and create a new one if allowed.
  } else if (strlen(q_id) > 0 && REGISTER) {
    // Store new user into the database.
    char u_str[100];
    snprintf(u_str, 100, "{\"id\":\"%s\",\"pass\":\"%s\",\"count\":0,\"rankings\":[]}", q_id, q_pass);
    db_put(dbi_user, q_id, u_str);
    mg_http_reply(c, 200, NULL, "");
  // An user with this id already exists or wrong user id or password.
  } else { mg_http_reply(c, 200, NULL, "1"); }
  // Free memory allocated for user data.
  free(user);
}

// Get rankings/leaderboards data.
// Params: 'id', 'mode', 'view'.
void get_ranking(struct mg_connection *c, struct mg_http_message *hm)
{
  // Get query param values.
  char q_id[18] = "", q_mode[2], q_view[4];
  mg_http_get_var(&hm->query, "id", q_id, sizeof(q_id));
  mg_http_get_var(&hm->query, "mode", q_mode, sizeof(q_mode));
  mg_http_get_var(&hm->query, "view", q_view, sizeof(q_view));

  // Convert mode query parameter to double.
  char *q_mode_p; double q_mode_d = strtod(q_mode, &q_mode_p);
  char *q_view_p; double q_view_d = strtod(q_view, &q_view_p);

  // Manage personal rankings.
  if (strlen(q_id) > 0 && q_view_d == 0) {
    char *user = db_get_one(dbi_user, q_id);
    // Get and parse user rankings object.
    double u_count;
    mjson_get_number(user, strlen(user), "$.count", &u_count);
    // Store the ranking objects for the selected mode.
    char *u_ranks = calloc(1, sizeof(char));
    for (int i = 0; i < u_count; i++) {
      double r_mode; char r_mode_s[24];
      snprintf(r_mode_s, 24, "%s%d%s", "$.rankings[", i, "].mode");
      mjson_get_number(user, strlen(user), r_mode_s, &r_mode);
      if (r_mode == q_mode_d) {
        // Get user ranking object values.
        double r_score; char r_score_s[24];
        snprintf(r_score_s, 24, "%s%d%s", "$.rankings[", i, "].score");
        mjson_get_number(user, strlen(user), r_score_s, &r_score);
        double r_level; char r_level_s[24];
        snprintf(r_level_s, 24, "%s%d%s", "$.rankings[", i, "].level");
        mjson_get_number(user, strlen(user), r_level_s, &r_level);
        double r_time; char r_time_s[24];
        snprintf(r_time_s, 24, "%s%d%s", "$.rankings[", i, "].time");
        mjson_get_number(user, strlen(user), r_time_s, &r_time);
        double r_jewel; char r_jewel_s[24];
        snprintf(r_jewel_s, 24, "%s%d%s", "$.rankings[", i, "].jewel");
        mjson_get_number(user, strlen(user), r_jewel_s, &r_jewel);

        // Build formatted response string.
        char r_str[200]; int lit = strlen(u_ranks) == 0 ? 1 : 0;
        snprintf(r_str, 200, "0\n0\n%s\n%d\n0\n%d\n0\n%d\n%d\n%d", q_id, (int)r_score, (int)r_level, (int)r_time, (int)r_jewel, lit);
        u_ranks = realloc(u_ranks, strlen(u_ranks) + strlen(r_str) + 2);
        if (strlen(u_ranks) > 0) { strcat(u_ranks, "."); }
        strcat(u_ranks, r_str);
      }
    } mg_http_reply(c, 200, NULL, "%s", u_ranks);
    // Free memory allocated for user rankings and data.
    free(user); free(u_ranks);

  // Manage global rankings.
  } else {
    char **rank = calloc(1, sizeof(char *)); int r_len;
    db_get(dbi_ranking, "", &rank, &r_len);

    if (r_len > 0) {
      // Get the ranking objects for the selected mode.
      char **rank_mode = calloc(1, sizeof(char *)); int r_mode_len = 0;
      for (int i = 0; i < r_len; i++) {
        double r_mode;
        mjson_get_number(rank[i], strlen(rank[i]), "$.mode", &r_mode);
        if (r_mode == q_mode_d) {
          // Allocate space for the new user ranking insert.
          if (r_mode_len > 0) { rank_mode = realloc(rank_mode, (r_mode_len + 2) * sizeof(char *)); }
          rank_mode[r_mode_len] = calloc(strlen(rank[i]) + 2, sizeof(char));
          strncpy(rank_mode[r_mode_len], rank[i], strlen(rank[i]));
          r_mode_len++;
        }
      }
      // Sort results for the selected mode.
      qsort(rank_mode, (int)r_mode_len, sizeof(char *), cmp_asc);
      // Set rankings table index.
      int idx = strcmp(q_view, "-1") == 0 ? 0 : (int)q_view_d;
      if (strlen(q_id) > 0) {
        // Get user score position table index.
        int f = -1;
        for (int i = 0; i < r_mode_len; i++) {
          char r_id[20];
          mjson_get_string(rank_mode[i], strlen(rank_mode[i]), "$.id", r_id, sizeof(r_id));
          if (strcmp(r_id, q_id) == 0) { f = i; break; }
        } if (f != -1) { idx = floor(f / 10); }
      }

      // Fill the 10-slots scores table.
      char *g_ranks = calloc(1, sizeof(char));  int lit_f = 0;
      for (int i = (idx * 10); i < (idx * 10 + 10); i++) {
        if (i >= (r_mode_len)) { break; }
        // Get global rankings object values.
        char r__id[30]; char r_id[20]; double r_score, r_level, r_class, r_time, r_jewel;
        mjson_get_string(rank_mode[i], strlen(rank_mode[i]), "$._id", r__id, sizeof(r__id));
        mjson_get_string(rank_mode[i], strlen(rank_mode[i]), "$.id", r_id, sizeof(r_id));
        mjson_get_number(rank_mode[i], strlen(rank_mode[i]), "$.score", &r_score);
        mjson_get_number(rank_mode[i], strlen(rank_mode[i]), "$.level", &r_level);
        mjson_get_number(rank_mode[i], strlen(rank_mode[i]), "$.class", &r_class);
        mjson_get_number(rank_mode[i], strlen(rank_mode[i]), "$.time", &r_time);
        mjson_get_number(rank_mode[i], strlen(rank_mode[i]), "$.jewel", &r_jewel);

        // Build formatted response string.
        int lit = strcmp(r_id, q_id) == 0 && !lit_f ? 1 : 0;
        if (strcmp(r_id, q_id) == 0) { lit_f = 1;} char r_str[200];
        snprintf(r_str, 200, "%d\n%s\n%s\n%d\n0\n%d\n%d\n%d\n%d\n%d", idx, r__id, r_id, (int)r_score, (int)r_level, (int)r_class, (int)r_time, (int)r_jewel, lit);
        g_ranks = realloc(g_ranks, strlen(g_ranks) + strlen(r_str) + 2);
        if (strlen(g_ranks) > 0) { strcat(g_ranks, "."); }
        strcat(g_ranks, r_str);
      } mg_http_reply(c, 200, NULL, "%s", g_ranks);

      // Free memory allocated for mode rankings.
      for (int i = 0; i < r_mode_len; i++) {
        free(rank_mode[i]);
      } free(rank_mode); free(g_ranks);
    } else { mg_http_reply(c, 200, NULL, ""); }

    // Free memory allocated for global rankings.
    for (int i = 0; i < r_len; i++) {
      free(rank[i]);
    } free(rank);
  }
}

// Get replay for the selected score.
// Params: 'id', 'mode', 'view'.
void get_replay(struct mg_connection *c, struct mg_http_message *hm)
{
  // Get query param values.
  char q_id[30], q_mode[2], q_view[4];
  mg_http_get_var(&hm->query, "id", q_id, sizeof(q_id));

  // Load and send replay file.
  char r_dir[MAX_PATH], r_file[MAX_PATH];
  GetCurrentDirectory(MAX_PATH, r_dir);
  snprintf(r_file, MAX_PATH, "%s\\server\\rep\\%s.rep", r_dir, q_id);
  struct mg_http_serve_opts opts = { };
  mg_http_serve_file(c, hm, r_file, &opts);
}

// Send user score to rankings/leaderboards and replay data.
// Params: 'id', 'mode', 'score', 'jewel', 'level', 'class', 'time'.
void score_entry(struct mg_connection *c, struct mg_http_message *hm)
{
  // Get query param values.
  char q_id[18], q_mode[2], q_score[12], q_jewel[6], q_level[4], q_class[4], q_time[18], q_key[20];
  mg_http_get_var(&hm->query, "id", q_id, sizeof(q_id));
  mg_http_get_var(&hm->query, "mode", q_mode, sizeof(q_mode));
  mg_http_get_var(&hm->query, "score", q_score, sizeof(q_score));
  mg_http_get_var(&hm->query, "jewel", q_jewel, sizeof(q_jewel));
  mg_http_get_var(&hm->query, "level", q_level, sizeof(q_level));
  mg_http_get_var(&hm->query, "class", q_class, sizeof(q_class));
  mg_http_get_var(&hm->query, "time", q_time, sizeof(q_time));

  // Convert mode query parameter to double.
  char *q_mode_p; double q_mode_d = strtod(q_mode, &q_mode_p);
  char *q_score_p; double q_score_d = strtod(q_score, &q_score_p);
  // Generate unique identifiable key for rankings.
  snprintf(q_key, 20, "%s%s", q_id, q_mode);

  // Manage global rankings database and replays storage.
  char *rank = db_get_one(dbi_ranking, q_key);
  // Update user score entry if already present.
  if (strlen(rank) > 0 && !MULTISCORES) {
    // Replace only if the score is higher than the already stored.
    double r_score;
    mjson_get_number(rank, strlen(rank), "$.score", &r_score);
    if (q_score_d > r_score) {
      // Update ranking entry in database.
      char r_id[25], r_str[200], r_dir[MAX_PATH], r_file[MAX_PATH];
      mjson_get_string(rank, strlen(rank), "$._id", r_id, sizeof(r_id));
      snprintf(r_str, 200, "{\"_id\":\"%s\",\"id\":\"%s\",\"mode\":%s,\"score\":%s,\"jewel\":%s,\"level\":%s,\"class\":%s,\"time\":%s}", r_id, q_id, q_mode, q_score, q_jewel, q_level, q_class, q_time);
      db_put(dbi_ranking, q_key, r_str);

      // Delete previous replay file and replace it with the new one.
      GetCurrentDirectory(MAX_PATH, r_dir);
      snprintf(r_file, MAX_PATH, "%s\\server\\rep\\%s.rep", r_dir, r_id);
      struct mg_http_part part; size_t ofs = 0;
      mg_http_next_multipart(hm->body, ofs, &part);
      remove(r_file);
      FILE *fp = fopen(r_file, "w");
      fwrite(part.body.ptr, (unsigned long)part.body.len, sizeof(char), fp);
      fclose(fp);
    }

  // Add score entry if it's from a new user or multiple scores are enabled.
  } else {
    // Store new score entry in the rankings database.
    char r_id[18]; random_num(r_id);
    char r_str[200], r_dir[MAX_PATH], r_file[MAX_PATH];
    snprintf(r_str, 200, "{\"_id\":\"%s\",\"id\":\"%s\",\"mode\":%s,\"score\":%s,\"jewel\":%s,\"level\":%s,\"class\":%s,\"time\":%s}", r_id, q_id, q_mode, q_score, q_jewel, q_level, q_class, q_time);
    db_put(dbi_ranking, q_key, r_str);

    // Store replay file with the newly created id as the filename.
    GetCurrentDirectory(MAX_PATH, r_dir);
    snprintf(r_file, MAX_PATH, "%s\\server\\rep\\%s.rep", r_dir, r_id);
    struct mg_http_part part; size_t ofs = 0;
    mg_http_next_multipart(hm->body, ofs, &part);
    FILE *fp = fopen(r_file, "w");
    fwrite(part.body.ptr, (unsigned long)part.body.len, sizeof(char), fp);
    fclose(fp);
  }
  // Free memory allocated for global rankings.
  free(rank);

  // Manage personal rankings from the users database.
  char *user = db_get_one(dbi_user, q_id);
  // Get and parse user rankings object.
  double u_count;
  mjson_get_number(user, strlen(user), "$.count", &u_count);
  // Store the ranking objects for the selected mode and the total length.
  int u_ranks_mode = 0;
  char **u_ranks = calloc(1, sizeof(char *));
  for (int i = 0; i < u_count; i++) {
    // Calculate amount of items for the selected mode.
    double u_rank_mode; char u_rank_mode_s[24];
    snprintf(u_rank_mode_s, 24, "%s%d%s", "$.rankings[", i, "].mode");
    mjson_get_number(user, strlen(user), u_rank_mode_s, &u_rank_mode);
    if (u_rank_mode == q_mode_d) { u_ranks_mode++; }

    // Add ranking object to user rankings array for future sorting and updating.
    const char *u_rank; int u_rank_len; char u_rank_s[24];
    snprintf(u_rank_s, 24, "%s%d%s", "$.rankings[", i, "]");
    mjson_find(user, strlen(user), u_rank_s, &u_rank, &u_rank_len);
    // Allocate space for the new user ranking insert.
    if (i > 0) { u_ranks = realloc(u_ranks, (i + 2) * sizeof(char *)); }
    u_ranks[i] = calloc(strlen(u_rank) + 2, sizeof(char));
    strncpy(u_ranks[i], u_rank, u_rank_len);
  }

  // Check if the user rankings slots are full for the selected mode.
  if (u_ranks_mode == 10) {
    // Sort rankings to get the smallest score for the selected mode.
    qsort(u_ranks, u_count, sizeof(char *), cmp_des);
    int u_ranks_sm = 0;
    for (int i = 0; i < u_count; i++) {
      double u_rank_mode;
      mjson_get_number(u_ranks[i], strlen(u_ranks[i]), "$.mode", &u_rank_mode);
      if (u_rank_mode == q_mode_d) {
        u_ranks_sm = i; break;
      }
    }
    // Replace only if the score is higher than the smallest stored.
    double u_rank_score;
    mjson_get_number(u_ranks[u_ranks_sm], strlen(u_ranks[u_ranks_sm]), "$.score", &u_rank_score);
    if (q_score_d > u_rank_score) {
      char r_str[200];
      snprintf(r_str, 200, "{\"id\":\"%s\",\"mode\":%s,\"score\":%s,\"jewel\":%s,\"level\":%s,\"class\":%s,\"time\":%s}", q_id, q_mode, q_score, q_jewel, q_level, q_class, q_time);
      u_ranks[u_ranks_sm] = realloc(u_ranks[u_ranks_sm], strlen(r_str) + 2);
      strncpy(u_ranks[u_ranks_sm], r_str, strlen(r_str));
    }

  // Add new score entry to the user personal ranking.
  } else {
    u_count++; char r_str[200];
    snprintf(r_str, 200, "{\"id\":\"%s\",\"mode\":%s,\"score\":%s,\"jewel\":%s,\"level\":%s,\"class\":%s,\"time\":%s}", q_id, q_mode, q_score, q_jewel, q_level, q_class, q_time);
    // Allocate space for the new ranking insert.
    u_ranks = realloc(u_ranks, u_count * sizeof(char *));
    u_ranks[(int)u_count - 1] = calloc(strlen(r_str) + 2, sizeof(char));
    strcpy(u_ranks[(int)u_count - 1], r_str);
  }

  // Sort the user rankings for storage, to avoid having to sort on each ranking request (and duplicate all the code from above).
  qsort(u_ranks, (int)u_count, sizeof(char *), cmp_asc);
  // Build user rankings object array string.
  char *u_ranks_str = calloc(1, sizeof(char));
  for (int i = 0; i < u_count; i++) {
    u_ranks_str = realloc(u_ranks_str, strlen(u_ranks_str) + strlen(u_ranks[i]) + 2);
    if (i != 0) { strcat(u_ranks_str, ","); }
    strcat(u_ranks_str, u_ranks[i]);
  }
  // Build final user object string and update the entry on the database.
  char u_pass[20];
  mjson_get_string(user, strlen(user), "$.pass", u_pass, sizeof(u_pass));
  char u_str[strlen(u_ranks_str) + 100];
  snprintf(u_str, strlen(u_ranks_str) + 100, "{\"id\":\"%s\",\"pass\":\"%s\",\"count\":%d,\"rankings\":[%s]}", q_id, u_pass, (int)u_count, u_ranks_str);
  db_put(dbi_user, q_id, u_str);
  free(u_ranks_str);

  // Free memory allocated for user rankings and data.
  for (int i = 0; i < u_count; i++) {
    free(u_ranks[i]);
  } free(u_ranks); free(user);
  mg_http_reply(c, 200, NULL, "");
}

// Main server polling function, runs forever.
static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
{
  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *)ev_data;
    if (mg_http_match_uri(hm, "/JM_test/service/GameEntry")) {
      printf("-GameEntry:\n%s", hm->query.ptr);
      game_entry(c, hm);
    // Get main menu message.
    } else if (mg_http_match_uri(hm, "/JM_test/service/GetMessage")) {
      printf("-GetMessage: %s\n\n", hm->query.ptr);
      mg_http_reply(c, 200, NULL, "Jewelry Master Server Emulator by Hipnosis, 2022\n");
    // Unkwnown usage. Probably unused?. Params: 'id'.
    } else if (mg_http_match_uri(hm, "/JM_test/service/GetName")) {
      printf("-GetName:\n%s", hm->query.ptr);
      mg_http_reply(c, 200, NULL, "");
    } else if (mg_http_match_uri(hm, "/JM_test/service/GetRanking")) {
      printf("-GetRanking:\n%s", hm->query.ptr);
      get_ranking(c, hm);
    } else if (mg_http_match_uri(hm, "/JM_test/service/GetReplay")) {
      printf("-GetReplay:\n%s", hm->query.ptr);
      get_replay(c, hm);
    } else if (mg_http_match_uri(hm, "/JM_test/service/ScoreEntry")) {
      printf("-ScoreEntry:\n%s", hm->query.ptr);
      if (!NOSCORES) { score_entry(c, hm); }
      else { mg_http_reply(c, 404, NULL, ""); }
    } else { mg_http_reply(c, 404, NULL, ""); }
  }
  // Check if the game has been closed.
  if (SERVERMODE != 2 && GetExitCodeProcess(pi.hProcess, &code)) {
    if (code != STATUS_PENDING) {
      CloseHandle(pi.hProcess);
      CloseHandle(pi.hThread);
      c->is_closing = 1; RUN = 0;
    }
  }
}

// DLL injection function.
void DllInject(const HANDLE process, const char *dll_path)
{
  // Allocate space for library in memory.
  const SIZE_T buf_len = strlen(dll_path) + 1;
  LPVOID *buf = VirtualAllocEx(process, NULL, buf_len, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  // Write library buffer to process memory.
  WriteProcessMemory(process, buf, dll_path, buf_len, NULL);
  CreateRemoteThread(process, NULL, 0, (LPTHREAD_START_ROUTINE)(LoadLibrary), buf, 0, NULL);
}

int main(int argc, char *argv[])
{
  // Create directories for database and replays storage.
  char dir[MAX_PATH], srv[MAX_PATH], db[MAX_PATH], rep[MAX_PATH];
  GetCurrentDirectory(MAX_PATH, dir);
  if (SERVERMODE != 1) {
    snprintf(srv, MAX_PATH, "%s\\server\\", dir);
    snprintf(db, MAX_PATH, "%s\\server\\db\\", dir);
    snprintf(rep, MAX_PATH, "%s\\server\\rep\\", dir);
    CreateDirectory(srv, NULL);
    CreateDirectory(db, NULL);
    CreateDirectory(rep, NULL);
  }

  // Load configuration options from file.
  char ini[MAX_PATH]; char *svr_p, *hdl_p, *ncl_p, *reg_p, *mul_p, *nsc_p;
  snprintf(ini, MAX_PATH, "%s\\server.ini", dir);
  ini_t *config = ini_load(ini);
  if (config) {
    const char *svr = ini_get(config, "Connection", "ServerMode");
    const char *hst = ini_get(config, "Connection", "HostName");
    const char *hdl = ini_get(config, "Connection", "HookDLL");
    const char *reg = ini_get(config, "Options", "Register");
    const char *mul = ini_get(config, "Options", "MultiScores");
    const char *nsc = ini_get(config, "Options", "NoScores");
    if (svr) { SERVERMODE = strtol(svr, &svr_p, 10); }
    if (hst && SERVERMODE != 0) { snprintf(HOSTNAME, 16, hst); }
    if (hdl) { HOOKDLL = strtol(hdl, &hdl_p, 10); }
    if (reg) { REGISTER = strtol(reg, &reg_p, 10); }
    if (mul) { MULTISCORES = strtol(mul, &mul_p, 10); }
    if (nsc) { NOSCORES = strtol(nsc, &nsc_p, 10); } ini_free(config);
  }

  // Close console window on start.
  if (SERVERMODE != 2) {
    HWND hWnd = GetConsoleWindow();
    ShowWindow(hWnd, SW_HIDE);
  }

  if (SERVERMODE != 1) {
    // Initialize database.
    db_init();
    // Intialize random number generator for replays ids.
    // Required for random_num() to have a unique seed.
    srand(time(NULL));
  }

  // Create game executable process.
  if (SERVERMODE != 2) {
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si); char exe[MAX_PATH];
    snprintf(exe, MAX_PATH, "%s\\JM.exe", dir);
    CreateProcess(exe, argv[1], NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi);
    // Inject library for networking functions hooking.
    if (HOOKDLL) { DllInject(pi.hProcess, "server.dll"); }
    ResumeThread(pi.hThread);
  }

  // Manage and start web server.
  if (SERVERMODE != 1) {
    char url[40], mes[80];
    snprintf(url, 40, "http://%s:8081", HOSTNAME);
    snprintf(mes, 80, "Server for Jewelry Master created on %s\n\n", url);
    struct mg_mgr mgr;
    mg_mgr_init(&mgr); printf(mes);
    mg_http_listen(&mgr, url, fn, &mgr);
    while (RUN) { mg_mgr_poll(&mgr, 1000); }
    // Close server and database and exit the program.
    mg_mgr_free(&mgr); db_close();
  } return 0;
}