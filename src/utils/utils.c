#include "utils.h"
#include "interrupts.h"


uint8_t next_char = 0;

void* memcpy(void* dest, const void* src, size_t count) {
    char* dst8 = (char*)dest;
    const char* src8 = (const char*)src;
    while (count--) {
        *dst8++ = *src8++;
    }
    return dest;
}

void strcpy(char* dest, char* src) {
    while (*src != '\0') {
        *dest++ = *src++;
    }
    *dest = '\0';
}

void memset(void* dest, uint8_t value, size_t num_bytes) {
    uint8_t* curr = (uint8_t*)dest;
    for (size_t i = 0; i < num_bytes; i++) {
        curr[i] = value;
    }
}

size_t strlen(const char* str) {
    size_t i = 0;
    while (str[i] != '\0') i++;
    return i;
}

int strcmp(char* str1, char* str2) {
    int i = 0;
    while (str1[i] == str2[i]) {
        if (str1[i] == '\0') {
            return 0;
        }
        i++;
    }
    return 1;
}

int tokencmp(char* str1, char* str2) {
    int i = 0;
    while (str1[i] == str2[i]) {
        if (str2[i] == '\0') {
            return 0;
        }
        i++;
    }
    if (str2[i] == '\0' && str1[i] == ' ') {
        return 0;
    }
    return 1;
}

void clear_terminal() {
    char *video_memory = (char *)0xb8000;
    for (int i = 0; i < 2000; i++) {
        video_memory[2 * i] = 0;
    }
}

void clear_line(int line) {
    char *video_memory = (char *)(0xb8000 + 160 * line);
    for (int i = 0; i < 80; i++) {
        video_memory[2 * i]     = 0;
        video_memory[2 * i + 1] = TEXT_FORMAT_BYTE;
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

void printk_line(char* string, int line) {
    char *video_memory = (char *)(0xb8000 + 160 * line);
    int i = 0;
    while (string[i] != 0) {
        video_memory[2 * i] = string[i];
        video_memory[2 * i + 1] = TEXT_FORMAT_BYTE;
        i++;
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

void serial_putc(char c) {
    outb(0x3F8, c);
}

void serial_print(const char* s) {
    for (int i = 0; s[i] != '\0'; i++) {
        serial_putc(s[i]);
    }
}

void uint_to_str(uint32_t n, char* out) {
    if (n == 0) { out[0] = '0'; out[1] = '\0'; return; }
    char tmp[12];
    int i = 0;
    while (n > 0) { tmp[i++] = '0' + (n % 10); n /= 10; }
    int j = 0;
    while (i > 0) out[j++] = tmp[--i];
    out[j] = '\0';
}

uint32_t str_to_uint(char* s) {
    uint32_t n = 0;
    while (*s >= '0' && *s <= '9') n = n * 10 + (*s++ - '0');
    return n;
}

void serial_print_uint(uint32_t num) {
    char digits[10];
    int i = 0;
    if (num == 0) { serial_putc('0'); return; }
    while (num > 0) {
        digits[i++] = '0' + (num % 10);
        num /= 10;
    }
    while (i > 0) {
        serial_putc(digits[--i]);
    }
}
