#ifndef CO_H
#define CO_H

#include <stdlib.h>
#include <setjmp.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>

#define MAX_STACK_SIZE 64 * 1024

typedef enum co_status_t {
  Create = 0,
  Run,
  Finish
} CoStatus;

// 协程结构体
typedef struct co {
  const char* co_name;
  void (*func)(void *);
  void* func_arg;
  CoStatus co_status;
  struct co* next;
  struct co* prev;
  struct co* wait;
  jmp_buf co_yield_place;
  char stack[MAX_STACK_SIZE];
} Co;

struct co* co_start(const char *name, void (*func)(void *), void *arg);
void co_yield();
void co_wait(struct co *co);
#endif