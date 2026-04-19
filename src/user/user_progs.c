#include "user_lib.h"
#include "user_progs.h"
#include "utils.h"

int shell_resume_line = 0;


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

void write_foo() {
    user_clear_terminal();
    int line = 0;
    char prompt[] = "Enter text to write to foo:";
    user_print_line(prompt, line++);
    char* input = get(line++);

    uint8_t buf[SECTOR_SIZE];
    memset(buf, 0, SECTOR_SIZE);
    memcpy(buf, input, strlen(input) + 1);
    char filename[] = "foo";
    file_write(filename, buf, strlen(input) + 1);

    line++;  // blank line between input and confirmation
    char done[] = "Written to foo.";
    user_print_line(done, line);
    shell_resume_line = line + 2;
    kill_process(get_pid());
    while (1);
}

void read_foo() {
    user_clear_terminal();
    uint8_t buf[SECTOR_SIZE];
    char filename[] = "foo";
    int result = file_read(filename, buf);

    int line = 0;
    if (result == -1) {
        char not_found[] = "foo not found.";
        user_print_line(not_found, line);
    } else {
        char header[] = "Contents of foo:";
        user_print_line(header, line++);
        line++;  // blank line
        user_print_line((char*)buf, line);
    }
    shell_resume_line = line + 2;
    kill_process(get_pid());
    while (1);
}

void write_read_junk() {
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

    shell_resume_line = 2;
    kill_process(get_pid());
    while (1);
}

static int ls_count(char* buf) {
    if (!buf[0]) return 0;
    int count = 1;
    for (int i = 0; buf[i]; i++)
        if (buf[i] == ',') count++;
    return count;
}

static int ls_contains(char* buf, char* name) {
    int i = 0;
    while (buf[i]) {
        int j = 0;
        while (name[j] && buf[i + j] && buf[i + j] != ',' && name[j] == buf[i + j])
            j++;
        if (!name[j] && (!buf[i + j] || buf[i + j] == ',')) return 1;
        while (buf[i] && buf[i] != ',') i++;
        if (buf[i] == ',') i++;
    }
    return 0;
}

static void print_int(int v, int line) {
    char tmp[16];
    if (v < 0) {
        tmp[0] = '-';
        uint_to_ascii((uint32_t)(-v), tmp + 1);
    } else {
        uint_to_ascii((uint32_t)v, tmp);
    }
    user_print_line(tmp, line);
}

