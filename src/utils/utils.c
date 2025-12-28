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

void memset(void* dest, uint8_t value, size_t num_bytes) {
    uint8_t* curr = (uint8_t*)dest;
    for (size_t i = 0; i < num_bytes; i++) {
        curr[i] = value;
    }
}

void clear_terminal() {
    char *video_memory = (char *)0xb8000;
    for (int i = 0; i < 2000; i++) {
        video_memory[2 * i] = 0;
    }
}

void printk(char* string) {
    char *video_memory = (char *)0xb8000;
    int i = 0;
    while (string[i] != 0) {
        video_memory[2 * next_char] = string[i];
        video_memory[2 * next_char + 1] = TEXT_FORMAT_BYTE;
        i++;
        next_char++;
    }
}

void print_uint(uint32_t num) {
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
        video_memory[next_char + 1] = TEXT_FORMAT_BYTE;
        j--;
        next_char += 2;
    }
}
