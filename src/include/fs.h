#ifndef FS_H
#define FS_H

#include <stdint.h>

#include "disk.h"

// Every sector begins with a 4-byte next_sector link (NO_NEXT_SECTOR = 0xFFFFFFFF marks the end
// of a chain). The remaining FS_DATA_SIZE = 508 bytes are payload.
//
// Directory first-sector payload layout (508 bytes):
//   Bytes 0-3:   Magic number 0x4F534653 ("OSFS")
//   Byte  4:     Total number of valid directory entries across all directory sectors (0-255)
//   Bytes 5-7:   Reserved (zero)
//   Bytes 8-502: Up to 15 directory entries, each 33 bytes:
//     Bytes  0-23: Filename, null-terminated (max 23 chars + null)
//     Bytes 24-27: First sector of the entry's chain (uint32_t)
//     Bytes 28-31: Size in bytes; 0 for directories (uint32_t)
//     Byte  32:   Entry type: FS_TYPE_FILE (0) or FS_TYPE_DIR (1)
//   Bytes 503-507: Unused
//
// When the directory holds more than 15 entries it chains to further directory sectors.
// Each subsequent directory sector payload is packed dir_entry_t records (15 per sector).
// num_entries is stored only in the directory's first sector and counts entries across
// the entire directory chain.
//
// The root directory always starts at sector 0.
// Sub-directories created via fs_mkdir have the same layout, starting at their first_sector.
//
// File data is stored as a chain of sectors. Each sector's 508-byte payload is raw file bytes.

#define FS_MAGIC           0x4F534653
#define FS_MAX_ENTRIES     15          // directory entries per sector
#define FS_MAX_FILENAME    24
#define NO_NEXT_SECTOR     0xFFFFFFFFU

#define FS_TYPE_FILE       0
#define FS_TYPE_DIR        1

// Read a file from the root directory into buf (must be large enough for the file's content).
// Returns 0 on success, -1 if not found.
int fs_read(char* filename, uint8_t* buf);

// Read a file from the root directory: len bytes starting at byte offset into buf.
// Returns 0 on success, -1 if not found, -2 if offset is past end of file.
int fs_read_at(char* filename, uint8_t* buf, uint32_t offset, uint32_t len);

// Write buf to a file in the root directory, creating it if it doesn't exist.
// Returns 0 on success.
int fs_write(char* filename, uint8_t* buf, uint32_t size);

// Fill names with the filenames of all files in the root directory (directories excluded).
// Returns the number of entries written, up to max_names.
int fs_list(char names[][FS_MAX_FILENAME], int max_names);

// Create a new empty directory in the root directory.
// Returns the first sector of the new directory, or NO_NEXT_SECTOR on failure (name taken).
uint32_t fs_mkdir(char* name);

// Look up a directory by name in the root directory.
// Returns its first sector, or NO_NEXT_SECTOR if not found.
uint32_t fs_find_dir(char* name);

// Read a file from the directory rooted at dir_first_sec into buf.
// Returns 0 on success, -1 if not found.
int fs_read_in(uint32_t dir_first_sec, char* filename, uint8_t* buf);

// Write buf to a file in the directory rooted at dir_first_sec, creating it if needed.
// Returns 0 on success.
int fs_write_in(uint32_t dir_first_sec, char* filename, uint8_t* buf, uint32_t size);

// Fill names with filenames of all files in the directory rooted at dir_first_sec.
// Returns the number of entries written, up to max_names.
int fs_list_in(uint32_t dir_first_sec, char names[][FS_MAX_FILENAME], int max_names);

#endif  // FS_H
