#include "files.h"
#include "utils.h"

#define DIR_BUF_SIZE 508
#define BITMAP_SECTOR 0
#define ROOT_SECTOR   1

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
    uint32_t current = sector_num;
    do {
        disk_read(current, sector);
        uint32_t next = *(uint32_t*)sector;
        for (int i = 4; i < SECTOR_SIZE; i++)
            *buffer++ = sector[i];
        current = next;
    } while (current != 0);
}

static void bitmap_set_sector(uint32_t sector_num) {
    uint8_t bitmap[SECTOR_SIZE];
    disk_read(BITMAP_SECTOR, bitmap);
    bitmap[sector_num / 8] |= (1 << (sector_num % 8));
    disk_write(BITMAP_SECTOR, bitmap);
}

void fs_init(void) {
    uint8_t bitmap[SECTOR_SIZE];
    for (int i = 0; i < SECTOR_SIZE; i++) bitmap[i] = 0;
    disk_write(BITMAP_SECTOR, bitmap);
    bitmap_set_sector(BITMAP_SECTOR);
    bitmap_set_sector(ROOT_SECTOR);
}

static uint32_t find_empty_sector(uint32_t start) {
    uint8_t bitmap[SECTOR_SIZE];
    disk_read(BITMAP_SECTOR, bitmap);
    for (uint32_t i = start; i < SECTOR_SIZE * 8; i++) {
        if (!(bitmap[i / 8] & (1 << (i % 8)))) return i;
    }
    return 0xFFFFFFFF;
}

void write_file(uint32_t sector_num, char* buffer) {
    uint8_t sector[SECTOR_SIZE];
    uint32_t current = sector_num;
    while (1) {
        for (int i = 0; i < SECTOR_SIZE; i++) sector[i] = 0;
        int full = 0;
        for (int i = 4; i < SECTOR_SIZE; i++) {
            char c = *buffer++;
            sector[i] = c;
            if (c == '\0') break;
            if (i == SECTOR_SIZE - 1) full = 1;
        }
        if (full) {
            uint32_t next = find_empty_sector(current + 1);
            bitmap_set_sector(next);
            *(uint32_t*)sector = next;
            disk_write(current, sector);
            current = next;
        } else {
            disk_write(current, sector);
            break;
        }
    }
}

// Look up `name` in directory at `dir_sector`.
// Sets *is_dir. Returns the child's sector, or 0 if not found.
static uint32_t lookup_in_dir(uint32_t dir_sector, char* name, int* is_dir) {
    SERIAL_PRINT("[files] lookup_in_dir: dir_sector=");
    serial_print_uint(dir_sector);
    SERIAL_PRINT(" name=");
    serial_print(name);
    SERIAL_PRINT("\n");

    char buf[DIR_BUF_SIZE];
    for (int i = 0; i < DIR_BUF_SIZE; i++) buf[i] = 0;
    read_file(dir_sector, buf);

    SERIAL_PRINT("[files]   dir contents: \"");
    serial_print(buf);
    SERIAL_PRINT("\"\n");

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

        SERIAL_PRINT("[files]   entry: name=\"");
        serial_print(entry_name);
        SERIAL_PRINT("\" is_dir=");
        serial_print_uint(dir_flag);
        SERIAL_PRINT(" sector=");
        serial_print_uint(sector);
        SERIAL_PRINT("\n");

        if (str_eq(entry_name, name)) {
            *is_dir = dir_flag;
            SERIAL_PRINT("[files]   FOUND sector=");
            serial_print_uint(sector);
            SERIAL_PRINT("\n");
            return sector;
        }
    }
    SERIAL_PRINT("[files]   NOT FOUND\n");
    return 0;
}

// Resolve a path to its sector number. Returns 0 for "/", 0xFFFFFFFF if not found.
static uint32_t lookup_path(char* path) {
    SERIAL_PRINT("[files] lookup_path: \"");
    serial_print(path);
    SERIAL_PRINT("\"\n");

    if (path[0] == '/' && path[1] == '\0') {
        SERIAL_PRINT("[files]   -> root (sector 1)\n");
        return ROOT_SECTOR;
    }

    uint32_t current = ROOT_SECTOR;
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
        if (next == 0) {
            SERIAL_PRINT("[files]   component \"");
            serial_print(component);
            SERIAL_PRINT("\" not found in sector ");
            serial_print_uint(current);
            SERIAL_PRINT(" -> NOT FOUND\n");
            return 0xFFFFFFFF;
        }
        SERIAL_PRINT("[files]   component \"");
        serial_print(component);
        SERIAL_PRINT("\" -> sector ");
        serial_print_uint(next);
        SERIAL_PRINT("\n");
        current = next;
    }
    SERIAL_PRINT("[files]   -> sector ");
    serial_print_uint(current);
    SERIAL_PRINT("\n");
    return current;
}

