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

// Gets input from the terminal and writes it to the file "foo".
void write_foo();

// Reads the file "foo" and prints its contents to the terminal.
void read_foo();

// Writes 600 bytes of junk followed by a message to "test", then reads the message back.
void write_read_junk();

// Userland filesystem unit tests, results go to serial.
void fs_tests();

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
