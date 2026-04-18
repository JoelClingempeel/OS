#include "fs.h"
#include "utils.h"

#define FS_DATA_SIZE  (SECTOR_SIZE - 4U)  // 508 usable bytes per sector

typedef struct {
    char     name[FS_MAX_FILENAME];
    uint32_t first_sector;
    uint32_t size;
    uint8_t  type;                    // FS_TYPE_FILE or FS_TYPE_DIR
} __attribute__((packed)) dir_entry_t;

// Structured payload of a directory's first sector, after its next_sector link.
typedef struct {
    uint32_t    magic;
    uint8_t     num_entries;          // total entries across all sectors in this directory chain
    uint8_t     reserved[3];
    dir_entry_t entries[FS_MAX_ENTRIES];
} __attribute__((packed)) dir_payload_t;

static uint32_t sectors_needed(uint32_t size) {
    if (size == 0) return 1;
    return (size + FS_DATA_SIZE - 1) / FS_DATA_SIZE;
}

// Walk all sectors reachable from the directory chain at dir_first_sec:
// the chain sectors themselves, all file chains they reference, and recursively
// all sub-directory chains. Updates *max_used with the highest sector number seen.
static void walk_all_sectors(uint32_t dir_first_sec, uint32_t* max_used) {
    uint8_t buf[SECTOR_SIZE];
    uint32_t dir_sec = dir_first_sec;
    int chain_pos = 0;
    int total_entries = 0;

    while (dir_sec != NO_NEXT_SECTOR) {
        if (dir_sec > *max_used) *max_used = dir_sec;
        disk_read(dir_sec, buf);
        uint32_t next_dir = *(uint32_t*)buf;

        dir_entry_t* entries;
        int n;
        if (chain_pos == 0) {
            dir_payload_t* dp = (dir_payload_t*)(buf + 4);
            total_entries = dp->num_entries;
            entries = dp->entries;
            n = total_entries < FS_MAX_ENTRIES ? total_entries : FS_MAX_ENTRIES;
        } else {
            entries = (dir_entry_t*)(buf + 4);
            int start = chain_pos * FS_MAX_ENTRIES;
            int rem = total_entries - start;
            n = rem < FS_MAX_ENTRIES ? rem : FS_MAX_ENTRIES;
        }

        // Copy entries before recursive calls overwrite buf.
        dir_entry_t local[FS_MAX_ENTRIES];
        for (int i = 0; i < n; i++) local[i] = entries[i];

        for (int i = 0; i < n; i++) {
            if (local[i].type == FS_TYPE_DIR) {
                walk_all_sectors(local[i].first_sector, max_used);
            } else {
                uint8_t fbuf[SECTOR_SIZE];
                uint32_t fsec = local[i].first_sector;
                while (fsec != NO_NEXT_SECTOR) {
                    if (fsec > *max_used) *max_used = fsec;
                    disk_read(fsec, fbuf);
                    fsec = *(uint32_t*)fbuf;
                }
            }
        }

        dir_sec = next_dir;
        chain_pos++;
    }
}

// Walk all sectors reachable from the root and return max_sector_used + 1.
static uint32_t find_free_sector(void) {
    uint8_t buf[SECTOR_SIZE];
    disk_read(0, buf);
    dir_payload_t* dp0 = (dir_payload_t*)(buf + 4);
    if (dp0->magic != FS_MAGIC) return 1;

    uint32_t max_used = 0;
    walk_all_sectors(0, &max_used);
    return max_used + 1;
}

// Rewrite the data of an existing file's sector chain, extending or truncating as needed.
static void update_file_chain(uint32_t first_sec, const uint8_t* data, uint32_t size) {
    uint8_t sec_buf[SECTOR_SIZE];
    uint32_t needed = sectors_needed(size);
    uint32_t sec = first_sec;
    uint32_t next_free = 0;
    int free_init = 0;

    for (uint32_t i = 0; i < needed; i++) {
        uint32_t current;

        if (sec != NO_NEXT_SECTOR) {
            current = sec;
            disk_read(sec, sec_buf);
            sec = *(uint32_t*)sec_buf;
        } else {
            if (!free_init) {
                next_free = find_free_sector();
                free_init = 1;
            }
            current = next_free++;
        }

        uint32_t next_link;
        if (i + 1 < needed) {
            if (sec != NO_NEXT_SECTOR) {
                next_link = sec;
            } else {
                if (!free_init) {
                    next_free = find_free_sector();
                    free_init = 1;
                }
                next_link = next_free;
            }
        } else {
            next_link = NO_NEXT_SECTOR;
        }

        uint32_t offset = i * FS_DATA_SIZE;
        uint32_t remaining = size - offset;
        uint32_t to_copy = remaining < FS_DATA_SIZE ? remaining : FS_DATA_SIZE;
        memset(sec_buf, 0, SECTOR_SIZE);
        *(uint32_t*)sec_buf = next_link;
        memcpy(sec_buf + 4, data + offset, to_copy);
        disk_write(current, sec_buf);
    }
}

