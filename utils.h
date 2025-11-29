#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stddef.h>


extern uint8_t next_char;

void* memcpy(void* dest, const void* src, size_t count);

void printk(char* string, uint8_t format);

void print_uint(uint32_t num, uint8_t format);

#endif  // UTILS_H
