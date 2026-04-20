#include "shell.h"


void shell(){
    program p_blinky = {"blinky", blinky, 0, 0};
    program p_blinky2 = {"blinky2", blinky2, 0, 0};
    program p_blinky3 = {"blinky3", blinky3, 0, 0};
    program p_fib = {"fib", fibonacci, 0, 0};
    program p_fs_tests = {"fstests", fs_tests, 0, 1};
    program p_ls = {"ls", prog_ls, 0, 1};
    program p_mkdir = {"mkdir", prog_mkdir, 0, 1};
    program p_rm = {"rm", prog_rm, 0, 1};
    program p_rmdir = {"rmdir", prog_rmdir, 0, 1};
    program p_write = {"write", prog_write, 0, 1};
    program p_read = {"read", prog_read, 0, 1};
    program *programs[] = {
        &p_blinky,
        &p_blinky2,
        &p_blinky3,
        &p_fib,
        &p_fs_tests,
        &p_ls,
        &p_mkdir,
        &p_rm,
        &p_rmdir,
        &p_write,
        &p_read
    };

    char cwd[256];
    cwd[0] = '/';
    cwd[1] = '\0';

    int index = 1;
    int num_programs = sizeof(programs) / sizeof(programs[0]);
    while(1) {
        char* user_input = get(index);
        index++;

        // Start or stop user programs if requested.
        int entered_program = 0;
        for (int i = 0; i < num_programs; i++) {
            if (tokencmp(user_input, programs[i]->name) == 0) {
                char* args = user_input + strlen(programs[i]->name) + 1;
                char resolved_args[256];
                if (!*args) {
                    args = cwd;
                } else if (args[0] != '/') {
                    strcpy(resolved_args, cwd);
                    int cwdlen = strlen(resolved_args);
                    if (resolved_args[cwdlen - 1] != '/') {
                        resolved_args[cwdlen] = '/';
                        resolved_args[cwdlen + 1] = '\0';
                    }
                    strcpy(resolved_args + strlen(resolved_args), args);
                    args = resolved_args;
                }
                entered_program = 1;
                if (programs[i]->pid == 0) {
                    shell_start_line = index;
                    programs[i]->pid = start_process(programs[i]->func, args);
                    if (programs[i]->foreground) {
                        wait_for(programs[i]->pid);
                        programs[i]->pid = 0;
                        index = shell_resume_line;
                    }
                } else {
                    kill_process(programs[i]->pid);
                    programs[i]->pid = 0;
                }
            }
        }

        if (entered_program == 0) {
            char cd_cmd[] = "cd";
            char pwd_cmd[] = "pwd";
            char lp[] = "lp";
            char ps[] = "ps";
            char clear[] = "clear";
            char help[] = "help";
            if (tokencmp(user_input, cd_cmd) == 0) {
                char* arg = user_input + 2;
                if (*arg == ' ') arg++;
                char resolved[256];
                if (!*arg) {
                    resolved[0] = '/';
                    resolved[1] = '\0';
                } else if (arg[0] == '/') {
                    strcpy(resolved, arg);
                } else {
                    // join cwd + "/" + arg
                    strcpy(resolved, cwd);
                    int len = strlen(resolved);
                    if (resolved[len - 1] != '/') { resolved[len] = '/'; resolved[len + 1] = '\0'; }
                    strcpy(resolved + strlen(resolved), arg);
                }
                if (dir_exists(resolved)) {
                    strcpy(cwd, resolved);
                } else {
                    char err[] = "cd: directory not found.";
                    user_print_line(err, index);
                    index++;
                }
            } else if (strcmp(user_input, pwd_cmd) == 0) {
                user_print_line(cwd, index);
                index++;
            } else if (strcmp(user_input, lp) == 0) {
                char lp_str[] = "Programs:";
                user_print_line(lp_str, index);
                index++;
                for (int i = 0; i < num_programs; i++) {
                    user_print_line(programs[i]->name, index);
                    index++;
                }
            } else if (strcmp(user_input, ps) == 0) {
                char ps_str[] = "Running:";
                user_print_line(ps_str, index);
                index++;
                for (int i = 0; i < num_programs; i++) {
                    if (programs[i]->pid > 0) {
                        user_print_line(programs[i]->name, index);
                        index++;
                    }
                }
            } else if (strcmp(user_input, clear) == 0) {
                user_clear_terminal();
                index = 0;
            } else if (strcmp(user_input, help) == 0) {
                char help1_str[] = "lp - list all programs.";
                char help2_str[] = "ps - list running programs.";
                char help3_str[] = "<program name> [args] - start or stop program.";
                char help4_str[] = "clear - clear terminal.";
                char help5_str[] = "help - show commands.";
                char help6_str[] = "cd <path> - change directory.";
                char help6b_str[] = "pwd - print working directory.";
                char help7_str[] = "ls [path] - list directory (default /).";
                char help8_str[] = "mkdir <path> - create directory.";
                char help9_str[] = "rm <path> - remove file.";
                char help10_str[] = "rmdir <path> - remove empty directory.";
                char help11_str[] = "write <path> - write input to file.";
                char help12_str[] = "read <path> - print file contents.";
                user_print_line(help1_str, index);
                user_print_line(help2_str, index+1);
                user_print_line(help3_str, index+2);
                user_print_line(help4_str, index+3);
                user_print_line(help5_str, index+4);
                user_print_line(help6_str, index+5);
                user_print_line(help6b_str, index+6);
                user_print_line(help7_str, index+7);
                user_print_line(help8_str, index+8);
                user_print_line(help9_str, index+9);
                user_print_line(help10_str, index+10);
                user_print_line(help11_str, index+11);
                user_print_line(help12_str, index+12);
                index += 13;
            } else {
                char invalid_str[] = "Invalid command. Type help for options.";
                user_print_line(invalid_str, index);
                index++;
            }
        }
    }
}
