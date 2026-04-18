#include "interrupts.h"
#include "memory.h"
#include "scheduler.h"
#include "shell.h"
#include "tss.h"
#include "user_lib.h"
#include "user_progs.h"
#include "utils.h"


void _kmain(void)
{
    init_mem();
    configure_interrupts();
    init_tss();
    init_scheduling();

    clear_terminal();
    add_task(shell);

    while (1) {
        asm("hlt"); 
    }
}
