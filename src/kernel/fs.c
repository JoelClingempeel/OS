#include "fs.h"
#include "utils.h"

typedef struct {
    char     name[FS_MAX_FILENAME];
    uint32_t sector;
    uint32_t size;
} __attribute__((packed)) dir_entry_t;

typedef struct {
    uint32_t    magic;
    uint8_t     num_entries;
    uint8_t     reserved[3];
    dir_entry_t entries[FS_MAX_ENTRIES];
} __attribute__((packed)) dir_sector_t;

int fs_read(char* filename, uint8_t* buf) {
    uint8_t sector_buf[SECTOR_SIZE];
    disk_read(0, sector_buf);
    dir_sector_t* dir = (dir_sector_t*)sector_buf;

    for (int i = 0; i < dir->num_entries; i++) {
        if (strcmp(dir->entries[i].name, filename) == 0) {
            disk_read(dir->entries[i].sector, buf);
            return 0;
        }
    }
    return -1;
}

int fs_write(char* filename, uint8_t* buf, uint32_t size) {
    uint8_t sector_buf[SECTOR_SIZE];
    disk_read(0, sector_buf);
    dir_sector_t* dir = (dir_sector_t*)sector_buf;

    // Search for an existing entry to overwrite.
    for (int i = 0; i < dir->num_entries; i++) {
        if (strcmp(dir->entries[i].name, filename) == 0) {
            dir->entries[i].size = size;
            disk_write(dir->entries[i].sector, buf);
            disk_write(0, sector_buf);
            return 0;
        }
    }

    // No existing entry — create a new one.
    if (dir->num_entries >= FS_MAX_ENTRIES) {
        return -1;
    }

    // Place the new file in the sector after the last one, starting at sector 1.
    uint32_t new_sector = 1;
    for (int i = 0; i < dir->num_entries; i++) {
        if (dir->entries[i].sector >= new_sector) {
            new_sector = dir->entries[i].sector + 1;
        }
    }

    dir_entry_t* entry = &dir->entries[dir->num_entries];
    memcpy(entry->name, filename, FS_MAX_FILENAME);
    entry->name[FS_MAX_FILENAME - 1] = '\0';
    entry->sector = new_sector;
    entry->size = size;
    dir->num_entries++;
    if (dir->magic != FS_MAGIC) {
        dir->magic = FS_MAGIC;
    }

    disk_write(new_sector, buf);
    disk_write(0, sector_buf);
    return 0;
}

int fs_list(char names[][FS_MAX_FILENAME], int max_names) {
    uint8_t sector_buf[SECTOR_SIZE];
    disk_read(0, sector_buf);
    dir_sector_t* dir = (dir_sector_t*)sector_buf;

    int count = dir->num_entries < max_names ? dir->num_entries : max_names;
    for (int i = 0; i < count; i++) {
        memcpy(names[i], dir->entries[i].name, FS_MAX_FILENAME);
    }
    return count;
}
