// libini: INI file parser library.
// Version 1.0 by Renzo Pigliacampo.
// Based on rxi/ini

#include "libini.h"

// Return next string from previously formatted (splitted) data.
static char* li_next(li_handler* handler, char* pointer) {
  pointer += strlen(pointer);
  while (pointer < handler->end && *pointer == '\0') {
    pointer++;
  }
  return pointer;
}

// Remove last character from value.
static void li_trim(li_handler* handler, char* pointer) {
  while (pointer >= handler->data && (*pointer == ' ' || *pointer == '\t' || *pointer == '\r')) {
    *pointer-- = '\0';
  }
}

// Discard invalid lines.
static char* li_discard(li_handler* handler, char* pointer) {
  while (pointer < handler->end && *pointer != '\n') {
    *pointer++ = '\0';
  }
  return pointer;
}

// Splits data in place into strings containing sections, keys
// and values using one or more null terminators as the delimiter.
static void li_split(li_handler* handler) {
  char* start;
  // Loop through the stored file data.
  char* pointer = handler->data;
  while (pointer < handler->end) {
    switch (*pointer) {
      // Clear spaces and line jumps.
      case '\r':
      case '\n':
      case '\t':
      case ' ': {
        *pointer = '\0';
      }
      // Skip null bytes.
      case '\0': {
        pointer++;
        break;
      }
      // Clear end of section.
      case '[': {
        pointer += strcspn(pointer, "]\n");
        *pointer = '\0';
        break;
      }
      // Skip comments.
      case ';': {
        pointer = li_discard(handler, pointer);
        break;
      }
      // Process rest of characters.
      default: {
        start = pointer;
        pointer += strcspn(pointer, "=\n");
        // Discard line if '=' is missing.
        if (*pointer != '=') {
          pointer = li_discard(handler, start);
          break;
        }
        li_trim(handler, pointer - 1);
        // Replace '=' and whitespace with null.
        do {
          *pointer++ = '\0';
        } while (*pointer == ' ' || *pointer == '\r' || *pointer == '\t');
        // Discard line if value is empty.
        if (*pointer == '\n' || *pointer == '\0') {
          pointer = li_discard(handler, start);
          break;
        }
        // Handle normal value.
        pointer += strcspn(pointer, "\n");
        li_trim(handler, pointer - 1);
        break;
      }
    }
  }
}

// Initialize file load.
li_handler* li_init(char* filename) {
  li_handler* handler = NULL;
  FILE* file = NULL;
  int n, size;
  // Initialize file handler.
  handler = malloc(sizeof(*handler));
  if (!handler) {
    goto error;
  }
  memset(handler, 0, sizeof(*handler));
  // Open file.
  file = fopen(filename, "rb");
  if (!file) {
    goto error;
  }
  // Get file size.
  fseek(file, 0, SEEK_END);
  size = ftell(file);
  rewind(file);
  // Read file data.
  handler->data = malloc(size + 1);
  handler->data[size] = '\0';
  handler->end = handler->data + size;
  if (fread(handler->data, 1, size, file) != size) {
    goto error;
  }
  // Format loaded data.
  li_split(handler);
  // Close file handler and return.
  fclose(file);
  return handler;
  // Handle errors.
  error:
  if (file) {
    fclose(file);
  }
  if (handler) {
    li_close(handler);
  }
  return NULL;
}

// Get all elements matching the given key.
lv_vector* li_find(li_handler* handler, char* section, char* key) {
  char* _section = "";
  char* value;
  lv_vector* values = calloc(sizeof(lv_vector), sizeof(char));
  lv_init(values, sizeof(char*));
  // Loop through the stored file data.
  char* pointer = handler->data;
  while (pointer < handler->end) {
    if (*pointer == '[') {
      // Handle sections.
      _section = pointer + 1;
    } else {
      // Handle keys.
      value = li_next(handler, pointer);
      // Return value if section and key match.
      if (!section || (strcmp(section, _section) == 0)) {
        if (strcmp(pointer, key) == 0) {
          lv_push(values, &value);
        }
      }
      pointer = value;
    }
    // Skip to next string.
    pointer = li_next(handler, pointer);
  }
  // Return found values.
  if (values->count) {
    return values;
  } else {
    lv_free(values);
    return NULL;
  }
}

// Get one element matching the given key.
char* li_find_one(li_handler* handler, char* section, char* key) {
  char* _section = "";
  char* value;
  // Loop through the stored file data.
  char* pointer = handler->data;
  while (pointer < handler->end) {
    if (*pointer == '[') {
      // Handle sections.
      _section = pointer + 1;
    } else {
      // Handle keys.
      value = li_next(handler, pointer);
      // Return value if section and key match.
      if (!section || (strcmp(section, _section) == 0)) {
        if (strcmp(pointer, key) == 0) {
          return value;
        }
      }
      pointer = value;
    }
    // Skip to next string.
    pointer = li_next(handler, pointer);
  }
  return NULL;
}

// Close file and clear memory.
void li_close(li_handler* handler) {
  free(handler->data);
  free(handler);
}