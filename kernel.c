#include "interrupts.h"
#include "memory.h"
#include "utils.h"


void _kmain(void)
{
    init_mem();
    configure_interrupts();

    asm volatile ("int $0x80");  // Test software interrupt.

    while (1) {
        asm("hlt"); 
    }
}
