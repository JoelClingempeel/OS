#include "user_lib.h"
#include "user_progs.h"
#include "utils.h"


void blinky() {
    char* args = get_args();
    user_print_line(args, 20);
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

void write_foo() {
    int line = 0;
    char prompt[] = "Enter text to write to foo:";
    user_print_line(prompt, line++);
    char* input = get(line++);

    uint8_t buf[SECTOR_SIZE];
    memset(buf, 0, SECTOR_SIZE);
    memcpy(buf, input, strlen(input) + 1);
    char filename[] = "foo";
    file_write(filename, buf, strlen(input) + 1);

    char done[] = "Written to foo.";
    user_print_line(done, line);
    kill_process(get_pid());
    while (1);
}

void read_foo() {
    uint8_t buf[SECTOR_SIZE];
    char filename[] = "foo";
    int result = file_read(filename, buf);

    int line = 0;
    if (result == -1) {
        char not_found[] = "foo not found.";
        user_print_line(not_found, line);
    } else {
        user_print_line((char*)buf, line);
    }
    kill_process(get_pid());
    while (1);
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
