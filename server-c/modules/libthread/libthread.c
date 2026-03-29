// libthread: Worker thread management library.
// Version 1.0 by Renzo Pigliacampo.

#include "libthread.h"

// Start thread's procedure.
static DWORD WINAPI lt_start(LPVOID lpParam) {
  struct lt_thread* thread = (struct lt_thread*) lpParam;
  struct lt_task task;
  // Start tasks thread polling.
  while (!thread->close) {
    // Wait for available task.
    EnterCriticalSection(&thread->queue.critical);
    while (thread->queue.count == 0 && !thread->close) {
      #if _WIN32_WINNT >= 0x0600
        SleepConditionVariableCS(&thread->queue.condition, &thread->queue.critical, INFINITE);
      #else
        LeaveCriticalSection(&thread->queue.critical);
        WaitForSingleObject(thread->queue.condition, INFINITE);
        EnterCriticalSection(&thread->queue.critical);
      #endif
    }
    if (thread->close) {
      LeaveCriticalSection(&thread->queue.critical); break;
    }
    // Pop task from queue.
    task = thread->queue.task[thread->queue.start];
    thread->queue.start = (thread->queue.start + 1) % thread->queue.size;
    thread->queue.count--;
    LeaveCriticalSection(&thread->queue.critical);
    // Execute task.
    task.function(task.data);
  }
  return 0;
}

// Create worker thread and tasks queue.
void lt_init(struct lt_thread* thread) {
  // Initialize task queue.
  thread->close = 0;
  thread->queue.size = 32;
  thread->queue.count = 0;
  thread->queue.start = 0;
  thread->queue.end = 0;
  thread->queue.task = (struct lt_task*) malloc(sizeof(struct lt_task) * thread->queue.size);
  InitializeCriticalSection(&thread->queue.critical);
  #if _WIN32_WINNT >= 0x0600
    InitializeConditionVariable(&thread->queue.condition);
  #else
    thread->queue.condition = CreateEvent(NULL, FALSE, FALSE, NULL);
  #endif
  // Create worker thread.
  thread->thread = CreateThread(NULL, 0, lt_start, thread, 0, NULL);
}

// Add task to worker thread's queue.
void lt_insert(struct lt_thread* thread, void(*function) (void*), void* data) {
  EnterCriticalSection(&thread->queue.critical);
  // Resize queue if full.
  if (thread->queue.count >= thread->queue.size) {
    int size = thread->queue.size * 2;
    thread->queue.task = (struct lt_task*) realloc(thread->queue.task, sizeof(struct lt_task) * size);
    // Manage queue wrap around.
    if (thread->queue.end < thread->queue.start) {
      memcpy(thread->queue.task + thread->queue.size, thread->queue.task, thread->queue.end * sizeof(struct lt_task));
      thread->queue.end += thread->queue.size;
    }
    thread->queue.size = size;
  }
  // Create task.
  thread->queue.task[thread->queue.end].function = function;
  thread->queue.task[thread->queue.end].data = data;
  thread->queue.end = (thread->queue.end + 1) % thread->queue.size;
  thread->queue.count++;
  LeaveCriticalSection(&thread->queue.critical);
  #if _WIN32_WINNT >= 0x0600
    WakeConditionVariable(&thread->queue.condition);
  #else
    SetEvent(thread->queue.condition);
  #endif
}

// Exit worker thread gracefully once free.
void lt_close(struct lt_thread* thread) {
  thread->close = 1;
  #if _WIN32_WINNT >= 0x0600
    WakeConditionVariable(&thread->queue.condition);
  #else
    SetEvent(thread->queue.condition);
  #endif
  WaitForSingleObject(thread->thread, INFINITE);
  // Clear resources and free allocated memory.
  CloseHandle(thread->thread);
  #if _WIN32_WINNT < 0x0600
    CloseHandle(thread->queue.condition);
  #endif
  DeleteCriticalSection(&thread->queue.critical);
  free(thread->queue.task);
  free(thread);
}