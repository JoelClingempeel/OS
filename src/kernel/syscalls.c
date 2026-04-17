#include "fs.h"
#include "interrupts.h"
#include "memory.h"
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
    char input_str[] = TTY_GREETING;
    printk_line(input_str, regs->ebx);
    update_cursor(TTY_GREET_LEN, regs->ebx);
    return 0;
}

static uint32_t sys_read_input(struct registers* regs) {
    if (tty.active > 0) {
        return 0;
    }
    return (uint32_t)&tty.input_buffer;
}

static uint32_t sys_start_process(struct registers* regs) {
    int pid = add_task((void (*)(void))regs->ebx, (char*)regs->ecx);
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

static uint32_t sys_fs_read(struct registers* regs) {
    return (uint32_t)fs_read((char*)regs->ebx, (uint8_t*)regs->ecx);
}

static uint32_t sys_fs_write(struct registers* regs) {
    return (uint32_t)fs_write((char*)regs->ebx, (uint8_t*)regs->ecx, regs->edx);
}

static uint32_t sys_fs_list(struct registers* regs) {
    return (uint32_t)fs_list((char (*)[FS_MAX_FILENAME])regs->ebx, (int)regs->ecx);
}

static uint32_t sys_get_pid(struct registers* regs) {
    return (uint32_t)current_task_ptr->task_index;
}

static uint32_t sys_is_running(struct registers* regs) {
    return (uint32_t)tasks[regs->ebx].active;
}

static uint32_t sys_alloc_page(struct registers* regs) {
    uint32_t addr = vram_border;
    map_page(addr, FLAG_PRESENT | FLAG_RW | FLAG_USER);
    vram_border += PAGE_SIZE;
    return addr;
}

static uint32_t sys_get_args(struct registers* regs) {
    return (uint32_t)current_task_ptr->args;
}

void do_syscall(struct registers* regs){
    uint32_t (*syscall_table[])(struct registers*) = {
        sys_get_ticks,     // Index 0
        sys_put_char,      // Index 1
        sys_get_input,     // Index 2
        sys_read_input,    // Index 3
        sys_start_process, // Index 4
        sys_kill_process,  // Index 5
        sys_print_line,    // Index 6
        sys_clear_line,    // Index 7
        sys_fs_read,       // Index 8
        sys_fs_write,      // Index 9
        sys_fs_list,       // Index 10
        sys_get_pid,       // Index 11
        sys_is_running,    // Index 12
        sys_alloc_page,    // Index 13
        sys_get_args,      // Index 14
    };
    uint32_t syscall_number = regs->eax;
    regs->eax = syscall_table[syscall_number](regs);
}
