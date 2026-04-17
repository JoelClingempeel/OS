#include "user_lib.h"
#include "user_progs.h"
#include "utils.h"


void blinky() {
    // Write 600 bytes of junk followed by a message into "test".
    uint8_t* wbuf = (uint8_t*)alloc_page();
    for (int i = 0; i < 600; i++) wbuf[i] = 'X';
    char msg[] = "Hello from offset 600!";
    uint32_t msg_len = strlen(msg) + 1;
    memcpy(wbuf + 600, msg, msg_len);
    char filename[] = "test";
    file_write(filename, wbuf, 600 + msg_len);

    // Read back just the message and display it.
    uint8_t rbuf[32];
    file_read_at(filename, rbuf, 600, msg_len);
    user_print_line((char*)rbuf, 0);

    kill_process(get_pid());
    while (1);
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
