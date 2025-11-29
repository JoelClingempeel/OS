#include "utils.h"


uint8_t next_char = 0;

void* memcpy(void* dest, const void* src, size_t count) {
    char* dst8 = (char*)dest;
    const char* src8 = (const char*)src;
    while (count--) {
        *dst8++ = *src8++;
    }
    return dest;
}

void printk(char* string, uint8_t format) {
    char *video_memory = (char *)0xb8000;
    int i = 0;
    while (string[i] != 0) {
        video_memory[2 * next_char] = string[i];
        video_memory[2 * next_char + 1] = format;
        i++;
        next_char++;
    }
}

void print_uint(uint32_t num, uint8_t format) {
    char *video_memory = (char *)0xb8000;
    uint8_t digits[10];
    for (int i = 0; i < 10; i++) {
        digits[i] = num % 10;
        num /= 10;
    }
    int j = 9;
    while (digits[j] == 0) {
        j--;
    }
    while (j >= 0) {
        video_memory[next_char] = digits[j] + 48;
        video_memory[next_char + 1] = format;
        j--;
        next_char += 2;
    }
}
