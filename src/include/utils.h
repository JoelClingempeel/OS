#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stddef.h>

#define TEXT_FORMAT_BYTE 0x1F


extern uint8_t next_char;

// Copy a specified number of bytes.
void* memcpy(void* dest, const void* src, size_t count);

// Copy a string into a buffer.
void strcpy(char* dest, char* src);

// Initialize a specified number of bytes with some value.
void memset(void* dest, uint8_t value, size_t num_bytes);

// Returns 0 if two strings match and 1 otherwise.
int strcmp(char* str1, char* str2);

// Returns 0 if two strings match up to the first space and 1 otherwise.
int tokencmp(char* str1, char* str2);

// Returns the length of a string not including the null terminator.
size_t strlen(const char* str);

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

// Write a character to COM1 serial.
void serial_putc(char c);

// Write a string to COM1 serial.
void serial_print(const char* s);

// Write an unsigned integer to COM1 serial.
void serial_print_uint(uint32_t num);

// Write a string literal to COM1 serial without relying on .rodata.
#define SERIAL_PRINT(str) do { char _m[] = str; serial_print(_m); } while(0)

// Converts a uint to a string.
void uint_to_str(uint32_t n, char* out);

// Converts a string ot a uint.
uint32_t str_to_uint(char* s);

int str_eq(const char* a, const char* b);

#endif  // UTILS_H
