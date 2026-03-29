// libvector: Dynamic vectors library.
// Version 1.0 by Renzo Pigliacampo.

#ifndef LIBVECTOR_H
#define LIBVECTOR_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define data structures.
typedef struct {
  int count;
  int size;
  void** data;
} lv_vector;

// Initialize vector.
void lv_init(lv_vector* vector, int size);

// Add element at the given index of the vector.
void lv_insert(lv_vector* vector, const void* element, int index);

// Add element at the end of the vector.
void lv_push(lv_vector* vector, const void* element);

// Remove element at the given index of the vector.
void lv_erase(lv_vector* vector, int index);

// Remove last element from the vector.
void lv_pop(lv_vector* vector);

// Clear allocated vector memory.
void lv_free(lv_vector* vector);

#endif