void fs_tests() {
    user_clear_terminal();
    char buf[512];
    char ls_buf[512];
    int r;
    int line = 0;

    { char m[] = "--- USERLAND FS TESTS ---"; user_print_line(m, line++); }

    // Test 1: mkdir /docs appears in ls /.
    { char p[] = "/docs"; fs_mkdir(p); }
    ls_buf[0] = '\0';
    { char p[] = "/"; fs_ls(p, ls_buf); }
    { char name[] = "docs";
      if (ls_contains(ls_buf, name)) {
          char m[] = "PASS: /docs in ls /"; user_print_line(m, line++);
      } else {
          char m[] = "FAIL: /docs in ls /"; user_print_line(m, line++);
          { char m2[] = "  ls / returned:"; user_print_line(m2, line++); }
          user_print_line(ls_buf[0] ? ls_buf : "(empty)", line++);
      } }

    // Test 2: mkdir /docs/notes appears in ls /docs.
    { char p[] = "/docs/notes"; fs_mkdir(p); }
    ls_buf[0] = '\0';
    { char p[] = "/docs"; fs_ls(p, ls_buf); }
    { char name[] = "notes";
      if (ls_contains(ls_buf, name)) {
          char m[] = "PASS: /docs/notes in ls /docs"; user_print_line(m, line++);
      } else {
          char m[] = "FAIL: /docs/notes in ls /docs"; user_print_line(m, line++);
          { char m2[] = "  ls /docs returned:"; user_print_line(m2, line++); }
          user_print_line(ls_buf[0] ? ls_buf : "(empty)", line++);
      } }

    // Test 3: write then read /docs/readme.
    { char p[] = "/docs/readme"; char c[] = "hello from readme"; r = fs_write(p, c); }
    { char m[] = "  T3 fs_write ret:"; user_print_line(m, line); print_int(r, line + 1); line += 2; }
    memset(buf, 0, sizeof(buf));
    { char p[] = "/docs/readme"; r = fs_read(p, buf); }
    { char expected[] = "hello from readme";
      if (r == 0 && strcmp(buf, expected) == 0) {
          char m[] = "PASS: write/read /docs/readme"; user_print_line(m, line++);
      } else {
          char m[] = "FAIL: write/read /docs/readme"; user_print_line(m, line++);
          { char m2[] = "  fs_read ret:"; user_print_line(m2, line); print_int(r, line + 1); line += 2; }
          { char m2[] = "  buf:"; user_print_line(m2, line++); }
          user_print_line(buf[0] ? buf : "(empty)", line++);
      } }

    // Test 4: write then read /docs/notes/entry1.
    { char p[] = "/docs/notes/entry1"; char c[] = "first note"; r = fs_write(p, c); }
    { char m[] = "  T4 fs_write ret:"; user_print_line(m, line); print_int(r, line + 1); line += 2; }
    memset(buf, 0, sizeof(buf));
    { char p[] = "/docs/notes/entry1"; r = fs_read(p, buf); }
    { char expected[] = "first note";
      if (r == 0 && strcmp(buf, expected) == 0) {
          char m[] = "PASS: write/read /docs/notes/entry1"; user_print_line(m, line++);
      } else {
          char m[] = "FAIL: write/read /docs/notes/entry1"; user_print_line(m, line++);
          { char m2[] = "  fs_read ret:"; user_print_line(m2, line); print_int(r, line + 1); line += 2; }
          { char m2[] = "  buf:"; user_print_line(m2, line++); }
          user_print_line(buf[0] ? buf : "(empty)", line++);
      } }

    // Test 5: ls /docs has 2 entries.
    ls_buf[0] = '\0';
    { char p[] = "/docs"; fs_ls(p, ls_buf); }
    { int cnt = ls_count(ls_buf);
      if (cnt == 2) {
          char m[] = "PASS: ls /docs has 2 entries"; user_print_line(m, line++);
      } else {
          char m[] = "FAIL: ls /docs has 2 entries"; user_print_line(m, line++);
          { char m2[] = "  count:"; user_print_line(m2, line); print_int(cnt, line + 1); line += 2; }
          { char m2[] = "  ls /docs:"; user_print_line(m2, line++); }
          user_print_line(ls_buf[0] ? ls_buf : "(empty)", line++);
      } }

    // Test 6: fs_read returns -1 for a missing path.
    { char p[] = "/does/not/exist"; r = fs_read(p, buf); }
    if (r == -1) {
        char m[] = "PASS: fs_read -1 for missing path"; user_print_line(m, line++);
    } else {
        char m[] = "FAIL: fs_read -1 for missing path"; user_print_line(m, line++);
        { char m2[] = "  fs_read ret (expected -1):"; user_print_line(m2, line); print_int(r, line + 1); line += 2; }
    }

    // Test 7: fs_rmdir fails on non-empty directory.
    { char p[] = "/docs"; r = fs_rmdir(p); }
    if (r == -1) {
        char m[] = "PASS: rmdir fails on non-empty dir"; user_print_line(m, line++);
    } else {
        char m[] = "FAIL: rmdir fails on non-empty dir"; user_print_line(m, line++);
    }

    // Test 8: fs_rm removes /docs/readme; ls /docs drops to 1 entry.
    { char p[] = "/docs/readme"; r = fs_rm(p); }
    ls_buf[0] = '\0';
    { char p[] = "/docs"; fs_ls(p, ls_buf); }
    { int cnt = ls_count(ls_buf);
      if (r == 0 && cnt == 1) {
          char m[] = "PASS: fs_rm /docs/readme"; user_print_line(m, line++);
      } else {
          char m[] = "FAIL: fs_rm /docs/readme"; user_print_line(m, line++);
          { char m2[] = "  fs_rm ret (expected 0):"; user_print_line(m2, line); print_int(r, line + 1); line += 2; }
          { char m2[] = "  ls /docs count (expected 1):"; user_print_line(m2, line); print_int(cnt, line + 1); line += 2; }
      } }

    // Test 9: deleted file is no longer readable.
    memset(buf, 0, sizeof(buf));
    { char p[] = "/docs/readme"; r = fs_read(p, buf); }
    if (r == -1) {
        char m[] = "PASS: deleted file not readable"; user_print_line(m, line++);
    } else {
        char m[] = "FAIL: deleted file not readable"; user_print_line(m, line++);
        { char m2[] = "  fs_read ret (expected -1):"; user_print_line(m2, line); print_int(r, line + 1); line += 2; }
    }

    // Test 10: fs_rm /docs/notes/entry1 then fs_rmdir /docs/notes.
    { char p[] = "/docs/notes/entry1"; fs_rm(p); }
    { char p[] = "/docs/notes"; r = fs_rmdir(p); }
    ls_buf[0] = '\0';
    { char p[] = "/docs"; fs_ls(p, ls_buf); }
    { int cnt = ls_count(ls_buf);
      if (r == 0 && cnt == 0) {
          char m[] = "PASS: rmdir /docs/notes after emptying"; user_print_line(m, line++);
      } else {
          char m[] = "FAIL: rmdir /docs/notes after emptying"; user_print_line(m, line++);
          { char m2[] = "  fs_rmdir ret (expected 0):"; user_print_line(m2, line); print_int(r, line + 1); line += 2; }
          { char m2[] = "  ls /docs count (expected 0):"; user_print_line(m2, line); print_int(cnt, line + 1); line += 2; }
      } }

    // Test 11: fs_rmdir /docs now that it is empty.
    { char p[] = "/docs"; r = fs_rmdir(p); }
    ls_buf[0] = '\0';
    { char p[] = "/"; fs_ls(p, ls_buf); }
    { int cnt = ls_count(ls_buf);
      if (r == 0 && cnt == 0) {
          char m[] = "PASS: rmdir /docs"; user_print_line(m, line++);
      } else {
          char m[] = "FAIL: rmdir /docs"; user_print_line(m, line++);
          { char m2[] = "  fs_rmdir ret (expected 0):"; user_print_line(m2, line); print_int(r, line + 1); line += 2; }
          { char m2[] = "  ls / count (expected 0):"; user_print_line(m2, line); print_int(cnt, line + 1); line += 2; }
      } }

    { char m[] = "--- DONE ---"; user_print_line(m, line++); }

    shell_resume_line = line + 1;
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