int fs_read_in(uint32_t dir_first_sec, char* filename, uint8_t* buf) {
    uint8_t sec_buf[SECTOR_SIZE];

    disk_read(dir_first_sec, sec_buf);
    dir_payload_t* dp0 = (dir_payload_t*)(sec_buf + 4);
    if (dp0->magic != FS_MAGIC) return -1;

    int total_entries = dp0->num_entries;
    uint32_t first_sector = NO_NEXT_SECTOR;
    uint32_t size = 0;
    uint32_t dir_sec = dir_first_sec;
    int chain_pos = 0;

    while (dir_sec != NO_NEXT_SECTOR && first_sector == NO_NEXT_SECTOR) {
        disk_read(dir_sec, sec_buf);
        uint32_t next_dir = *(uint32_t*)sec_buf;

        dir_entry_t* entries;
        int n;
        if (chain_pos == 0) {
            entries = ((dir_payload_t*)(sec_buf + 4))->entries;
            n = total_entries < FS_MAX_ENTRIES ? total_entries : FS_MAX_ENTRIES;
        } else {
            entries = (dir_entry_t*)(sec_buf + 4);
            int start = chain_pos * FS_MAX_ENTRIES;
            int rem = total_entries - start;
            n = rem < FS_MAX_ENTRIES ? rem : FS_MAX_ENTRIES;
        }

        for (int i = 0; i < n; i++) {
            if (entries[i].type == FS_TYPE_FILE && strcmp(entries[i].name, filename) == 0) {
                first_sector = entries[i].first_sector;
                size = entries[i].size;
                break;
            }
        }

        dir_sec = next_dir;
        chain_pos++;
    }

    if (first_sector == NO_NEXT_SECTOR) return -1;

    uint32_t bytes_left = size;
    uint32_t sec = first_sector;
    while (sec != NO_NEXT_SECTOR && bytes_left > 0) {
        disk_read(sec, sec_buf);
        uint32_t next = *(uint32_t*)sec_buf;
        uint32_t to_copy = bytes_left < FS_DATA_SIZE ? bytes_left : FS_DATA_SIZE;
        memcpy(buf, sec_buf + 4, to_copy);
        buf += to_copy;
        bytes_left -= to_copy;
        sec = next;
    }

    return 0;
}

int fs_read(char* filename, uint8_t* buf) {
    return fs_read_in(0, filename, buf);
}

int fs_read_at(char* filename, uint8_t* buf, uint32_t offset, uint32_t len) {
    uint8_t sec_buf[SECTOR_SIZE];

    disk_read(0, sec_buf);
    dir_payload_t* dp0 = (dir_payload_t*)(sec_buf + 4);
    if (dp0->magic != FS_MAGIC) return -1;

    int total_entries = dp0->num_entries;
    uint32_t first_sector = NO_NEXT_SECTOR;
    uint32_t size = 0;
    uint32_t dir_sec = 0;
    int chain_pos = 0;

    while (dir_sec != NO_NEXT_SECTOR && first_sector == NO_NEXT_SECTOR) {
        disk_read(dir_sec, sec_buf);
        uint32_t next_dir = *(uint32_t*)sec_buf;

        dir_entry_t* entries;
        int n;
        if (chain_pos == 0) {
            entries = ((dir_payload_t*)(sec_buf + 4))->entries;
            n = total_entries < FS_MAX_ENTRIES ? total_entries : FS_MAX_ENTRIES;
        } else {
            entries = (dir_entry_t*)(sec_buf + 4);
            int start = chain_pos * FS_MAX_ENTRIES;
            int rem = total_entries - start;
            n = rem < FS_MAX_ENTRIES ? rem : FS_MAX_ENTRIES;
        }

        for (int i = 0; i < n; i++) {
            if (entries[i].type == FS_TYPE_FILE && strcmp(entries[i].name, filename) == 0) {
                first_sector = entries[i].first_sector;
                size = entries[i].size;
                break;
            }
        }

        dir_sec = next_dir;
        chain_pos++;
    }

    if (first_sector == NO_NEXT_SECTOR) return -1;
    if (offset >= size) return -2;

    uint32_t available = size - offset;
    if (len > available) len = available;

    uint32_t sec_skip = offset / FS_DATA_SIZE;
    uint32_t sec_offset = offset % FS_DATA_SIZE;
    uint32_t sec = first_sector;
    for (uint32_t i = 0; i < sec_skip && sec != NO_NEXT_SECTOR; i++) {
        disk_read(sec, sec_buf);
        sec = *(uint32_t*)sec_buf;
    }

    uint32_t bytes_left = len;
    int first = 1;
    while (sec != NO_NEXT_SECTOR && bytes_left > 0) {
        disk_read(sec, sec_buf);
        uint32_t next = *(uint32_t*)sec_buf;
        uint8_t* data = sec_buf + 4;
        uint32_t start = first ? sec_offset : 0;
        uint32_t available_in_sec = FS_DATA_SIZE - start;
        uint32_t to_copy = bytes_left < available_in_sec ? bytes_left : available_in_sec;
        memcpy(buf, data + start, to_copy);
        buf += to_copy;
        bytes_left -= to_copy;
        sec = next;
        first = 0;
    }

    return 0;
}

