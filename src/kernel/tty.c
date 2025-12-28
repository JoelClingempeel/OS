#include "scheduler.h"
#include "tty.h"
#include "utils.h"


struct tty_struct tty = {
    .input_buffer = {0},
    .index = 0,
    .row = 0,
    .active = 0,
    .task_index = 0
};

void tty_handle_keyboard(uint8_t scancode){
    unsigned char local_kbd[128] = {
        0,  27, '1', '2', '3', '4', '5', '6', '7', '8',
        '9', '0', '-', '=', '\b', '\t', 'q', 'w', 'e', 'r',
        't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
        'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
        '\'', '`', 0, '\\', 'z', 'x', 'c', 'v', 'b', 'n',
        'm', ',', '.', '/', 0, '*', 0, ' '
    };
    if (tty.active) {
        if (scancode == 0x1c) {
            tty.active = 0;
        } else {
            char c = local_kbd[scancode];
            tty.input_buffer[tty.index] = c;
            char *video_memory = (char *)0xb8000;
            video_memory[2*tty.index + 160*tty.row] = c;
            tty.index++;
        }
    }
}
