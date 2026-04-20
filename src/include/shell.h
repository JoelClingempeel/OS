#ifndef SHELL_H
#define SHELL_H

#include "fs_tests.h"
#include "user_lib.h"
#include "user_progs.h"


#define MAX_BG_INSTANCES 4

typedef struct {
    char name[32];
    void (*func)(void);
    int pids[MAX_BG_INSTANCES];
    int foreground;  // 1 = shell waits for exit before resuming, 0 = background
} program;

// User process to start a shell.
void shell();

#endif  // SHELL_H
