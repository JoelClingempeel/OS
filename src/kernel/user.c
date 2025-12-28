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

char* get_user_input() {
    uint32_t buffer_addr;
    asm volatile (
        "movl $2, %%eax;"    // Syscall index 2
        "int $0x80;"         // Trigger syscall
        "movl %%eax, %0;"    // Move result from eax to our variable
        : "=r"(buffer_addr)  // Output: the address returned by kernel
        :                    // No inputs
        : "eax", "memory"    // We tell the compiler we modified eax
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

char* get(){
    uint32_t str_addr;
    get_user_input();
    while (1){
        str_addr = read_user_input();
        if (str_addr != 0) {
            break;
        }
        put_char('N', 0x0c, 0x50);
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

void user_test_program1() {
    put_char('Y', 0x0a, 0x50);  // Green
    uint32_t pid2 = start_process(user_test_program2);
    delay(500);
    print_uint(pid2);
    kill_process(pid2);
    uint32_t pid3 = start_process(user_test_program3);
    delay(500);
    kill_process(pid3);
    print_uint(pid3);
    // print_uint(get_ticks());
    // char* user_input = get();
    // char clear[] = "                              ";
    // printk(clear);
    // delay(200);
    // printk(user_input);
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
    while (1) {
        delay(50);
        put_char('Q', 0x0b, 0x700);  // Cyan
        delay(50);
        put_char(0, 0x0b, 0x700);
    }
    // delay(700);
    // put_char('Y', 0x0b, 0x700);  // Cyan
    // put_char('A', 0x0b, 0x702);
    // put_char('Y', 0x0b, 0x704);
    // while (1);
}
