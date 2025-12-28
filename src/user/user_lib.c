#include "user_lib.h"


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

char* get_user_input(uint32_t line) {
    uint32_t buffer_addr;
    asm volatile (
        "int $0x80"
        : "=a"(buffer_addr)  // eax = return value (output)
        : "a"(2),            // eax = 2 (syscall number)
          "b"(line)          // ebx = line (your single argument)
        : "memory"           // Tell compiler memory might be modified
    );
    return (char*)buffer_addr;
}

uint32_t read_user_input(){
    uint32_t eax = 3;
    asm volatile (
        "int $0x80"
        : "+a"(eax)
        :
        : "memory"
    );
    return eax;
}

void delay(int delay_length){
    for (int i = 0; i < delay_length * 1000000; i++) {
        __asm__ volatile ("nop");
    }
}

char* get(uint32_t line){
    uint32_t str_addr;
    get_user_input(line);
    while (1){
        str_addr = read_user_input();
        if (str_addr != 0) {
            break;
        }
        delay(1);
    }
   return (char*) str_addr;
}

uint32_t start_process(void (*func_addr)(void)){
    uint32_t pid;
    asm volatile (
        "int $0x80"
        : "=a"(pid)       /* Output: Put the final value of EAX into 'pid' */
        : "a"(4),         /* Input:  Put 4 into EAX before starting */
          "b"(func_addr)  /* Input:  Put function pointer into EBX */
        : "memory"
    );
    return pid;
}

void kill_process(int pid){
    asm volatile (
        "int $0x80"
        :
        : "a"(5),
          "b"(pid)
        : "memory"
    );
}

void user_print_line(char* string, int line){
    asm volatile (
        "int $0x80"
        : 
        : "a"(6),
          "b"((uint32_t)string),
          "c"(line)
        : "memory"
    );
}

void user_clear_line(int line){
    asm volatile (
        "int $0x80"
        : 
        : "a"(7),
          "b"(line)
        : "memory"
    );
}

void user_clear_terminal(){
    for (int i = 0; i < 25; i++) {
        user_clear_line(i);
    }
}
