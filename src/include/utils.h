#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stddef.h>

#define TEXT_FORMAT_BYTE 0x07 


extern uint8_t next_char;

// Copy a specified number of bytes.
void* memcpy(void* dest, const void* src, size_t count);

// Initialize a specified number of bytes with some value.
void memset(void* dest, uint8_t value, size_t num_bytes);

// Returns 0 if two strings match and 1 otherwise.
int strcmp(char* str1, char* str2);

// Clears terminal.
void clear_terminal();

// Clears a line on the terminal.
void clear_line(int line);

// Print a string.
void printk(char* string);

// Print a string at a chosen line.
void printk_line(char* string, int line);

// Print an unsigned integer.
void print_uint(uint32_t num);

#endif  // UTILS_H
