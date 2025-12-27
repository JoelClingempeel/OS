#include "user.h"


uint32_t get_ticks(){
    uint32_t count = 0;
    asm volatile (
        "int $0x80"
        : "+a"(count)
        :
        : "memory"
    );
    return count;
}

void put_char(uint32_t character, uint32_t color, uint32_t location) {
    asm volatile (
        "int $0x80"
        : 
        : "a"(1),         // eax = 1 (syscall number)
          "b"(character), // ebx = character
          "c"(color),     // ecx = color
          "d"(location)   // edx = location
        : "memory"        // Tell compiler memory might be modified
    );
}

void user_test_program() {
   put_char('Y', 0x0a, 0x50);

    for (uint32_t i = 0; i < 500000000; i++) {
        __asm__ volatile ("nop");
    }

    print_uint(get_ticks());

    while(1) {}
}
