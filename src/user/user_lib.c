#include "user_lib.h"


uint32_t get_ticks(){
    uint32_t count = 0;
    asm volatile (
        "int $0x80"
        : "+a"(count)
        :
        : "memory"
    );
    return count;
}

void put_char(uint32_t character, uint32_t color, uint32_t location){
    asm volatile (
        "int $0x80"
        : 
        : "a"(1),         // eax = 1 (syscall number)
          "b"(character), // ebx = character
          "c"(color),     // ecx = color
          "d"(location)   // edx = location
        : "memory"        // Tell compiler memory might be modified
    );
}

char* get_user_input(uint32_t line) {
    uint32_t buffer_addr;
    asm volatile (
        "int $0x80"
        : "=a"(buffer_addr)  // eax = return value (output)
        : "a"(2),            // eax = 2 (syscall number)
          "b"(line)          // ebx = line (your single argument)
        : "memory"           // Tell compiler memory might be modified
    );
    return (char*)buffer_addr;
}

uint32_t read_user_input(){
    uint32_t eax = 3;
    asm volatile (
        "int $0x80"
        : "+a"(eax)
        :
        : "memory"
    );
    return eax;
}

void delay(int delay_length){
    for (int i = 0; i < delay_length * 1000000; i++) {
        __asm__ volatile ("nop");
    }
}

char* get_prefilled(uint32_t line, char* content) {
    uint32_t str_addr;
    asm volatile (
        "int $0x80"
        :
        : "a"(18),
          "b"(line),
          "c"((uint32_t)content)
        : "memory"
    );
    while (1) {
        str_addr = read_user_input();
        if (str_addr != 0) break;
        delay(1);
    }
    return (char*)str_addr;
}

char* get(uint32_t line){
    uint32_t str_addr;
    get_user_input(line);
    while (1){
        str_addr = read_user_input();
        if (str_addr != 0) {
            break;
        }
        delay(1);
    }
   return (char*) str_addr;
}

uint32_t start_process(void (*func_addr)(void), char* args){
    uint32_t pid;
    asm volatile (
        "int $0x80"
        : "=a"(pid)       /* Output: Put the final value of EAX into 'pid' */
        : "a"(4),         /* Input:  Put 4 into EAX before starting */
          "b"(func_addr),  /* Input:  Put function pointer into EBX */
          "c"((uint32_t)args)  /* Input:  Put args pointer into ECX */
        : "memory"
    );
    return pid;
}

void kill_process(int pid){
    asm volatile (
        "int $0x80"
        :
        : "a"(5),
          "b"(pid)
        : "memory"
    );
}

void user_print_line(char* string, int line){
    asm volatile (
        "int $0x80"
        : 
        : "a"(6),
          "b"((uint32_t)string),
          "c"(line)
        : "memory"
    );
}

void user_clear_line(int line){
    asm volatile (
        "int $0x80"
        : 
        : "a"(7),
          "b"(line)
        : "memory"
    );
}

void user_clear_terminal(){
    for (int i = 0; i < 25; i++) {
        user_clear_line(i);
    }
}

int get_pid() {
    uint32_t pid;
    asm volatile (
        "int $0x80"
        : "=a"(pid)
        : "a"(12)
        : "memory"
    );
    return (int)pid;
}

int is_running(int pid) {
    uint32_t ret;
    asm volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(13),
          "b"((uint32_t)pid)
        : "memory"
    );
    return (int)ret;
}

void wait_for(int pid) {
    while (is_running(pid)) {
        delay(1);
    }
}

int file_read(char* filename, uint8_t* buf) {
    uint32_t ret;
    asm volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(8),
          "b"((uint32_t)filename),
          "c"((uint32_t)buf)
        : "memory"
    );
    return (int)ret;
}

int file_write(char* filename, uint8_t* buf, uint32_t size) {
    uint32_t ret;
    asm volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(9),
          "b"((uint32_t)filename),
          "c"((uint32_t)buf),
          "d"(size)
        : "memory"
    );
    return (int)ret;
}

int file_list(char names[][MAX_NAME], int max_names) {
    uint32_t ret;
    asm volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(10),
          "b"((uint32_t)names),
          "c"((uint32_t)max_names)
        : "memory"
    );
    return (int)ret;
}

