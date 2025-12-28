#include "shell.h"


void __attribute__((optimize("O0"))) shell(){
    program p_blinky = {"blinky", blinky, 0};
    program p_blinky2 = {"blinky2", blinky2, 0};
    program p_blinky3 = {"blinky3", blinky3, 0};
    program *programs[] = {
        &p_blinky,
        &p_blinky2,
        &p_blinky3
    };

    int index = 1;
    int num_programs = sizeof(programs) / sizeof(programs[0]);
    while(1) {
        char* user_input = get(index);
        index++;

        // Start or stop user programs if requested.
        int entered_program = 0;
        for (int i = 0; i < num_programs; i++) {
            if (strcmp(user_input, programs[i]->name) == 0) {
                entered_program = 1;
                if (programs[i]->pid == 0) {
                    programs[i]->pid = start_process(programs[i]->func);
                } else {
                    kill_process(programs[i]->pid);
                    programs[i]->pid = 0;
                }
            }
        }

        if (entered_program == 0) {
            char ls[] = "ls";
            char ps[] = "ps";
            char help[] = "help";
            if (strcmp(user_input, ls) == 0) {
                char ls_str[] = "Programs:";
                user_print_line(ls_str, index);
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
            } else if (strcmp(user_input, help) == 0) {
                char help1_str[] = "ls - list all programs.";
                char help2_str[] = "ps - list running programs.";
                char help3_str[] = "<program name> - start or stop program.";
                char help4_str[] = "help - show commands.";
                user_print_line(help1_str, index);
                user_print_line(help2_str, index+1);
                user_print_line(help3_str, index+2);
                user_print_line(help4_str, index+3);
                index += 4;
            } else {
                char invalid_str[] = "Invalid command. Type help for options.";
                user_print_line(invalid_str, index);
                index++;
            }
        }
    }
}