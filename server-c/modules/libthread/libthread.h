// libthread: Worker thread management library.
// Version 1.0 by Renzo Pigliacampo.

#ifndef LIBTHREAD_H
#define LIBTHREAD_H

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

// Define data structures.
struct lt_task {
  void(*function) (void*);
  void* data;
};

struct lt_queue {
  struct lt_task* task;
  int count;
  int size;
  int start;
  int end;
  CRITICAL_SECTION critical;
  #if _WIN32_WINNT >= 0x0600
    CONDITION_VARIABLE condition;
  #else
    HANDLE condition;
  #endif
};

struct lt_thread {
  HANDLE thread;
  struct lt_queue queue;
  volatile int close;
};

// Create worker thread and tasks queue.
void lt_init(struct lt_thread* thread);

// Add task to worker thread's queue.
void lt_insert(struct lt_thread* thread, void(*function) (void*), void* data);

// Close worker thread.
void lt_close(struct lt_thread* thread);

#endif