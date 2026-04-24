#ifndef FILES_H
#define FILES_H

#include <stdint.h>
#include "disk.h"

#define MAX_NAME 64

// Initiate file system.
void fs_init(void);

// Read from and write to a file by sector number.
void read_file(uint32_t sector_num, char* buffer);
void write_file(uint32_t sector_num, char* buffer);

// Read/write a file by path. Return 0 on success, -1 if path not found.
int read_path(char* path, char* buffer);
int write_path(char* path, char* buffer);

// Creates a file or directory at `path`. Pass is_dir=1 for a directory, 0 for a file.
void make_file(char* path, int is_dir);

// Lists immediate children of `path`. Returns count, or -1 if path not found.
int lsdir(char* path, char names[][MAX_NAME], int max_names);

// Move/rename src to dst. Returns 0 on success, -1 on failure.
int rename_path(char* src, char* dst);

// Delete a file or an empty directory. Return 0 on success, -1 on failure.
int delete_file(char* path);
int delete_dir(char* path);

#endif  // FILES_H
