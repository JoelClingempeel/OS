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

void put_char(uint32_t character, uint32_t color, uint32_t location){
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

void delay(int delay_length){
    for (int i = 0; i < delay_length * 1000000; i++) {
        __asm__ volatile ("nop");
    }
}

void user_test_program1() {
    put_char('Y', 0x0a, 0x50);  // Green
    delay(500);
    print_uint(get_ticks());
    while(1) {}
}

void user_test_program2() {
    while (1) {
        delay(50);
        put_char('N', 0x0c, 0x350);  // Red
        delay(50);
        put_char(0, 0x0c, 0x350);
    }
}

void user_test_program3() {
    delay(700);
    put_char('Y', 0x0b, 0x700);  // Cyan
    put_char('A', 0x0b, 0x702);
    put_char('Y', 0x0b, 0x704);
    while (1);
}
