#include "interrupts.h"
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

void update_cursor(int x, int y) {
    uint16_t pos = y * 80 + x;

    outb(0x3D4, 0x0F); // Tell VGA we are sending the LOW byte
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E); // Tell VGA we are sending the HIGH byte
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void tty_handle_keyboard(uint8_t scancode){
    unsigned char local_kbd[128] = {
        0,  27, '1', '2', '3', '4', '5', '6', '7', '8',
        '9', '0', '-', '=', '\b', '\t', 'q', 'w', 'e', 'r',
        't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
        'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
        '\'', '`', 0, '\\', 'z', 'x', 'c', 'v', 'b', 'n',
        'm', ',', '.', '/', 0, '*', 0, ' '
    };
    unsigned char shift_kbd[128] = {
        0,  27, '!', '@', '#', '$', '%', '^', '&', '*',
        '(', ')', '_', '+', '\b', '\t', 'Q', 'W', 'E', 'R',
        'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0,
        'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
        '"', '~', 0, '|', 'Z', 'X', 'C', 'V', 'B', 'N',
        'M', '<', '>', '?', 0, '*', 0, ' '
    };

    // Left shift: 0x2A press / 0xAA release
    // Right shift: 0x36 press / 0xB6 release
    if (scancode == 0x2A || scancode == 0x36) { tty.shift = 1; return; }
    if (scancode == 0xAA || scancode == 0xB6) { tty.shift = 0; return; }

    const int offset = TTY_GREET_LEN;
    if (tty.active) {
        if (scancode == 0x1c) {
            tty.active = 0;
        } else if (scancode == 0x0E) {
            if (tty.index > 0) {
                tty.input_buffer[tty.index - 1] = 0;
                tty.index -= 1;
                char *video_memory = (char *)0xb8000;
                video_memory[2*tty.index + 160*tty.row + 2*offset] = 0;
                update_cursor(TTY_GREET_LEN + tty.index, tty.row);
            }
        } else {
            char c = tty.shift ? shift_kbd[scancode] : local_kbd[scancode];
            tty.input_buffer[tty.index] = c;
            char *video_memory = (char *)0xb8000;
            video_memory[2*tty.index + 160*tty.row + 2*offset] = c;
            tty.index++;
            update_cursor(TTY_GREET_LEN + tty.index, tty.row);
        }
    }
}
