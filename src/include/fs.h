#ifndef FS_H
#define FS_H

#include <stdint.h>

#include "disk.h"

// Sector 0 layout (512 bytes):
//
//   Bytes 0-3:   Magic number 0x4F534653 ("OSFS")
//   Byte  4:     Number of valid entries (0-15)
//   Bytes 5-7:   Reserved (zero)
//   Bytes 8-487: Up to 15 directory entries, each 32 bytes:
//     Bytes  0-23: Filename, null-terminated (max 23 chars + null)
//     Bytes 24-27: Sector index (uint32_t)
//     Bytes 28-31: File size in bytes (uint32_t)
//   Bytes 488-511: Unused
//
// Each file occupies exactly one sector, capping file size at 512 bytes.

#define FS_MAGIC        0x4F534653
#define FS_MAX_ENTRIES  15
#define FS_MAX_FILENAME 24

// Read a file into buf (must be at least 512 bytes). Returns 0 on success, -1 if not found.
int fs_read(char* filename, uint8_t* buf);

// Write buf to a file, creating it if it doesn't exist. size must be <= 512.
// Returns 0 on success, -1 if the directory is full.
int fs_write(char* filename, uint8_t* buf, uint32_t size);

// Fill names with the filenames of all existing files.
// Returns the number of entries written, up to max_names.
int fs_list(char names[][FS_MAX_FILENAME], int max_names);

#endif  // FS_H
