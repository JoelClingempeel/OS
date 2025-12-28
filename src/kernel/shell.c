#include "shell.h"


void shell(){
    int index = 1;
    int blinky_pid = 0;
    while(1) {
        char* user_input = get();
        user_clear_line(0);
        char ls[] = "ls";
        char blinky[] = "blinky";
        if (strcmp(user_input, ls) == 0) {
            char ls_string[] = "blinky - Blink a red N";
            user_print_line(ls_string, index);
            index++;
        } else if (strcmp(user_input, blinky) == 0) {
            // start_process(user_test_program2);
            if (blinky_pid == 0) {
                blinky_pid = start_process(user_test_program2);
            } else {
                kill_process(blinky_pid);
                blinky_pid = 0;
            }
        } else {
            char invalid_string[] = "Invalid command. Type ls to see programs.";
            user_print_line(invalid_string, index);
            index++;
        }
    }
}