void* alloc_page() {
    uint32_t addr;
    asm volatile (
        "int $0x80"
        : "=a"(addr)
        : "a"(14)
        : "memory"
    );
    return (void*)addr;
}

char* get_args() {
    uint32_t addr;
    asm volatile (
        "int $0x80"
        : "=a"(addr)
        : "a"(15)
        : "memory"
    );
    return (char*)addr;
}

void split_path(char* path, char* parent_out, char* name_out) {
    int len = strlen(path);
    int last_slash = 0;
    for (int i = 0; i < len; i++)
        if (path[i] == '/') last_slash = i;

    int j = 0;
    for (int i = last_slash + 1; i < len; i++)
        name_out[j++] = path[i];
    name_out[j] = '\0';

    if (last_slash == 0) {
        parent_out[0] = '/';
        parent_out[1] = '\0';
    } else {
        for (int i = 0; i < last_slash; i++)
            parent_out[i] = path[i];
        parent_out[last_slash] = '\0';
    }
}

int dir_exists(char* path) {
    if (path[0] == '/' && path[1] == '\0') return 1;

    char parent[256];
    char name[32];
    split_path(path, parent, name);

    char buf[512];
    buf[0] = '\0';
    fs_ls(parent, buf);

    int i = 0;
    while (buf[i]) {
        int j = 0;
        while (name[j] && buf[i+j] && buf[i+j] != ',' && name[j] == buf[i+j]) j++;
        if (!name[j] && (!buf[i+j] || buf[i+j] == ',')) return 1;
        while (buf[i] && buf[i] != ',') i++;
        if (buf[i] == ',') i++;
    }
    return 0;
}

void fs_mkdir(char* path) {
    asm volatile (
        "int $0x80"
        :
        : "a"(8),
          "b"((uint32_t)path)
        : "memory"
    );
}

void fs_ls(char* path, char* buf) {
    asm volatile (
        "int $0x80"
        :
        : "a"(9),
          "b"((uint32_t)path),
          "c"((uint32_t)buf)
        : "memory"
    );
}

int fs_read(char* path, char* buf) {
    uint32_t ret;
    asm volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(10),
          "b"((uint32_t)path),
          "c"((uint32_t)buf)
        : "memory"
    );
    return (int)ret;
}

int fs_write(char* path, char* buf) {
    uint32_t ret;
    asm volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(11),
          "b"((uint32_t)path),
          "c"((uint32_t)buf)
        : "memory"
    );
    return (int)ret;
}

int fs_rm(char* path) {
    uint32_t ret;
    asm volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(16),
          "b"((uint32_t)path)
        : "memory"
    );
    return (int)ret;
}

int fs_copy(char* src, char* dst) {
    uint32_t ret;
    asm volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(20),
          "b"((uint32_t)src),
          "c"((uint32_t)dst)
        : "memory"
    );
    return (int)ret;
}

int fs_rename(char* src, char* dst) {
    uint32_t ret;
    asm volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(19),
          "b"((uint32_t)src),
          "c"((uint32_t)dst)
        : "memory"
    );
    return (int)ret;
}

int fs_rmdir(char* path) {
    uint32_t ret;
    asm volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(17),
          "b"((uint32_t)path)
        : "memory"
    );
    return (int)ret;
}

// TODO See if this works with new file system approach.
int file_read_at(char* filename, uint8_t* buf, uint32_t offset, uint32_t len) {
    uint32_t ret;
    asm volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(15),
          "b"((uint32_t)filename),
          "c"((uint32_t)buf),
          "d"(offset),
          "S"(len)
        : "memory"
    );
    return (int)ret;
}

void uint_to_ascii(uint32_t num, char* buffer) {
    if (num == 0) { buffer[0] = '0'; buffer[1] = '\0'; return; }
    uint8_t digits[10];
    for (int i = 0; i < 10; i++) {
        digits[i] = num % 10;
        num /= 10;
    }
    int j = 9;
    while (digits[j] == 0) {
        j--;
    }
    int pos = 0;
    while (j >= 0) {
        buffer[pos] = digits[j] + 48;
        j--;
        pos += 1;
    }
    buffer[pos] = 0;
}
