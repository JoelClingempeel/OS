#ifndef USER_H
#define USER_H

#include <stdint.h>

#include "utils.h"


// Print a character using sys_put_char.
void put_char(uint32_t character, uint32_t color, uint32_t location);

// Get the number of timer ticks using sys_get_ticks.
uint32_t get_ticks();

// Waste delay_length * 1,000,000 CPU cycles with NOPs.
void delay(int delay_length);

// Get user input from the terminal.
char* get();

// Start user program and get PID.
uint32_t start_process(void (*func_addr)(void));

// Put a green Y on the screen and after waiting a bit display the number of ticks.
void user_test_program1();

// Make a red N repeatedly appear and disappear.
void user_test_program2();

// Wait a while then display YAY in blue.
void user_test_program3();

#endif  // USER_H
