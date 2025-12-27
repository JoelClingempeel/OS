#include "scheduler.h"


task_struct* current_task_ptr;
task_struct tasks[MAX_TASKS];
int current_task_index = 0;
int num_tasks = 0;

uint8_t task1_kstack[4096*5];
uint8_t task1_ustack[4096*5];

void add_task(void (*entry_point)(void)){
    task_struct* new_task = &tasks[num_tasks];
    new_task->kstack_bottom = (uint32_t)&task1_kstack;
    new_task->kstack_top = new_task->kstack_bottom + 4096;

    uint32_t* new_task_ptr = (uint32_t*)new_task->kstack_top;
    *(--new_task_ptr) = 0x23;                     // SS (User Data)
    *(--new_task_ptr) = (uint32_t)&task1_ustack + 4096; // ESP (A new User Stack)
    *(--new_task_ptr) = 0x202;                    // EFLAGS (Interrupts enabled)
    *(--new_task_ptr) = 0x1B;                     // CS (User Code)
    *(--new_task_ptr) = (uint32_t)entry_point;    // EIP (Where to start)

    // Segment registers
    for(int i = 0; i < 4; i++) {
        *(--new_task_ptr) = 0x23; 
    }

    // General purpose registers (popad)
    for(int i = 0; i < 8; i++) {
        *(--new_task_ptr) = 0;
    }

    new_task->esp = (uint32_t)new_task_ptr;

    num_tasks++;
}

void schedule(){
    current_task_index = (current_task_index + 1) % num_tasks;
    current_task_ptr = &tasks[current_task_index];
}

void init_scheduling(){
    // The kernel is the 0-th task.
    current_task_index = 0;
    current_task_ptr = &tasks[0];
    num_tasks = 1;
}
