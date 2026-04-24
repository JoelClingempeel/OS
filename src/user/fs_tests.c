#include "user_lib.h"
#include "user_progs.h"
#include "utils.h"


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

    // Test 12: multisector write/read (600 bytes junk + message spanning 2 sectors).
    { char* wbuf = (char*)alloc_page();
      for (int i = 0; i < 600; i++) wbuf[i] = 'A';
      char msg[] = "Hello from offset 600!";
      memcpy(wbuf + 600, msg, strlen(msg) + 1);
      char p[] = "/testfile";
      fs_write(p, wbuf); }
    { char* rbuf = (char*)alloc_page();
      char p[] = "/testfile";
      r = fs_read(p, rbuf);
      fs_rm(p);
      char expected[] = "Hello from offset 600!";
      if (r == 0 && strcmp(rbuf + 600, expected) == 0) {
          char m[] = "PASS: multisector write/read"; user_print_line(m, line++);
      } else {
          char m[] = "FAIL: multisector write/read"; user_print_line(m, line++);
      } }

    // Test 13: rename within same directory.
    { char p[] = "/mvtest"; fs_mkdir(p); }
    { char p[] = "/mvtest/original"; char c[] = "rename content"; fs_write(p, c); }
    { char src[] = "/mvtest/original"; char dst[] = "/mvtest/renamed"; r = fs_rename(src, dst); }
    ls_buf[0] = '\0';
    { char p[] = "/mvtest"; fs_ls(p, ls_buf); }
    memset(buf, 0, sizeof(buf));
    { char p[] = "/mvtest/renamed"; fs_read(p, buf); }
    { char has[]      = "renamed";
      char gone[]     = "original";
      char expected[] = "rename content";
      if (r == 0 && ls_contains(ls_buf, has) && !ls_contains(ls_buf, gone) && strcmp(buf, expected) == 0) {
          char m[] = "PASS: rename within directory"; user_print_line(m, line++);
      } else {
          char m[] = "FAIL: rename within directory"; user_print_line(m, line++);
      } }
    { char p[] = "/mvtest/renamed"; fs_rm(p); }
    { char p[] = "/mvtest"; fs_rmdir(p); }

    // Test 14: move between directories.
    { char p[] = "/mvsrc"; fs_mkdir(p); }
    { char p[] = "/mvdst"; fs_mkdir(p); }
    { char p[] = "/mvsrc/myfile"; char c[] = "move content"; fs_write(p, c); }
    { char src[] = "/mvsrc/myfile"; char dst[] = "/mvdst/myfile"; r = fs_rename(src, dst); }
    ls_buf[0] = '\0';
    { char p[] = "/mvdst"; fs_ls(p, ls_buf); }
    { char src_ls[512]; src_ls[0] = '\0';
      { char p[] = "/mvsrc"; fs_ls(p, src_ls); }
      memset(buf, 0, sizeof(buf));
      { char p[] = "/mvdst/myfile"; fs_read(p, buf); }
      char has[]      = "myfile";
      char expected[] = "move content";
      if (r == 0 && ls_contains(ls_buf, has) && !ls_contains(src_ls, has) && strcmp(buf, expected) == 0) {
          char m[] = "PASS: move between directories"; user_print_line(m, line++);
      } else {
          char m[] = "FAIL: move between directories"; user_print_line(m, line++);
      } }
    { char p[] = "/mvdst/myfile"; fs_rm(p); }
    { char p[] = "/mvsrc"; fs_rmdir(p); }
    { char p[] = "/mvdst"; fs_rmdir(p); }

    { char m[] = "--- DONE ---"; user_print_line(m, line++); }

    shell_resume_line = line;
    kill_process(get_pid());
    while (1);
}
