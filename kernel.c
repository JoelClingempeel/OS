#include "memory.h"

void _kmain(void)
{
    init_mem();

    while (1) {
        asm("hlt"); 
    }
}
