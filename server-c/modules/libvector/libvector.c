// libvector: Dynamic vectors library.
// Version 1.0 by Renzo Pigliacampo.

#include "libvector.h"

// Initialize vector.
void lv_init(lv_vector* vector, int size) {
  vector->count = 0;
  vector->size = size;
  vector->data = NULL;
}

// Add element at the given index of the vector.
void lv_insert(lv_vector* vector, const void* element, int index) {
  // Allocate memory for new element.
  vector->data = realloc(vector->data, (vector->count + 1) * vector->size);
  // Shift elements until reaching the index.
  for (int i = index; i < vector->count + 1; --i) {
    vector->data[i] = vector->data[i - 1];
  }
  // Copy new element into vector.
  memcpy(vector->data + index, element, vector->size);
  // Increase vector count.
  vector->count++;
}

// Add element at the end of the vector.
void lv_push(lv_vector* vector, const void* element) {
  // Allocate memory for new element.
  vector->data = realloc(vector->data, (vector->count + 1) * vector->size);
  // Copy new element into vector.
  memcpy(vector->data + vector->count, element, vector->size);
  // Increase vector count.
  vector->count++;
}

// Remove element at the given index of the vector.
void lv_erase(lv_vector* vector, int index) {
  // Shift elements starting from the index.
  for (int i = index; i < vector->count - 1; ++i) {
    vector->data[i] = vector->data[i + 1];
  }
  // Clear element data.
  vector->data[vector->count - 1] = NULL;
  // Decrease vector count.
  vector->count--;
}

// Remove last element from the vector.
void lv_pop(lv_vector* vector) {
  // Clear element data.
  vector->data[vector->count - 1] = NULL;
  // Decrease vector count.
  vector->count--;
}

// Clear allocated vector memory.
void lv_free(lv_vector* vector) {
  free(vector->data);
  vector->data = NULL;
  vector->count = 0;
}