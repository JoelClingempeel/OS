#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stddef.h>


extern uint8_t next_char;

// Copy a specified number of bytes.
void* memcpy(void* dest, const void* src, size_t count);

// Print a string.
void printk(char* string, uint8_t format);

// Print an unsigned integer.
void print_uint(uint32_t num, uint8_t format);

#endif  // UTILS_H
