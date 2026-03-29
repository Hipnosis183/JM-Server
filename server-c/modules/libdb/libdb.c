// libdb: Database management library.
// Version 1.1 by Renzo Pigliacampo.
// LMDB 0.9.33

#include "libdb.h"

// Initialize database connections.
void ld_init(struct ld_db* database, char* path, int count, char** names, unsigned int flags) {
  // Create database directory.
  CreateDirectory(path, NULL);
  // Create and open environment.
  database->count = count;
  database->dbi = calloc(count, sizeof(MDB_dbi) * count);
  mdb_env_create(&database->env);
  mdb_env_set_maxdbs(database->env, count);
  mdb_env_open(database->env, path, MDB_WRITEMAP, 0664);
  // Open all databases.
  mdb_txn_begin(database->env, NULL, 0, &database->txn);
  for (int i = 0; i < count; i++) {
    mdb_dbi_open(database->txn, names[i], flags ? flags : MDB_CREATE, &database->dbi[i]);
  }
  mdb_txn_commit(database->txn);
}

// Close database connections.
void ld_close(struct ld_db* database) {
  for (int i = 0; i < database->count; i++) {
    mdb_dbi_close(database->env, database->dbi[i]);
  }
  free(database->dbi);
  mdb_env_close(database->env);
}

// Search all elements in database.
lv_vector* ld_find(struct ld_db* database, int index, void* key, size_t key_size) {
  // Define query data.
  MDB_val mdb_key = key ? (MDB_val) { key_size, key } : (MDB_val) { 0, NULL };
  MDB_val mdb_value = { 0, NULL };
  // Execute query.
  MDB_cursor *cur;
  mdb_txn_begin(database->env, NULL, 0, &database->txn);
  mdb_cursor_open(database->txn, database->dbi[index], &cur);
  // Create vector from query results.
  lv_vector* elements = calloc(sizeof(lv_vector), sizeof(char));
  lv_init(elements, sizeof(ld_data*));
  while (mdb_cursor_get(cur, &mdb_key, &mdb_value, MDB_NEXT) == MDB_SUCCESS) {
    // Copy element data to buffer.
    ld_data* element = calloc(sizeof(ld_data), sizeof(char));
    memcpy(&element->key, &mdb_key.mv_data, sizeof(mdb_key.mv_data));
    memcpy(&element->value, &mdb_value.mv_data, sizeof(mdb_value.mv_data));
    element->size_key = mdb_key.mv_size;
    element->size_value = mdb_value.mv_size;
    lv_push(elements, &element);
  }
  mdb_cursor_close(cur);
  mdb_txn_abort(database->txn);
  return elements;
}

// Search one element in database.
ld_data* ld_find_one(struct ld_db* database, int index, void* key, size_t key_size) {
  // Define query data.
  MDB_val mdb_key = { key_size, key };
  MDB_val mdb_value = { 0, NULL };
  // Execute query.
  mdb_txn_begin(database->env, NULL, 0, &database->txn);
  mdb_get(database->txn, database->dbi[index], &mdb_key, &mdb_value);
  mdb_txn_abort(database->txn);
  // Copy element data to buffer.
  ld_data* element = calloc(sizeof(ld_data), sizeof(char));
  memcpy(&element->key, &mdb_key.mv_data, sizeof(mdb_key.mv_data));
  memcpy(&element->value, &mdb_value.mv_data, sizeof(mdb_value.mv_data));
  element->size_key = mdb_key.mv_size;
  element->size_value = mdb_value.mv_size;
  return element;
}

// Store/update element in database.
void ld_save(struct ld_db* database, int index, void* key, size_t key_size, void* value, size_t value_size) {
  // Define query data.
  MDB_val mdb_key = { key_size, key };
  MDB_val mdb_value = { value_size, value };
  // Execute query.
  mdb_txn_begin(database->env, NULL, 0, &database->txn);
  mdb_put(database->txn, database->dbi[index], &mdb_key, &mdb_value, 0);
  mdb_txn_commit(database->txn);
}

// Store multiple elements in database.
void ld_save_bulk(struct ld_db* database, int index, lv_vector* keys, size_t key_size, lv_vector* values, size_t value_size) {
  // Begin query.
  mdb_txn_begin(database->env, NULL, 0, &database->txn);
  for (int i = 0; i < keys->count; i++) {
    // Define query data.
    MDB_val mdb_key = { key_size, keys->data[i] };
    MDB_val mdb_value = { value_size, values->data[i] };
    // Execute query.
    mdb_put(database->txn, database->dbi[index], &mdb_key, &mdb_value, 0);
  }
  mdb_txn_commit(database->txn);
}

// Update multiple elements in database.
void ld_update_bulk(struct ld_db* database, int index, lv_vector* elements) {
  // Begin query.
  mdb_txn_begin(database->env, NULL, 0, &database->txn);
  for (int i = 0; i < elements->count; i++) {
    // Define query data.
    ld_data* element = (ld_data*) elements->data[i];
    MDB_val mdb_key = { element->size_key, element->key };
    MDB_val mdb_value = { element->size_value, element->value };
    // Execute query.
    mdb_put(database->txn, database->dbi[index], &mdb_key, &mdb_value, 0);
  }
  mdb_txn_commit(database->txn);
}

// Delete element from database.
void ld_remove(struct ld_db* database, int index, void* key, size_t key_size) {
  // Define query data.
  MDB_val mdb_key = { key_size, key };
  // Execute query.
  mdb_txn_begin(database->env, NULL, 0, &database->txn);
  mdb_del(database->txn, database->dbi[index], &mdb_key, NULL);
  mdb_txn_commit(database->txn);
}

// Clear list of elements from a database search.
void ld_free(lv_vector* elements) {
  for (int i = 0; i < elements->count; i++) {
    free(elements->data[i]);
  }
  lv_free(elements);
}