#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "memory.h"

#include <stdint.h>

#define MAX_TASKS 16


typedef struct {
    uint32_t esp;
    uint32_t kstack_top;
    uint32_t kstack_bottom;
} task_struct;

extern task_struct* current_task_ptr;

// Create a new task from a function pointer.
void add_task(void (*entry_point)(void));

// Schedule a new task for a context switch.
extern void schedule();

// Set up scheduling.
void init_scheduling();

#endif  // SCHEDULER_H
