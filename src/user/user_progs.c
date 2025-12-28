#include "user_lib.h"
#include "user_progs.h"


void blinky() {
    while (1) {
        delay(50);
        put_char('N', 0x0c, 0x350);  // Red
        delay(50);
        put_char(0, 0x0c, 0x350);
    }
}

void blinky2() {
    while (1) {
        delay(50);
        put_char('Q', 0x0b, 0x700);  // Cyan
        delay(50);
        put_char(0, 0x0b, 0x700);
    }
}

void blinky3() {
    while (1) {
        delay(50);
        put_char('Y', 0x0a, 0x100);  // Green
        delay(50);
        put_char(0, 0x0a, 0x100);
    }
}
