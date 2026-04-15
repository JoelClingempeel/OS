#ifndef USER_LIB_H
#define USER_LIB_H

#include <stdint.h>

#include "fs.h"
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

// Converts a uint to an ascii string.
void uint_to_ascii(uint32_t num, char* buffer);

// Read a file into buf (must be at least 512 bytes). Returns 0 on success, -1 if not found.
int file_read(char* filename, uint8_t* buf);

// Write buf to a file, creating it if it doesn't exist. Returns 0 on success, -1 if directory full.
int file_write(char* filename, uint8_t* buf, uint32_t size);

// Fill names with all filenames. Returns the number of entries written, up to max_names.
int file_list(char names[][FS_MAX_FILENAME], int max_names);

#endif  // USER_LIB_H
