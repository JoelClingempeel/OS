#ifndef TTY_H
#define TTY_H

#include <stdint.h>

#define TTY_GREETING "os% "
#define TTY_GREET_LEN sizeof(TTY_GREETING)-1


struct tty_struct {
    char input_buffer[1024];
    uint16_t index;
    uint8_t row;
    uint8_t active;
    uint8_t task_index;
};

extern struct tty_struct tty;

void update_cursor(int x, int y);

void tty_handle_keyboard(uint8_t scancode);

#endif  // TTY_H
