#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "memory.h"

#include <stdint.h>

#define MAX_TASKS 4


typedef struct {
    uint32_t esp;
    uint32_t kstack_top;
    uint32_t kstack_bottom;
    uint8_t active;
} task_struct;

extern task_struct* current_task_ptr;

// TODO Use kmalloc instead of these after debugging kmalloc / page allocation.
extern uint8_t user_stacks[MAX_TASKS][4096];
extern uint8_t kernel_stacks[MAX_TASKS][4096];

// Create a new task from a function pointer.
void add_task(void (*entry_point)(void));

// Schedule a new task for a context switch.
extern void schedule();

// Set up scheduling.
void init_scheduling();

#endif  // SCHEDULER_H
