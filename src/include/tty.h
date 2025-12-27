#ifndef TTY_H
#define TTY_H

#include <stdint.h>

#include "interrupts.h"  // Only for outb for testing

struct tty_struct {
    char input_buffer[1024];
    uint16_t index;
    uint8_t active;
};

extern struct tty_struct tty;

void tty_handle_keyboard(uint8_t scancode);

#endif  // TTY_H
