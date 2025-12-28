#include "shell.h"


void shell(){
    while(1) {
        char* user_input = get();
        char clear[] = "                              ";
        printk(clear);
        delay(200);
        printk(user_input);
    }
}