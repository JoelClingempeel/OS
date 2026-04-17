#ifndef FS_H
#define FS_H

#include <stdint.h>

#include "disk.h"

// Every sector begins with a 4-byte next_sector link (NO_NEXT_SECTOR = 0xFFFFFFFF marks the end
// of a chain). The remaining FS_DATA_SIZE = 508 bytes are payload.
//
// Sector 0 payload layout (508 bytes):
//   Bytes 0-3:   Magic number 0x4F534653 ("OSFS")
//   Byte  4:     Total number of valid directory entries across all directory sectors (0-255)
//   Bytes 5-7:   Reserved (zero)
//   Bytes 8-487: Up to 15 directory entries, each 32 bytes:
//     Bytes  0-23: Filename, null-terminated (max 23 chars + null)
//     Bytes 24-27: First sector of the file's data chain (uint32_t)
//     Bytes 28-31: File size in bytes (uint32_t)
//   Bytes 488-507: Unused
//
// When the directory holds more than 15 entries it chains to further directory sectors.
// Each subsequent directory sector payload is packed dir_entry_t records (15 per sector).
// num_entries is stored only in sector 0 and counts entries across the entire directory chain.
//
// File data is stored as a chain of sectors. Each sector's 508-byte payload is raw file bytes.

#define FS_MAGIC           0x4F534653
#define FS_MAX_ENTRIES     15          // directory entries per sector
#define FS_MAX_FILENAME    24
#define NO_NEXT_SECTOR     0xFFFFFFFFU

// Read a file into buf (must be large enough for the file's entire content).
// Returns 0 on success, -1 if not found.
int fs_read(char* filename, uint8_t* buf);

// Read len bytes starting at byte offset within the file into buf.
// Returns 0 on success, -1 if not found, -2 if offset is past end of file.
int fs_read_at(char* filename, uint8_t* buf, uint32_t offset, uint32_t len);

// Write buf to a file, creating it if it doesn't exist.
// Returns 0 on success, -1 if the directory is full.
int fs_write(char* filename, uint8_t* buf, uint32_t size);

// Fill names with the filenames of all existing files.
// Returns the number of entries written, up to max_names.
int fs_list(char names[][FS_MAX_FILENAME], int max_names);

#endif  // FS_H
