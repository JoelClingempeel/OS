#include "user_lib.h"
#include "user_progs.h"
#include "utils.h"

int shell_resume_line = 0;
int shell_start_line = 0;


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

    { char p[] = "/fstests"; fs_mkdir(p); }

    // Test 1: mkdir /fstests/docs appears in ls /fstests.
    { char p[] = "/fstests/docs"; fs_mkdir(p); }
    ls_buf[0] = '\0';
    { char p[] = "/fstests"; fs_ls(p, ls_buf); }
    { char name[] = "docs";
      if (ls_contains(ls_buf, name)) {
          char m[] = "PASS: /fstests/docs in ls /fstests"; user_print_line(m, line++);
      } else {
          char m[] = "FAIL: /fstests/docs in ls /fstests"; user_print_line(m, line++);
          { char m2[] = "  ls /fstests returned:"; user_print_line(m2, line++); }
          user_print_line(ls_buf[0] ? ls_buf : "(empty)", line++);
      } }

    // Test 2: mkdir /fstests/docs/notes appears in ls /fstests/docs.
    { char p[] = "/fstests/docs/notes"; fs_mkdir(p); }
    ls_buf[0] = '\0';
    { char p[] = "/fstests/docs"; fs_ls(p, ls_buf); }
    { char name[] = "notes";
      if (ls_contains(ls_buf, name)) {
          char m[] = "PASS: /fstests/docs/notes in ls /fstests/docs"; user_print_line(m, line++);
      } else {
          char m[] = "FAIL: /fstests/docs/notes in ls /fstests/docs"; user_print_line(m, line++);
          { char m2[] = "  ls /fstests/docs returned:"; user_print_line(m2, line++); }
          user_print_line(ls_buf[0] ? ls_buf : "(empty)", line++);
      } }

    // Test 3: write then read /fstests/docs/readme.
    { char p[] = "/fstests/docs/readme"; char c[] = "hello from readme"; r = fs_write(p, c); }
    { char m[] = "  T3 fs_write ret:"; user_print_line(m, line); print_int(r, line + 1); line += 2; }
    memset(buf, 0, sizeof(buf));
    { char p[] = "/fstests/docs/readme"; r = fs_read(p, buf); }
    { char expected[] = "hello from readme";
      if (r == 0 && strcmp(buf, expected) == 0) {
          char m[] = "PASS: write/read /fstests/docs/readme"; user_print_line(m, line++);
      } else {
          char m[] = "FAIL: write/read /fstests/docs/readme"; user_print_line(m, line++);
          { char m2[] = "  fs_read ret:"; user_print_line(m2, line); print_int(r, line + 1); line += 2; }
          { char m2[] = "  buf:"; user_print_line(m2, line++); }
          user_print_line(buf[0] ? buf : "(empty)", line++);
      } }

    // Test 4: write then read /fstests/docs/notes/entry1.
    { char p[] = "/fstests/docs/notes/entry1"; char c[] = "first note"; r = fs_write(p, c); }
    { char m[] = "  T4 fs_write ret:"; user_print_line(m, line); print_int(r, line + 1); line += 2; }
    memset(buf, 0, sizeof(buf));
    { char p[] = "/fstests/docs/notes/entry1"; r = fs_read(p, buf); }
    { char expected[] = "first note";
      if (r == 0 && strcmp(buf, expected) == 0) {
          char m[] = "PASS: write/read /fstests/docs/notes/entry1"; user_print_line(m, line++);
      } else {
          char m[] = "FAIL: write/read /fstests/docs/notes/entry1"; user_print_line(m, line++);
          { char m2[] = "  fs_read ret:"; user_print_line(m2, line); print_int(r, line + 1); line += 2; }
          { char m2[] = "  buf:"; user_print_line(m2, line++); }
          user_print_line(buf[0] ? buf : "(empty)", line++);
      } }

    // Test 5: ls /fstests/docs has 2 entries.
    ls_buf[0] = '\0';
    { char p[] = "/fstests/docs"; fs_ls(p, ls_buf); }
    { int cnt = ls_count(ls_buf);
      if (cnt == 2) {
          char m[] = "PASS: ls /fstests/docs has 2 entries"; user_print_line(m, line++);
      } else {
          char m[] = "FAIL: ls /fstests/docs has 2 entries"; user_print_line(m, line++);
          { char m2[] = "  count:"; user_print_line(m2, line); print_int(cnt, line + 1); line += 2; }
          { char m2[] = "  ls /fstests/docs:"; user_print_line(m2, line++); }
          user_print_line(ls_buf[0] ? ls_buf : "(empty)", line++);
      } }

    // Test 6: fs_read returns -1 for a missing path.
    { char p[] = "/fstests/does/not/exist"; r = fs_read(p, buf); }
    if (r == -1) {
        char m[] = "PASS: fs_read -1 for missing path"; user_print_line(m, line++);
    } else {
        char m[] = "FAIL: fs_read -1 for missing path"; user_print_line(m, line++);
        { char m2[] = "  fs_read ret (expected -1):"; user_print_line(m2, line); print_int(r, line + 1); line += 2; }
    }

    // Test 7: fs_rmdir fails on non-empty directory.
    { char p[] = "/fstests/docs"; r = fs_rmdir(p); }
    if (r == -1) {
        char m[] = "PASS: rmdir fails on non-empty dir"; user_print_line(m, line++);
    } else {
        char m[] = "FAIL: rmdir fails on non-empty dir"; user_print_line(m, line++);
    }

    // Test 8: fs_rm removes /fstests/docs/readme; ls /fstests/docs drops to 1 entry.
    { char p[] = "/fstests/docs/readme"; r = fs_rm(p); }
    ls_buf[0] = '\0';
    { char p[] = "/fstests/docs"; fs_ls(p, ls_buf); }
    { int cnt = ls_count(ls_buf);
      if (r == 0 && cnt == 1) {
          char m[] = "PASS: fs_rm /fstests/docs/readme"; user_print_line(m, line++);
      } else {
          char m[] = "FAIL: fs_rm /fstests/docs/readme"; user_print_line(m, line++);
          { char m2[] = "  fs_rm ret (expected 0):"; user_print_line(m2, line); print_int(r, line + 1); line += 2; }
          { char m2[] = "  ls /fstests/docs count (expected 1):"; user_print_line(m2, line); print_int(cnt, line + 1); line += 2; }
      } }

    // Test 9: deleted file is no longer readable.
    memset(buf, 0, sizeof(buf));
    { char p[] = "/fstests/docs/readme"; r = fs_read(p, buf); }
    if (r == -1) {
        char m[] = "PASS: deleted file not readable"; user_print_line(m, line++);
    } else {
        char m[] = "FAIL: deleted file not readable"; user_print_line(m, line++);
        { char m2[] = "  fs_read ret (expected -1):"; user_print_line(m2, line); print_int(r, line + 1); line += 2; }
    }

    // Test 10: fs_rm /fstests/docs/notes/entry1 then fs_rmdir /fstests/docs/notes.
    { char p[] = "/fstests/docs/notes/entry1"; fs_rm(p); }
    { char p[] = "/fstests/docs/notes"; r = fs_rmdir(p); }
    ls_buf[0] = '\0';
    { char p[] = "/fstests/docs"; fs_ls(p, ls_buf); }
    { int cnt = ls_count(ls_buf);
      if (r == 0 && cnt == 0) {
          char m[] = "PASS: rmdir /fstests/docs/notes after emptying"; user_print_line(m, line++);
      } else {
          char m[] = "FAIL: rmdir /fstests/docs/notes after emptying"; user_print_line(m, line++);
          { char m2[] = "  fs_rmdir ret (expected 0):"; user_print_line(m2, line); print_int(r, line + 1); line += 2; }
          { char m2[] = "  ls /fstests/docs count (expected 0):"; user_print_line(m2, line); print_int(cnt, line + 1); line += 2; }
      } }

    // Test 11: fs_rmdir /fstests/docs now that it is empty.
    { char p[] = "/fstests/docs"; r = fs_rmdir(p); }
    ls_buf[0] = '\0';
    { char p[] = "/fstests"; fs_ls(p, ls_buf); }
    { int cnt = ls_count(ls_buf);
      if (r == 0 && cnt == 0) {
          char m[] = "PASS: rmdir /fstests/docs"; user_print_line(m, line++);
      } else {
          char m[] = "FAIL: rmdir /fstests/docs"; user_print_line(m, line++);
          { char m2[] = "  fs_rmdir ret (expected 0):"; user_print_line(m2, line); print_int(r, line + 1); line += 2; }
          { char m2[] = "  ls /fstests count (expected 0):"; user_print_line(m2, line); print_int(cnt, line + 1); line += 2; }
      } }

    { char p[] = "/fstests"; fs_rmdir(p); }

    { char m[] = "--- DONE ---"; user_print_line(m, line++); }

    shell_resume_line = line;
    kill_process(get_pid());
    while (1);
}