int fs_write_in(uint32_t dir_first_sec, char* filename, uint8_t* buf, uint32_t size) {
    uint8_t sec_buf[SECTOR_SIZE];

    disk_read(dir_first_sec, sec_buf);
    dir_payload_t* dp0 = (dir_payload_t*)(sec_buf + 4);
    int total_entries = (dp0->magic == FS_MAGIC) ? (int)dp0->num_entries : 0;

    uint32_t dir_sec = (dp0->magic == FS_MAGIC) ? dir_first_sec : NO_NEXT_SECTOR;
    int chain_pos = 0;
    while (dir_sec != NO_NEXT_SECTOR) {
        disk_read(dir_sec, sec_buf);
        uint32_t next_dir = *(uint32_t*)sec_buf;

        dir_entry_t* entries;
        int n;
        if (chain_pos == 0) {
            entries = ((dir_payload_t*)(sec_buf + 4))->entries;
            n = total_entries < FS_MAX_ENTRIES ? total_entries : FS_MAX_ENTRIES;
        } else {
            entries = (dir_entry_t*)(sec_buf + 4);
            int start = chain_pos * FS_MAX_ENTRIES;
            int rem = total_entries - start;
            n = rem < FS_MAX_ENTRIES ? rem : FS_MAX_ENTRIES;
        }

        for (int i = 0; i < n; i++) {
            if (entries[i].type == FS_TYPE_FILE && strcmp(entries[i].name, filename) == 0) {
                uint32_t first_sec = entries[i].first_sector;
                entries[i].size = size;
                disk_write(dir_sec, sec_buf);
                update_file_chain(first_sec, buf, size);
                return 0;
            }
        }

        dir_sec = next_dir;
        chain_pos++;
    }

    // No existing entry — create a new one.
    int target_chain_pos = total_entries / FS_MAX_ENTRIES;
    int entry_slot = total_entries % FS_MAX_ENTRIES;
    uint32_t next_free = find_free_sector();

    // Walk to target_chain_pos in this directory chain, allocating new dir sectors as needed.
    dir_sec = dir_first_sec;
    chain_pos = 0;
    while (chain_pos < target_chain_pos) {
        disk_read(dir_sec, sec_buf);
        uint32_t next_dir = *(uint32_t*)sec_buf;
        if (next_dir == NO_NEXT_SECTOR) {
            uint32_t new_dir_sec = next_free++;
            *(uint32_t*)sec_buf = new_dir_sec;
            disk_write(dir_sec, sec_buf);
            memset(sec_buf, 0, SECTOR_SIZE);
            *(uint32_t*)sec_buf = NO_NEXT_SECTOR;
            disk_write(new_dir_sec, sec_buf);
            dir_sec = new_dir_sec;
        } else {
            dir_sec = next_dir;
        }
        chain_pos++;
    }

    // Load the target directory sector and insert the new entry.
    disk_read(dir_sec, sec_buf);
    dir_entry_t* entry;
    if (target_chain_pos == 0) {
        dir_payload_t* dp = (dir_payload_t*)(sec_buf + 4);
        if (dp->magic != FS_MAGIC) {
            *(uint32_t*)sec_buf = NO_NEXT_SECTOR;
            dp->magic = FS_MAGIC;
            dp->num_entries = 0;
        }
        dp->num_entries++;
        entry = &dp->entries[entry_slot];
    } else {
        // num_entries lives in dir_first_sec; update it separately.
        uint8_t s0[SECTOR_SIZE];
        disk_read(dir_first_sec, s0);
        ((dir_payload_t*)(s0 + 4))->num_entries++;
        disk_write(dir_first_sec, s0);
        entry = &((dir_entry_t*)(sec_buf + 4))[entry_slot];
    }

    memcpy(entry->name, filename, FS_MAX_FILENAME);
    entry->name[FS_MAX_FILENAME - 1] = '\0';
    entry->first_sector = next_free;
    entry->size = size;
    entry->type = FS_TYPE_FILE;
    disk_write(dir_sec, sec_buf);

    // Write file data sectors contiguously from next_free.
    uint32_t needed = sectors_needed(size);
    for (uint32_t i = 0; i < needed; i++) {
        uint32_t this_sec = next_free++;
        uint32_t next_link = (i + 1 < needed) ? next_free : NO_NEXT_SECTOR;
        memset(sec_buf, 0, SECTOR_SIZE);
        *(uint32_t*)sec_buf = next_link;
        uint32_t offset = i * FS_DATA_SIZE;
        uint32_t remaining = size - offset;
        uint32_t to_copy = remaining < FS_DATA_SIZE ? remaining : FS_DATA_SIZE;
        memcpy(sec_buf + 4, buf + offset, to_copy);
        disk_write(this_sec, sec_buf);
    }

    return 0;
}

