#include "interrupts.h"
#include "scheduler.h"
#include "syscalls.h"
#include "tty.h"
#include "utils.h"


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
    tty.row = regs->ebx;
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

static uint32_t sys_print_line(struct registers* regs) {
    printk_line((char*)regs->ebx, regs->ecx);
    return 0;
}

static uint32_t sys_clear_line(struct registers* regs) {
    clear_line(regs->ebx);
    return 0;
}

void do_syscall(struct registers* regs){
    uint32_t (*syscall_table[])(struct registers*) = {
        sys_get_ticks,     // Index 0
        sys_put_char,      // Index 1
        sys_get_input,     // Index 2
        sys_read_input,    // Index 3
        sys_start_process, // Index 4
        sys_kill_process,  // Index 5,
        sys_print_line,    // Index 6,
        sys_clear_line,    // Index 7
    };
    uint32_t syscall_number = regs->eax;
    regs->eax = syscall_table[syscall_number](regs);
}
