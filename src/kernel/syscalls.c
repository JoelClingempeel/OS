#include "files.h"
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

static uint32_t sys_fs_mkdir(struct registers* regs) {
    make_file((char*)regs->ebx, 1);
    return 0;
}

static uint32_t sys_fs_ls(struct registers* regs) {
    char* path = (char*)regs->ebx;
    char* buf = (char*)regs->ecx;
    char names[8][MAX_NAME];
    int count = lsdir(path, names, 16);
    int pos = 0;
    for (int i = 0; i < count; i++) {
        for (int j = 0; names[i][j]; j++) buf[pos++] = names[i][j];
        if (i + 1 < count) buf[pos++] = ',';
    }
    buf[pos] = '\0';
    return 0;
}

static uint32_t sys_fs_read(struct registers* regs) {
    return (uint32_t)read_path((char*)regs->ebx, (char*)regs->ecx);
}

static uint32_t sys_fs_write(struct registers* regs) {
    char* path = (char*)regs->ebx;
    char* buf = (char*)regs->ecx;
    make_file(path, 0);
    return (uint32_t)write_path(path, buf);
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

static uint32_t sys_fs_rm(struct registers* regs) {
    return (uint32_t)delete_file((char*)regs->ebx);
}

static uint32_t sys_fs_rmdir(struct registers* regs) {
    return (uint32_t)delete_dir((char*)regs->ebx);
}

static uint32_t sys_fs_rename(struct registers* regs) {
    return (uint32_t)rename_path((char*)regs->ebx, (char*)regs->ecx);
}

static uint32_t sys_get_input_prefilled(struct registers* regs) {
    char* content = (char*)regs->ecx;
    memset(&tty.input_buffer, 0, 1024);
    strcpy(tty.input_buffer, content);
    tty.index = (uint16_t)strlen(content);
    tty.active = 1;
    tty.row = regs->ebx;
    tty.task_index = current_task_ptr->task_index;
    char input_str[] = TTY_GREETING;
    printk_line(input_str, regs->ebx);
    char* video_memory = (char*)0xb8000;
    for (uint16_t i = 0; i < tty.index; i++) {
        video_memory[2*i + 160*tty.row + 2*TTY_GREET_LEN - 1] = content[i];
    }
    update_cursor(TTY_GREET_LEN + tty.index, tty.row);
    return 0;
}

void do_syscall(struct registers* regs){
    uint32_t syscall_number = regs->eax;
    switch (syscall_number) {
        case  0: regs->eax = sys_get_ticks(regs);     break;
        case  1: regs->eax = sys_put_char(regs);       break;
        case  2: regs->eax = sys_get_input(regs);      break;
        case  3: regs->eax = sys_read_input(regs);     break;
        case  4: regs->eax = sys_start_process(regs);  break;
        case  5: regs->eax = sys_kill_process(regs);   break;
        case  6: regs->eax = sys_print_line(regs);     break;
        case  7: regs->eax = sys_clear_line(regs);     break;
        case  8: regs->eax = sys_fs_mkdir(regs);       break;
        case  9: regs->eax = sys_fs_ls(regs);          break;
        case 10: regs->eax = sys_fs_read(regs);        break;
        case 11: regs->eax = sys_fs_write(regs);       break;
        case 12: regs->eax = sys_get_pid(regs);        break;
        case 13: regs->eax = sys_is_running(regs);     break;
        case 14: regs->eax = sys_alloc_page(regs);     break;
        case 15: regs->eax = sys_get_args(regs);       break;
        case 16: regs->eax = sys_fs_rm(regs);          break;
        case 17: regs->eax = sys_fs_rmdir(regs);          break;
        case 18: regs->eax = sys_get_input_prefilled(regs); break;
        case 19: regs->eax = sys_fs_rename(regs);           break;
        default:
            SERIAL_PRINT("[syscall] bad syscall number: ");
            serial_print_uint(syscall_number);
            SERIAL_PRINT("\n");
            regs->eax = (uint32_t)-1;
            break;
    }
}