int fs_write(char* filename, uint8_t* buf, uint32_t size) {
    return fs_write_in(0, filename, buf, size);
}

int fs_list_in(uint32_t dir_first_sec, char names[][FS_MAX_FILENAME], int max_names) {
    uint8_t sec_buf[SECTOR_SIZE];

    disk_read(dir_first_sec, sec_buf);
    dir_payload_t* dp0 = (dir_payload_t*)(sec_buf + 4);
    if (dp0->magic != FS_MAGIC) return 0;

    int total_entries = dp0->num_entries;
    int count = 0;
    uint32_t dir_sec = dir_first_sec;
    int chain_pos = 0;

    while (dir_sec != NO_NEXT_SECTOR && count < max_names) {
        disk_read(dir_sec, sec_buf);
        uint32_t next_dir = *(uint32_t*)sec_buf;

        dir_entry_t* entries;
        int n;
        if (chain_pos == 0) {
            entries = ((dir_payload_t*)(sec_buf + 4))->entries;
            n = total_entries < FS_MAX_ENTRIES ? total_entries : FS_MAX_ENTRIES;
        } else {
            entries = (dir_entry_t*)(sec_buf + 4);
            int start = chain_pos * FS_MAX_ENTRIES;
            int rem = total_entries - start;
            n = rem < FS_MAX_ENTRIES ? rem : FS_MAX_ENTRIES;
        }

        for (int i = 0; i < n && count < max_names; i++) {
            if (entries[i].type == FS_TYPE_FILE)
                memcpy(names[count++], entries[i].name, FS_MAX_FILENAME);
        }

        dir_sec = next_dir;
        chain_pos++;
    }

    return count;
}

int fs_list(char names[][FS_MAX_FILENAME], int max_names) {
    return fs_list_in(0, names, max_names);
}

uint32_t fs_find_dir(char* name) {
    uint8_t sec_buf[SECTOR_SIZE];

    disk_read(0, sec_buf);
    dir_payload_t* dp0 = (dir_payload_t*)(sec_buf + 4);
    if (dp0->magic != FS_MAGIC) return NO_NEXT_SECTOR;

    int total_entries = dp0->num_entries;
    uint32_t dir_sec = 0;
    int chain_pos = 0;

    while (dir_sec != NO_NEXT_SECTOR) {
        disk_read(dir_sec, sec_buf);
        uint32_t next_dir = *(uint32_t*)sec_buf;

        dir_entry_t* entries;
        int n;
        if (chain_pos == 0) {
            entries = ((dir_payload_t*)(sec_buf + 4))->entries;
            n = total_entries < FS_MAX_ENTRIES ? total_entries : FS_MAX_ENTRIES;
        } else {
            entries = (dir_entry_t*)(sec_buf + 4);
            int start = chain_pos * FS_MAX_ENTRIES;
            int rem = total_entries - start;
            n = rem < FS_MAX_ENTRIES ? rem : FS_MAX_ENTRIES;
        }

        for (int i = 0; i < n; i++) {
            if (entries[i].type == FS_TYPE_DIR && strcmp(entries[i].name, name) == 0)
                return entries[i].first_sector;
        }

        dir_sec = next_dir;
        chain_pos++;
    }

    return NO_NEXT_SECTOR;
}

