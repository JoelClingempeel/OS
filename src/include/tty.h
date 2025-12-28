#ifndef TTY_H
#define TTY_H

#include <stdint.h>


struct tty_struct {
    char input_buffer[1024];
    uint16_t index;
    uint8_t active;
    uint8_t task_index;
};

extern struct tty_struct tty;

void tty_handle_keyboard(uint8_t scancode);

#endif  // TTY_H
