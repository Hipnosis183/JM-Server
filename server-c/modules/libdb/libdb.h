// libdb: Database management library.
// Version 1.1 by Renzo Pigliacampo.
// LMDB 0.9.33

#ifndef LIBDB_H
#define LIBDB_H

#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include "libvector/libvector.h"
#include "lmdb.h"

// Define data structures.
struct ld_db {
  MDB_env* env;
  MDB_txn* txn;
  MDB_dbi* dbi;
  int count;
};

typedef struct {
  int size_key;
  int size_value;
  char* key;
  char* value;
} ld_data;

// Initialize database connections.
void ld_init(struct ld_db* database, char* path, int count, char** names, unsigned int flags);

// Close database connections.
void ld_close(struct ld_db* database);

// Search all elements in database.
lv_vector* ld_find(struct ld_db* database, int index, void* key, size_t key_size);

// Search one element in database.
ld_data* ld_find_one(struct ld_db* database, int index, void* key, size_t key_size);

// Store/update element in database.
void ld_save(struct ld_db* database, int index, void* key, size_t key_size, void* value, size_t value_size);

// Store multiple elements in database.
void ld_save_bulk(struct ld_db* database, int index, lv_vector* keys, size_t key_size, lv_vector* values, size_t value_size);

// Update multiple elements in database.
void ld_update_bulk(struct ld_db* database, int index, lv_vector* elements);

// Delete element from database.
void ld_remove(struct ld_db* database, int index, void* key, size_t key_size);

// Clear list of elements from a database search.
void ld_free(lv_vector* elements);

#endif