int read_path(char* path, char* buffer) {
    SERIAL_PRINT("[files] read_path: \"");
    serial_print(path);
    SERIAL_PRINT("\"\n");
    uint32_t sector = lookup_path(path);
    if (sector == 0xFFFFFFFF) {
        SERIAL_PRINT("[files] read_path: NOT FOUND\n");
        return -1;
    }
    read_file(sector, buffer);
    SERIAL_PRINT("[files] read_path: sector=");
    serial_print_uint(sector);
    SERIAL_PRINT(" content=\"");
    serial_print(buffer);
    SERIAL_PRINT("\"\n");
    return 0;
}

int write_path(char* path, char* buffer) {
    SERIAL_PRINT("[files] write_path: \"");
    serial_print(path);
    SERIAL_PRINT("\" content=\"");
    serial_print(buffer);
    SERIAL_PRINT("\"\n");
    uint32_t sector = lookup_path(path);
    if (sector == 0xFFFFFFFF) {
        SERIAL_PRINT("[files] write_path: NOT FOUND\n");
        return -1;
    }
    SERIAL_PRINT("[files] write_path: writing to sector ");
    serial_print_uint(sector);
    SERIAL_PRINT("\n");
    write_file(sector, buffer);
    return 0;
}

void make_file(char* path, int is_dir) {
    SERIAL_PRINT("[files] make_file: \"");
    serial_print(path);
    SERIAL_PRINT("\" is_dir=");
    serial_print_uint(is_dir);
    SERIAL_PRINT("\n");

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

    SERIAL_PRINT("[files]   name=\"");
    serial_print(dir_name);
    SERIAL_PRINT("\"\n");

    uint32_t parent_sector;
    if (last_slash == 0) {
        parent_sector = ROOT_SECTOR;
        SERIAL_PRINT("[files]   parent=root (sector 1)\n");
    } else {
        char parent_path[256];
        for (int i = 0; i < last_slash; i++) parent_path[i] = path[i];
        parent_path[last_slash] = '\0';
        SERIAL_PRINT("[files]   looking up parent: \"");
        serial_print(parent_path);
        SERIAL_PRINT("\"\n");
        parent_sector = lookup_path(parent_path);
        if (parent_sector == 0xFFFFFFFF) {
            SERIAL_PRINT("[files]   parent not found, aborting\n");
            return;
        }
        SERIAL_PRINT("[files]   parent sector=");
        serial_print_uint(parent_sector);
        SERIAL_PRINT("\n");
    }

    int existing_is_dir = 0;
    if (lookup_in_dir(parent_sector, dir_name, &existing_is_dir) != 0) {
        SERIAL_PRINT("[files]   already exists, skipping\n");
        return;
    }

    uint32_t new_sector = find_empty_sector(2);
    bitmap_set_sector(new_sector);
    SERIAL_PRINT("[files]   new_sector=");
    serial_print_uint(new_sector);
    SERIAL_PRINT("\n");

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

    SERIAL_PRINT("[files]   writing parent dir: \"");
    serial_print(buf);
    SERIAL_PRINT("\"\n");
    write_file(parent_sector, buf);
    SERIAL_PRINT("[files]   make_file done\n");
}

int lsdir(char* path, char names[][MAX_NAME], int max_names) {
    SERIAL_PRINT("[files] lsdir: \"");
    serial_print(path);
    SERIAL_PRINT("\"\n");
    uint32_t dir_sector = lookup_path(path);
    if (dir_sector == 0xFFFFFFFF) {
        SERIAL_PRINT("[files] lsdir: NOT FOUND\n");
        return -1;
    }

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
    SERIAL_PRINT("[files] lsdir: count=");
    serial_print_uint(count);
    SERIAL_PRINT("\n");
    return count;
}
