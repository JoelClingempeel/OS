#ifndef USER_LIB_H
#define USER_LIB_H

#include <stdint.h>

#include "utils.h"


// Print a character using sys_put_char.
void put_char(uint32_t character, uint32_t color, uint32_t location);

// Get the number of timer ticks using sys_get_ticks.
uint32_t get_ticks();

// Waste delay_length * 1,000,000 CPU cycles with NOPs.
void delay(int delay_length);

// Get user input from the terminal.
char* get(uint32_t line);

// Start user program and get PID.
uint32_t start_process(void (*func_addr)(void));

// Kill a process.
void kill_process(int pid);

// Print a string at a designated line.
void user_print_line(char* string, int line);

// Clears a designated line in the terminal.
void user_clear_line(int line);

// Clears the terminal.
void user_clear_terminal();

#endif  // USER_LIB_H
