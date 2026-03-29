// libini: INI file parser library.
// Version 1.0 by Renzo Pigliacampo.
// Based on rxi/ini

#ifndef LIBINI_H
#define LIBINI_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libvector/libvector.h"

// Define data structures.
typedef struct {
  char* data;
  char* end;
} li_handler;

// Initialize file load.
li_handler* li_init(char* filename);

// Get all elements matching the given key.
lv_vector* li_find(li_handler* handler, char* section, char* key);

// Get one element matching the given key.
char* li_find_one(li_handler* handler, char* section, char* key);

// Close file and clear memory.
void li_close(li_handler* handler);

#endif