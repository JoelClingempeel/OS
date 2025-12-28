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

void fibonacci() {
    int start_line = 25 - NUM_FIB_NUMS;
    char str_buf[10];
    uint32_t prev = 1;
    uint32_t cur = 1;
    uint32_t temp;
    char str_1[] = "1";
    user_print_line(str_1, start_line);
    user_print_line(str_1, start_line + 1);
    for (int i = start_line + 2; i < 25; i++) {
        temp = cur;
        cur = cur + prev;
        prev = temp;
        uint_to_ascii(cur, str_buf);
        user_print_line(str_buf, i);
    }
    while(1); 
}
