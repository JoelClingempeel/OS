#include "files.h"
#include "utils.h"

#define DIR_BUF_SIZE 1024

static void uint_to_str(uint32_t n, char* out) {
    if (n == 0) { out[0] = '0'; out[1] = '\0'; return; }
    char tmp[12];
    int i = 0;
    while (n > 0) { tmp[i++] = '0' + (n % 10); n /= 10; }
    int j = 0;
    while (i > 0) out[j++] = tmp[--i];
    out[j] = '\0';
}

static uint32_t str_to_uint(char* s) {
    uint32_t n = 0;
    while (*s >= '0' && *s <= '9') n = n * 10 + (*s++ - '0');
    return n;
}

static int str_eq(const char* a, const char* b) {
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}


void read_file(uint32_t sector_num, char* buffer) {
    uint8_t sector[SECTOR_SIZE];
    disk_read(sector_num, sector);

    uint32_t next_sector = *(uint32_t*)sector;
    for (int i = 4; i < SECTOR_SIZE; i++)
        *buffer++ = sector[i];

    if (next_sector != 0)
        read_file(next_sector, buffer);
}

// TODO Get a better bookkeeping mechanism.
static uint32_t find_empty_sector(uint32_t start) {
    uint8_t sector[SECTOR_SIZE];
    for (uint32_t i = start; ; i++) {
        disk_read(i, sector);
        int empty = 1;
        for (int j = 0; j < SECTOR_SIZE; j++) {
            if (sector[j] != 0) { empty = 0; break; }
        }
        if (empty) return i;
    }
}

void write_file(uint32_t sector_num, char* buffer) {
    uint8_t sector[SECTOR_SIZE];
    for (int i = 0; i < SECTOR_SIZE; i++) sector[i] = 0;

    int full = 0;
    for (int i = 4; i < SECTOR_SIZE; i++) {
        char c = *buffer++;
        sector[i] = c;
        if (c == '\0') break;
        if (i == SECTOR_SIZE - 1) full = 1;
    }

    if (full) {
        uint32_t next = find_empty_sector(sector_num + 1);
        *(uint32_t*)sector = next;
        disk_write(sector_num, sector);
        write_file(next, buffer);
    } else {
        disk_write(sector_num, sector);
    }
}

// Look up `name` in directory at `dir_sector`.
// Sets *is_dir. Returns the child's sector, or 0 if not found.
static uint32_t lookup_in_dir(uint32_t dir_sector, char* name, int* is_dir) {
    char buf[DIR_BUF_SIZE];
    for (int i = 0; i < DIR_BUF_SIZE; i++) buf[i] = 0;
    read_file(dir_sector, buf);

    char* p = buf;
    while (*p) {
        char entry_name[MAX_NAME];
        int ni = 0;
        while (*p && *p != ',' && *p != '\n') {
            if (ni < MAX_NAME - 1) entry_name[ni++] = *p;
            p++;
        }
        entry_name[ni] = '\0';

        if (*p != ',') {
            while (*p && *p != '\n') p++;
            if (*p) p++;
            continue;
        }
        p++; // skip ','

        int dir_flag = 0;
        if (*p == 'd') { dir_flag = 1; p++; }
        uint32_t sector = str_to_uint(p);
        while (*p && *p != '\n') p++;
        if (*p) p++;

        if (str_eq(entry_name, name)) {
            *is_dir = dir_flag;
            return sector;
        }
    }
    return 0;
}

// Resolve a path to its sector number. Returns 0 for "/", 0xFFFFFFFF if not found.
static uint32_t lookup_path(char* path) {
    if (path[0] == '/' && path[1] == '\0') return 0;

    uint32_t current = 0;
    char* p = path;
    if (*p == '/') p++;

    while (*p) {
        char component[MAX_NAME];
        int ci = 0;
        while (*p && *p != '/') {
            if (ci < MAX_NAME - 1) component[ci++] = *p;
            p++;
        }
        component[ci] = '\0';
        if (*p == '/') p++;

        int is_dir = 0;
        uint32_t next = lookup_in_dir(current, component, &is_dir);
        if (next == 0) return 0xFFFFFFFF;
        current = next;
    }
    return current;
}

int read_path(char* path, char* buffer) {
    uint32_t sector = lookup_path(path);
    if (sector == 0xFFFFFFFF) return -1;
    read_file(sector, buffer);
    return 0;
}

int write_path(char* path, char* buffer) {
    uint32_t sector = lookup_path(path);
    if (sector == 0xFFFFFFFF) return -1;
    write_file(sector, buffer);
    return 0;
}

void make_file(char* path, int is_dir) {
    // Split path into parent path and new directory name.
    int len = 0;
    while (path[len]) len++;

    int last_slash = 0;
    for (int i = len - 1; i >= 0; i--) {
        if (path[i] == '/') { last_slash = i; break; }
    }

    char dir_name[MAX_NAME];
    int ni = 0;
    for (int i = last_slash + 1; i < len; i++)
        dir_name[ni++] = path[i];
    dir_name[ni] = '\0';

    uint32_t parent_sector;
    if (last_slash == 0) {
        parent_sector = 0; // parent is root
    } else {
        char parent_path[256];
        for (int i = 0; i < last_slash; i++) parent_path[i] = path[i];
        parent_path[last_slash] = '\0';
        parent_sector = lookup_path(parent_path);
        if (parent_sector == 0xFFFFFFFF) return;
    }

    int existing_is_dir = 0;
    if (lookup_in_dir(parent_sector, dir_name, &existing_is_dir) != 0) return;

    uint32_t new_sector = find_empty_sector(1);

    // Mark the new sector as allocated so find_empty_sector won't reuse it.
    // An empty directory is a single newline; the parser skips blank lines.
    if (is_dir) {
        char marker[] = "\n";
        write_file(new_sector, marker);
    }

    // Read parent directory contents and append the new entry.
    char buf[DIR_BUF_SIZE];
    for (int i = 0; i < DIR_BUF_SIZE; i++) buf[i] = 0;
    read_file(parent_sector, buf);

    int end = 0;
    while (buf[end]) end++;

    for (int i = 0; dir_name[i]; i++) buf[end++] = dir_name[i];
    buf[end++] = ',';
    if (is_dir) buf[end++] = 'd';
    char num_str[12];
    uint_to_str(new_sector, num_str);
    for (int i = 0; num_str[i]; i++) buf[end++] = num_str[i];
    buf[end++] = '\n';
    buf[end] = '\0';

    write_file(parent_sector, buf);
}

int lsdir(char* path, char names[][MAX_NAME], int max_names) {
    uint32_t dir_sector = lookup_path(path);
    if (dir_sector == 0xFFFFFFFF) return -1;

    char buf[DIR_BUF_SIZE];
    for (int i = 0; i < DIR_BUF_SIZE; i++) buf[i] = 0;
    read_file(dir_sector, buf);

    int count = 0;
    char* p = buf;
    while (*p && count < max_names) {
        int ni = 0;
        while (*p && *p != ',' && *p != '\n') {
            if (ni < MAX_NAME - 1) names[count][ni++] = *p;
            p++;
        }
        names[count][ni] = '\0';

        if (*p != ',') {
            while (*p && *p != '\n') p++;
            if (*p) p++;
            continue;
        }
        while (*p && *p != '\n') p++;
        if (*p) p++;
        count++;
    }
    return count;
}
