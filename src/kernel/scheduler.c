#include "scheduler.h"


task_struct* current_task_ptr;
task_struct tasks[MAX_TASKS];
int current_task_index = 0;

int add_task(void (*entry_point)(void)){
    int i = 1;
    while (tasks[i].task_index != 0) {
        i++;
    }

    // TODO Clear stacks before rescheduling a new process.

    // Allocate one page each for the kernel and user stacks.
    uint32_t kstack_page = vram_border;
    vram_border += PAGE_SIZE;
    map_page(kstack_page, FLAG_PRESENT | FLAG_RW);

    uint32_t ustack_page = vram_border;
    vram_border += PAGE_SIZE;
    map_page(ustack_page, FLAG_PRESENT | FLAG_RW | FLAG_USER);

    task_struct* new_task = &tasks[i];
    new_task->kstack_bottom = kstack_page;
    new_task->kstack_top = kstack_page + PAGE_SIZE;
    new_task->ustack_bottom = ustack_page;
    new_task->active = 1;
    new_task->task_index = i;

    uint32_t* new_task_ptr = (uint32_t*)new_task->kstack_top;
    *(--new_task_ptr) = 0x23;                          // SS (User Data)
    *(--new_task_ptr) = ustack_page + PAGE_SIZE;       // ESP (top of user stack page)
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

    return i;
}

void kill_task(int pid){
    task_struct* task = &tasks[pid];
    unmap_page(task->kstack_bottom);
    unmap_page(task->ustack_bottom);
    task->active = 0;  // Skip when scheduling.
    task->task_index = 0;  // Allow being rescheduled.
}

void schedule(){
    do {
        current_task_index = (current_task_index % (MAX_TASKS - 1)) + 1;
    } while(tasks[current_task_index].active == 0);
    current_task_ptr = &tasks[current_task_index];
}

void init_scheduling(){
    // The kernel is the 0-th task.
    current_task_index = 0;
    current_task_ptr = &tasks[0];
}
