#include "shell.h"


void shell(){
    program p_blinky  = {"blinky",   blinky,    {0}, 0};
    program p_blinky2 = {"blinky2",  blinky2,   {0}, 0};
    program p_blinky3 = {"blinky3",  blinky3,   {0}, 0};
    program p_fib     = {"fib",      fibonacci,  {0}, 0};
    program p_fs_tests = {"fstests", fs_tests,  {0}, 1};
    program p_ls      = {"ls",       prog_ls,   {0}, 1};
    program p_mkdir   = {"mkdir",    prog_mkdir, {0}, 1};
    program p_rm      = {"rm",       prog_rm,   {0}, 1};
    program p_rmdir   = {"rmdir",    prog_rmdir, {0}, 1};
    program p_write   = {"write",    prog_write, {0}, 1};
    program p_read    = {"read",     prog_read,  {0}, 1};
    program p_editor  = {"editor",   editor,     {0}, 1};
    program p_interp  = {"interp",   interp,     {0}, 1};
    program *programs[] = {
        &p_blinky, &p_blinky2, &p_blinky3, &p_fib,
        &p_fs_tests, &p_ls, &p_mkdir, &p_rm, &p_rmdir,
        &p_write, &p_read, &p_editor, &p_interp
    };

    char cwd[256];
    cwd[0] = '/';
    cwd[1] = '\0';

    int index = 1;
    int num_programs = sizeof(programs) / sizeof(programs[0]);
    while(1) {
        char* user_input = get(index);
        index++;

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

                if (programs[i]->foreground) {
                    if (programs[i]->pids[0] == 0) {
                        shell_start_line = index;
                        programs[i]->pids[0] = start_process(programs[i]->func, args);
                        wait_for(programs[i]->pids[0]);
                        programs[i]->pids[0] = 0;
                        index = shell_resume_line;
                    }
                } else {
                    int slot = -1;
                    for (int s = 0; s < MAX_BG_INSTANCES; s++) {
                        if (programs[i]->pids[s] == 0) { slot = s; break; }
                    }
                    if (slot >= 0) {
                        programs[i]->pids[slot] = start_process(programs[i]->func, args);
                    } else {
                        char err[] = "Max instances already running.";
                        user_print_line(err, index++);
                    }
                }
            }
        }

        if (entered_program == 0) {
            char cd_cmd[]   = "cd";
            char pwd_cmd[]  = "pwd";
            char lp_cmd[]   = "lp";
            char ps_cmd[]   = "ps";
            char kill_cmd[] = "kill";
            char clear_cmd[] = "clear";
            char help_cmd[] = "help";
            char move_cmd[] = "move";
            char copy_cmd[] = "copy";

            if (tokencmp(user_input, cd_cmd) == 0) {
                char* arg = user_input + 2;
                if (*arg == ' ') arg++;
                char resolved[256];
                if (!*arg) {
                    resolved[0] = '/'; resolved[1] = '\0';
                } else if (arg[0] == '/') {
                    strcpy(resolved, arg);
                } else {
                    strcpy(resolved, cwd);
                    int len = strlen(resolved);
                    if (resolved[len - 1] != '/') { resolved[len] = '/'; resolved[len + 1] = '\0'; }
                    strcpy(resolved + strlen(resolved), arg);
                }
                if (dir_exists(resolved)) {
                    strcpy(cwd, resolved);
                } else {
                    char err[] = "cd: directory not found.";
                    user_print_line(err, index++);
                }
            } else if (strcmp(user_input, pwd_cmd) == 0) {
                user_print_line(cwd, index++);
            } else if (strcmp(user_input, lp_cmd) == 0) {
                char hdr[] = "Programs:";
                user_print_line(hdr, index++);
                for (int i = 0; i < num_programs; i++)
                    user_print_line(programs[i]->name, index++);
            } else if (strcmp(user_input, ps_cmd) == 0) {
                char hdr[] = "Running (name pid):";
                user_print_line(hdr, index++);
                for (int i = 0; i < num_programs; i++) {
                    for (int s = 0; s < MAX_BG_INSTANCES; s++) {
                        if (programs[i]->pids[s] > 0) {
                            char line_buf[48];
                            char pid_str[12];
                            uint_to_ascii((uint32_t)programs[i]->pids[s], pid_str);
                            strcpy(line_buf, programs[i]->name);
                            int nlen = strlen(line_buf);
                            line_buf[nlen] = ' ';
                            strcpy(line_buf + nlen + 1, pid_str);
                            user_print_line(line_buf, index++);
                        }
                    }
                }
            } else if (tokencmp(user_input, kill_cmd) == 0) {
                char* arg = user_input + 4;
                if (*arg == ' ') arg++;
                int pid = (int)str_to_uint(arg);
                if (pid > 0) {
                    kill_process(pid);
                    for (int i = 0; i < num_programs; i++)
                        for (int s = 0; s < MAX_BG_INSTANCES; s++)
                            if (programs[i]->pids[s] == pid)
                                programs[i]->pids[s] = 0;
                } else {
                    char err[] = "Usage: kill <pid>";
                    user_print_line(err, index++);
                }
            } else if (strcmp(user_input, clear_cmd) == 0) {
                user_clear_terminal();
                index = 0;
            } else if (strcmp(user_input, help_cmd) == 0) {
                char h1[]  = "lp - list all programs.";
                char h2[]  = "ps - list running instances with PIDs.";
                char h3[]  = "kill <pid> - kill a background process.";
                char h4[]  = "<program> [args] - start program.";
                char h5[]  = "clear - clear terminal.";
                char h6[]  = "cd <path> - change directory.";
                char h7[]  = "pwd - print working directory.";
                char h8[]  = "ls [path] - list directory (default /).";
                char h9[]  = "mkdir <path> - create directory.";
                char h10[] = "rm <path> - remove file.";
                char h11[] = "rmdir <path> - remove empty directory.";
                char h12[] = "write <path> - write input to file.";
                char h13[] = "read <path> - print file contents.";
                char h14[] = "move <src> <dst> - move or rename a file/directory.";
                char h15[] = "copy <src> <dst> - copy a file.";
                user_print_line(h1,  index);
                user_print_line(h2,  index+1);
                user_print_line(h3,  index+2);
                user_print_line(h4,  index+3);
                user_print_line(h5,  index+4);
                user_print_line(h6,  index+5);
                user_print_line(h7,  index+6);
                user_print_line(h8,  index+7);
                user_print_line(h9,  index+8);
                user_print_line(h10, index+9);
                user_print_line(h11, index+10);
                user_print_line(h12, index+11);
                user_print_line(h13, index+12);
                user_print_line(h14, index+13);
                user_print_line(h15, index+14);
                index += 15;
            } else if (tokencmp(user_input, move_cmd) == 0) {
                char* arg = user_input + 4;
                if (*arg == ' ') arg++;

                char src_raw[256];
                int i = 0;
                while (*arg && *arg != ' ' && i < 255) src_raw[i++] = *arg++;
                src_raw[i] = '\0';
                if (*arg == ' ') arg++;

                char dst_raw[256];
                i = 0;
                while (*arg && i < 255) dst_raw[i++] = *arg++;
                dst_raw[i] = '\0';

                if (!src_raw[0] || !dst_raw[0]) {
                    char err[] = "Usage: move <src> <dst>";
                    user_print_line(err, index++);
                } else {
                    char src_abs[256];
                    if (src_raw[0] == '/') {
                        strcpy(src_abs, src_raw);
                    } else {
                        strcpy(src_abs, cwd);
                        int len = strlen(src_abs);
                        if (src_abs[len - 1] != '/') { src_abs[len] = '/'; src_abs[len + 1] = '\0'; }
                        strcpy(src_abs + strlen(src_abs), src_raw);
                    }

                    char dst_abs[256];
                    if (dst_raw[0] == '/') {
                        strcpy(dst_abs, dst_raw);
                    } else {
                        strcpy(dst_abs, cwd);
                        int len = strlen(dst_abs);
                        if (dst_abs[len - 1] != '/') { dst_abs[len] = '/'; dst_abs[len + 1] = '\0'; }
                        strcpy(dst_abs + strlen(dst_abs), dst_raw);
                    }

                    int r = fs_rename(src_abs, dst_abs);
                    if (r == 0) {
                        char done[] = "Moved.";
                        user_print_line(done, index++);
                    } else {
                        char fail[] = "Error: move failed.";
                        user_print_line(fail, index++);
                    }
                }
            } else if (tokencmp(user_input, copy_cmd) == 0) {
                char* arg = user_input + 4;
                if (*arg == ' ') arg++;

                char src_raw[256];
                int i = 0;
                while (*arg && *arg != ' ' && i < 255) src_raw[i++] = *arg++;
                src_raw[i] = '\0';
                if (*arg == ' ') arg++;

                char dst_raw[256];
                i = 0;
                while (*arg && i < 255) dst_raw[i++] = *arg++;
                dst_raw[i] = '\0';

                if (!src_raw[0] || !dst_raw[0]) {
                    char err[] = "Usage: copy <src> <dst>";
                    user_print_line(err, index++);
                } else {
                    char src_abs[256];
                    if (src_raw[0] == '/') {
                        strcpy(src_abs, src_raw);
                    } else {
                        strcpy(src_abs, cwd);
                        int len = strlen(src_abs);
                        if (src_abs[len - 1] != '/') { src_abs[len] = '/'; src_abs[len + 1] = '\0'; }
                        strcpy(src_abs + strlen(src_abs), src_raw);
                    }

                    char dst_abs[256];
                    if (dst_raw[0] == '/') {
                        strcpy(dst_abs, dst_raw);
                    } else {
                        strcpy(dst_abs, cwd);
                        int len = strlen(dst_abs);
                        if (dst_abs[len - 1] != '/') { dst_abs[len] = '/'; dst_abs[len + 1] = '\0'; }
                        strcpy(dst_abs + strlen(dst_abs), dst_raw);
                    }

                    int r = fs_copy(src_abs, dst_abs);
                    if (r == 0) {
                        char done[] = "Copied.";
                        user_print_line(done, index++);
                    } else {
                        char fail[] = "Error: copy failed.";
                        user_print_line(fail, index++);
                    }
                }
            } else {
                char invalid_str[] = "Invalid command. Type help for options.";
                user_print_line(invalid_str, index++);
            }
        }
    }
}
