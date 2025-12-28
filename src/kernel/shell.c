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
    int blinky_pid = 0;
    while(1) {
        char* user_input = get();
        user_clear_line(0);
        char ls[] = "ls";
        if (strcmp(user_input, ls) == 0) {
            char ls_string[] = "ls coming soon";
            user_print_line(ls_string, index);
            index++;
        } else {
            // Start or stop user programs if requested.
            int num_programs = sizeof(programs) / sizeof(programs[0]);
            for (int i = 0; i < num_programs; i++) {
                if (strcmp(user_input, programs[i]->name) == 0) {
                    if (programs[i]->pid == 0) {
                        programs[i]->pid = start_process(programs[i]->func);
                    } else {
                        kill_process(programs[i]->pid);
                        programs[i]->pid = 0;
                    }
                }
            }
        }
    }
}