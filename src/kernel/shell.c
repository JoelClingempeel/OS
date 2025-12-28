#include "shell.h"


void shell(){
    user_clear_line(0);
    user_clear_line(3);
    while(1) {
        char* user_input = get();
        char clear[] = "                              ";
        printk(clear);
        delay(200);
        printk(user_input);
    }
}