void prog_ls() {
    char* path = get_args();
    char default_path[] = "/";
    if (!path || !path[0]) path = default_path;

    int line = shell_start_line;

    char buf[512];
    buf[0] = '\0';
    fs_ls(path, buf);

    if (!buf[0]) {
        char empty[] = "(empty)";
        user_print_line(empty, line++);
    } else {
        int i = 0;
        while (buf[i]) {
            char name[32];
            int j = 0;
            while (buf[i] && buf[i] != ',') name[j++] = buf[i++];
            name[j] = '\0';
            if (buf[i] == ',') i++;
            user_print_line(name, line++);
        }
    }

    shell_resume_line = line;
    kill_process(get_pid());
    while (1);
}

void prog_mkdir() {
    char* path = get_args();
    int line = shell_start_line;

    if (!path || !path[0]) {
        char err[] = "Usage: mkdir <path>";
        user_print_line(err, line++);
    } else {
        fs_mkdir(path);
        char done[] = "Directory created.";
        user_print_line(done, line++);
    }

    shell_resume_line = line;
    kill_process(get_pid());
    while (1);
}

void prog_rm() {
    char* path = get_args();
    int line = shell_start_line;

    if (!path || !path[0]) {
        char err[] = "Usage: rm <path>";
        user_print_line(err, line++);
    } else {
        int r = fs_rm(path);
        if (r == 0) {
            char done[] = "Removed.";
            user_print_line(done, line++);
        } else {
            char fail[] = "Error: file not found.";
            user_print_line(fail, line++);
        }
    }

    shell_resume_line = line;
    kill_process(get_pid());
    while (1);
}

