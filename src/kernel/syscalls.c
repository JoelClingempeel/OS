#include "interrupts.h"
#include "scheduler.h"
#include "syscalls.h"
#include "tty.h"
#include "utils.h"


// TODO Create a syscall table.

static uint32_t sys_get_ticks(struct registers* regs) {
    return timer_ticks;
}

static uint32_t sys_put_char(struct registers* regs) {
    char* video_memory = (char*)0xb8000;
    video_memory[regs->edx] = regs->ebx;
    video_memory[regs->edx+1] = regs->ecx;
    return 0;
}

static uint32_t sys_get_input(struct registers* regs) {
    memset(&tty.input_buffer, 0, 1024);
    tty.index = 0;
    tty.active = 1;
    tty.task_index = current_task_ptr->task_index;
    return 0;
}

static uint32_t sys_read_input(struct registers* regs) {
    if (tty.active > 0) {
        return 0;
    }
    return (uint32_t)&tty.input_buffer;
}

static uint32_t sys_start_process(struct registers* regs) {
    int pid = add_task((void (*)(void))regs->ebx);
    return (uint32_t)pid;
}

static uint32_t sys_kill_process(struct registers* regs) {
    kill_task((int)(regs->ebx));
    return 0;
}

void do_syscall(struct registers* regs){
    if (regs->eax == 0) {
        regs->eax = sys_get_ticks(regs);
    } else if (regs->eax == 1) {
        regs->eax = sys_put_char(regs);
    } else if (regs->eax == 2) {
        regs->eax = sys_get_input(regs);
    } else if (regs->eax == 3) {
        regs->eax = sys_read_input(regs);
    } else if (regs->eax == 4) {
        regs->eax = sys_start_process(regs);
    } else if (regs->eax == 5) {
        regs->eax = sys_kill_process(regs);
    }
}
