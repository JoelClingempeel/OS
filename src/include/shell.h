#ifndef SHELL_H
#define SHELL_H

#include "user.h"


typedef struct {
    char name[32];
    void (*func)(void);
    int pid;
} program;

// User process to start a shell.
void shell();

#endif  // SHELL_H
