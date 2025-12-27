#include <stdint.h>

#include "utils.h"


// Print a character using sys_put_char.
void put_char(uint32_t character, uint32_t color, uint32_t location);

// Get the number of timer ticks using sys_get_ticks.
uint32_t get_ticks();

// Test user program to demonstrate syscalls.
void user_test_program();
