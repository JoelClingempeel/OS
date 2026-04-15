#ifndef DISK_H
#define DISK_H

#include <stdint.h>

#define SECTOR_SIZE 512

// Read one sector at the given LBA index into buf (must be at least 512 bytes).
void disk_read(uint32_t lba, uint8_t* buf);

// Write one sector from buf (must be at least 512 bytes) to the given LBA index.
void disk_write(uint32_t lba, const uint8_t* buf);

#endif  // DISK_H