void prog_rmdir() {
    char* path = get_args();
    int line = shell_start_line;

    if (!path || !path[0]) {
        char err[] = "Usage: rmdir <path>";
        user_print_line(err, line++);
    } else {
        int r = fs_rmdir(path);
        if (r == 0) {
            char done[] = "Directory removed.";
            user_print_line(done, line++);
        } else {
            char fail[] = "Error: not found or not empty.";
            user_print_line(fail, line++);
        }
    }

    shell_resume_line = line;
    kill_process(get_pid());
    while (1);
}

void prog_write() {
    char* path = get_args();
    int line = shell_start_line;

    if (!path || !path[0]) {
        char err[] = "Usage: write <path>";
        user_print_line(err, line++);
        shell_resume_line = line;
        kill_process(get_pid());
        while (1);
    }

    char prompt[] = "Enter text:";
    user_print_line(prompt, line++);
    char* input = get(line++);

    int r = fs_write(path, input);
    if (r == 0) {
        char done[] = "Written.";
        user_print_line(done, line++);
    } else {
        char fail[] = "Error writing file.";
        user_print_line(fail, line++);
    }

    shell_resume_line = line;
    kill_process(get_pid());
    while (1);
}

void prog_read() {
    char* path = get_args();
    int line = shell_start_line;

    if (!path || !path[0]) {
        char err[] = "Usage: read <path>";
        user_print_line(err, line++);
    } else {
        char* buf = (char*)alloc_page();
        int r = fs_read(path, buf);
        if (r == -1) {
            char fail[] = "Error: file not found.";
            user_print_line(fail, line++);
        } else {
            user_print_line(buf, line++);
        }
    }

    shell_resume_line = line;
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
