#ifndef USER_PROGS_H
#define USER_PROGS_H

#define NUM_FIB_NUMS 10

// Foreground programs write the next available terminal line here before exiting
// so the shell knows where to place its next prompt.
extern int shell_resume_line;
extern int shell_start_line;


// Make a red N repeatedly appear and disappear.
void blinky();

// Make a cyan Q repeatedly appear and disappear.
void blinky2();

// Make a green Y repeatedly appear and disappear.
void blinky3();

// Computes Fibonacci numbers.
void fibonacci();

// List directory contents one per line.
void prog_ls();

// Create a directory.
void prog_mkdir();

// Remove a file.
void prog_rm();

// Remove an empty directory.
void prog_rmdir();

// Write one line of user input to a file.
void prog_write();

// Print file contents.
void prog_read();

#endif  // USER_PROGS_H
