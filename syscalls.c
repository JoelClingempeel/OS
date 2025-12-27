#include "interrupts.h"
#include "syscalls.h"


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

void do_syscall(struct registers* regs){
    if (regs->eax == 0) {
        regs->eax = sys_get_ticks(regs);
    } else if (regs->eax == 1) {
        regs->eax = sys_put_char(regs);
    }
}
