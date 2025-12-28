#include "scheduler.h"


task_struct* current_task_ptr;
task_struct tasks[MAX_TASKS];
int current_task_index = 0;
int num_tasks = 0;

uint8_t user_stacks[MAX_TASKS][4096] __attribute__((aligned(4096)));
uint8_t kernel_stacks[MAX_TASKS][4096] __attribute__((aligned(4096)));

int add_task(void (*entry_point)(void)){
    int i = 1;
    while (tasks[i].task_index != 0) {
        i++;
    }

    // TODO Clear stacks before rescheduling a new process.

    task_struct* new_task = &tasks[i];
    new_task->kstack_bottom = (uint32_t)&kernel_stacks[i][4096];
    new_task->kstack_top = new_task->kstack_bottom + 4096;
    new_task->active = 1;
    new_task->task_index = i;

    uint32_t* new_task_ptr = (uint32_t*)new_task->kstack_top;
    *(--new_task_ptr) = 0x23;                     // SS (User Data)
    *(--new_task_ptr) = (uint32_t)&user_stacks[i][4096]; // ESP (A new User Stack)
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
    return i;
}

void kill_task(int pid){
    task_struct* task = &tasks[pid];
    // TODO Clear block when memset is available.
    task->active = 0;  // Skip when scheduling.
    task->task_index = 0;  // Allow being rescheduled.
}

void schedule(){
    do {
        current_task_index = (current_task_index + 1) % num_tasks;
    } while(tasks[current_task_index].active == 0);
    current_task_ptr = &tasks[current_task_index];
}

void init_scheduling(){
    // The kernel is the 0-th task.
    current_task_index = 0;
    current_task_ptr = &tasks[0];
    num_tasks = 1;
}