uint32_t fs_mkdir(char* name) {
    uint8_t sec_buf[SECTOR_SIZE];

    disk_read(0, sec_buf);
    dir_payload_t* dp0 = (dir_payload_t*)(sec_buf + 4);
    int total_entries = (dp0->magic == FS_MAGIC) ? (int)dp0->num_entries : 0;

    // Check for name conflict anywhere in the root directory chain.
    uint32_t dir_sec = (dp0->magic == FS_MAGIC) ? 0 : NO_NEXT_SECTOR;
    int chain_pos = 0;
    while (dir_sec != NO_NEXT_SECTOR) {
        disk_read(dir_sec, sec_buf);
        uint32_t next_dir = *(uint32_t*)sec_buf;

        dir_entry_t* entries;
        int n;
        if (chain_pos == 0) {
            entries = ((dir_payload_t*)(sec_buf + 4))->entries;
            n = total_entries < FS_MAX_ENTRIES ? total_entries : FS_MAX_ENTRIES;
        } else {
            entries = (dir_entry_t*)(sec_buf + 4);
            int start = chain_pos * FS_MAX_ENTRIES;
            int rem = total_entries - start;
            n = rem < FS_MAX_ENTRIES ? rem : FS_MAX_ENTRIES;
        }

        for (int i = 0; i < n; i++) {
            if (strcmp(entries[i].name, name) == 0) return NO_NEXT_SECTOR;
        }

        dir_sec = next_dir;
        chain_pos++;
    }

    int target_chain_pos = total_entries / FS_MAX_ENTRIES;
    int entry_slot = total_entries % FS_MAX_ENTRIES;
    uint32_t next_free = find_free_sector();

    // Walk to target_chain_pos in root, allocating new dir sectors as needed.
    dir_sec = 0;
    chain_pos = 0;
    while (chain_pos < target_chain_pos) {
        disk_read(dir_sec, sec_buf);
        uint32_t next_dir = *(uint32_t*)sec_buf;
        if (next_dir == NO_NEXT_SECTOR) {
            uint32_t new_dir_sec = next_free++;
            *(uint32_t*)sec_buf = new_dir_sec;
            disk_write(dir_sec, sec_buf);
            memset(sec_buf, 0, SECTOR_SIZE);
            *(uint32_t*)sec_buf = NO_NEXT_SECTOR;
            disk_write(new_dir_sec, sec_buf);
            dir_sec = new_dir_sec;
        } else {
            dir_sec = next_dir;
        }
        chain_pos++;
    }

    // Insert the new directory entry into the target root sector.
    disk_read(dir_sec, sec_buf);
    dir_entry_t* entry;
    if (target_chain_pos == 0) {
        dir_payload_t* dp = (dir_payload_t*)(sec_buf + 4);
        if (dp->magic != FS_MAGIC) {
            *(uint32_t*)sec_buf = NO_NEXT_SECTOR;
            dp->magic = FS_MAGIC;
            dp->num_entries = 0;
        }
        dp->num_entries++;
        entry = &dp->entries[entry_slot];
    } else {
        uint8_t s0[SECTOR_SIZE];
        disk_read(0, s0);
        ((dir_payload_t*)(s0 + 4))->num_entries++;
        disk_write(0, s0);
        entry = &((dir_entry_t*)(sec_buf + 4))[entry_slot];
    }

    uint32_t new_dir_first_sec = next_free;
    memcpy(entry->name, name, FS_MAX_FILENAME);
    entry->name[FS_MAX_FILENAME - 1] = '\0';
    entry->first_sector = new_dir_first_sec;
    entry->size = 0;
    entry->type = FS_TYPE_DIR;
    disk_write(dir_sec, sec_buf);

    // Initialize the new directory's first sector.
    memset(sec_buf, 0, SECTOR_SIZE);
    *(uint32_t*)sec_buf = NO_NEXT_SECTOR;
    dir_payload_t* new_dp = (dir_payload_t*)(sec_buf + 4);
    new_dp->magic = FS_MAGIC;
    new_dp->num_entries = 0;
    disk_write(new_dir_first_sec, sec_buf);

    return new_dir_first_sec;
}
