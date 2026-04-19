#include "scheduler.h"

#define STACK_PAGES 1

task_struct* current_task_ptr;
task_struct tasks[MAX_TASKS];
int current_task_index = 0;

int add_task(void (*entry_point)(void), char* args){
    int i = 1;
    while (tasks[i].task_index != 0) {
        i++;
    }

    // If this slot held a self-terminated task, its stacks were not freed at kill
    // time (to avoid a use-after-free). Free them now before reusing the slot.
    if (tasks[i].kstack_bottom != 0) {
        for (int p = 0; p < STACK_PAGES; p++) {
            unmap_page(tasks[i].kstack_bottom + p * PAGE_SIZE);
            unmap_page(tasks[i].ustack_bottom + p * PAGE_SIZE);
        }
        tasks[i].kstack_bottom = 0;
        tasks[i].ustack_bottom = 0;
    }

    // Allocate STACK_PAGES pages each for the kernel and user stacks.
    uint32_t kstack_page = vram_border;
    vram_border += STACK_PAGES * PAGE_SIZE;
    for (int p = 0; p < STACK_PAGES; p++)
        map_page(kstack_page + p * PAGE_SIZE, FLAG_PRESENT | FLAG_RW | FLAG_USER); // TODO: remove FLAG_USER once user/kernel memory protection is sorted out

    uint32_t ustack_page = vram_border;
    vram_border += STACK_PAGES * PAGE_SIZE;
    for (int p = 0; p < STACK_PAGES; p++)
        map_page(ustack_page + p * PAGE_SIZE, FLAG_PRESENT | FLAG_RW | FLAG_USER);

    task_struct* new_task = &tasks[i];
    new_task->kstack_bottom = kstack_page;
    new_task->kstack_top = kstack_page + STACK_PAGES * PAGE_SIZE;
    new_task->ustack_bottom = ustack_page;
    new_task->active = 1;
    new_task->task_index = i;

    uint32_t* new_task_ptr = (uint32_t*)new_task->kstack_top;
    *(--new_task_ptr) = 0x23;                          // SS (User Data)
    *(--new_task_ptr) = ustack_page + STACK_PAGES * PAGE_SIZE;  // ESP (top of user stack)
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

    // Set task args.
    strcpy(new_task->args, args);

    return i;
}

void kill_task(int pid){
    task_struct* task = &tasks[pid];
    // Don't unmap the stacks if the task is killing itself — the stack is still
    // in use until the next context switch and unmapping it here causes a page fault
    // on return from the syscall. Stack pages are reclaimed when the slot is reused.
    if (pid != (int)current_task_ptr->task_index) {
        for (int p = 0; p < STACK_PAGES; p++) {
            unmap_page(task->kstack_bottom + p * PAGE_SIZE);
            unmap_page(task->ustack_bottom + p * PAGE_SIZE);
        }
    }
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
