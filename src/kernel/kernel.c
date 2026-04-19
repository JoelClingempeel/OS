#include "files.h"
#include "interrupts.h"
#include "memory.h"
#include "scheduler.h"
#include "shell.h"
#include "tss.h"
#include "user_lib.h"
#include "user_progs.h"
#include "utils.h"

static void run_fs_tests(void) {
    char buf[512];
    char names[8][MAX_NAME];
    int count, r;

    SERIAL_PRINT("--- FS TESTS ---\n");

    // Test 1: mkdir /docs appears in lsdir /.
    { char p[] = "/docs"; make_file(p, 1); }
    { char p[] = "/"; count = lsdir(p, names, 8); }
    { char expected[] = "docs";
      if (count == 1 && strcmp(names[0], expected) == 0)
          SERIAL_PRINT("PASS: /docs in lsdir /\n");
      else
          SERIAL_PRINT("FAIL: /docs in lsdir /\n"); }

    // Test 2: mkdir /docs/notes appears in lsdir /docs.
    { char p[] = "/docs/notes"; make_file(p, 1); }
    { char p[] = "/docs"; count = lsdir(p, names, 8); }
    { char expected[] = "notes";
      if (count == 1 && strcmp(names[0], expected) == 0)
          SERIAL_PRINT("PASS: /docs/notes in lsdir /docs\n");
      else {
          SERIAL_PRINT("FAIL: /docs/notes in lsdir /docs\n");
          SERIAL_PRINT("  entries found:\n");
          for (int i = 0; i < count; i++) {
              SERIAL_PRINT("    - ");
              serial_print(names[i]);
              SERIAL_PRINT("\n");
          }
      } }

    // Test 3: write then read /docs/readme.
    { char p[] = "/docs/readme"; make_file(p, 0); }
    { char p[] = "/docs/readme"; char c[] = "hello from readme"; write_path(p, c); }
    memset(buf, 0, sizeof(buf));
    { char p[] = "/docs/readme"; r = read_path(p, buf); }
    { char expected[] = "hello from readme";
      if (r == 0 && strcmp(buf, expected) == 0)
          SERIAL_PRINT("PASS: write/read /docs/readme\n");
      else
          SERIAL_PRINT("FAIL: write/read /docs/readme\n"); }

    // Test 4: write then read /docs/notes/entry1.
    { char p[] = "/docs/notes/entry1"; make_file(p, 0); }
    { char p[] = "/docs/notes/entry1"; char c[] = "first note"; write_path(p, c); }
    memset(buf, 0, sizeof(buf));
    { char p[] = "/docs/notes/entry1"; r = read_path(p, buf); }
    { char expected[] = "first note";
      if (r == 0 && strcmp(buf, expected) == 0)
          SERIAL_PRINT("PASS: write/read /docs/notes/entry1\n");
      else
          SERIAL_PRINT("FAIL: write/read /docs/notes/entry1\n"); }

    // Test 5: lsdir /docs has 2 entries.
    { char p[] = "/docs"; count = lsdir(p, names, 8); }
    if (count == 2)
        SERIAL_PRINT("PASS: lsdir /docs has 2 entries\n");
    else {
        SERIAL_PRINT("FAIL: lsdir /docs has 2 entries\n");
        SERIAL_PRINT("  entries found:\n");
        for (int i = 0; i < count; i++) {
            SERIAL_PRINT("    - ");
            serial_print(names[i]);
            SERIAL_PRINT("\n");
        }
    }

    // Test 6: read_path returns -1 for a missing path.
    { char p[] = "/does/not/exist"; r = read_path(p, buf); }
    if (r == -1)
        SERIAL_PRINT("PASS: read_path -1 for missing path\n");
    else
        SERIAL_PRINT("FAIL: read_path -1 for missing path\n");

    // Test 7: delete_dir fails on non-empty directory.
    { char p[] = "/docs"; r = delete_dir(p); }
    if (r == -1)
        SERIAL_PRINT("PASS: delete_dir fails on non-empty dir\n");
    else
        SERIAL_PRINT("FAIL: delete_dir fails on non-empty dir\n");

    // Test 8: delete_file removes /docs/readme; lsdir /docs drops to 1 entry.
    { char p[] = "/docs/readme"; r = delete_file(p); }
    { char p[] = "/docs"; count = lsdir(p, names, 8); }
    if (r == 0 && count == 1)
        SERIAL_PRINT("PASS: delete_file /docs/readme\n");
    else
        SERIAL_PRINT("FAIL: delete_file /docs/readme\n");

    // Test 9: deleted file is no longer readable.
    memset(buf, 0, sizeof(buf));
    { char p[] = "/docs/readme"; r = read_path(p, buf); }
    if (r == -1)
        SERIAL_PRINT("PASS: deleted file not readable\n");
    else
        SERIAL_PRINT("FAIL: deleted file not readable\n");

    // Test 10: delete_file on /docs/notes/entry1, then delete_dir /docs/notes.
    { char p[] = "/docs/notes/entry1"; r = delete_file(p); }
    { int r2; char p[] = "/docs/notes"; r2 = delete_dir(p); r = r2; }
    { char p[] = "/docs"; count = lsdir(p, names, 8); }
    if (r == 0 && count == 0)
        SERIAL_PRINT("PASS: delete_dir /docs/notes after emptying it\n");
    else
        SERIAL_PRINT("FAIL: delete_dir /docs/notes after emptying it\n");

    // Test 11: delete_dir /docs now that it is empty.
    { char p[] = "/docs"; r = delete_dir(p); }
    { char p[] = "/"; count = lsdir(p, names, 8); }
    if (r == 0 && count == 0)
        SERIAL_PRINT("PASS: delete_dir /docs\n");
    else
        SERIAL_PRINT("FAIL: delete_dir /docs\n");

    SERIAL_PRINT("--- DONE ---\n");
}

void _kmain(void)
{
    init_mem();
    fs_init();
    configure_interrupts();
    init_tss();
    init_scheduling();

    clear_terminal();
    run_fs_tests();
    add_task(shell, "");

    while (1) {
        asm("hlt");
    }
}
