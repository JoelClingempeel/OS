#include "user_lib.h"
#include "user_progs.h"
#include "utils.h"

int shell_resume_line = 0;
int shell_start_line = 0;


void blinky() {
    uint8_t bg_bytes = TEXT_FORMAT_BYTE & 0xF0;
    while (1) {
        delay(50);
        put_char('N', 0x0c | bg_bytes, 0x350);  // Red
        delay(50);
        put_char(0, 0x0c | bg_bytes, 0x350);
    }
}

void blinky2() {
    uint8_t bg_bytes = TEXT_FORMAT_BYTE & 0xF0;
    while (1) {
        delay(50);
        put_char('Q', 0x0b | bg_bytes, 0x700);  // Cyan
        delay(50);
        put_char(0, 0x0b | bg_bytes, 0x700);
    }
}

void blinky3() {
    uint8_t bg_bytes = TEXT_FORMAT_BYTE & 0xF0;
    while (1) {
        delay(50);
        put_char('Y', 0x0a | bg_bytes, 0x100);  // Green
        delay(50);
        put_char(0, 0x0a | bg_bytes, 0x100);
    }
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
            char segment[81];
            int i = 0, j = 0;
            while (buf[i]) {
                if (buf[i] == '\n') {
                    segment[j] = '\0';
                    user_print_line(segment, line++);
                    j = 0;
                } else if (j < 80) {
                    segment[j++] = buf[i];
                }
                i++;
            }
            if (j > 0) {
                segment[j] = '\0';
                user_print_line(segment, line++);
            }
        }
    }

    shell_resume_line = line;
    kill_process(get_pid());
    while (1);
}

void prog_move() {
    char* args = get_args();
    int line = shell_start_line;

    char src[256], dst[256];
    int i = 0;
    while (*args && *args != ' ' && i < 255) src[i++] = *args++;
    src[i] = '\0';
    if (*args == ' ') args++;
    i = 0;
    while (*args && i < 255) dst[i++] = *args++;
    dst[i] = '\0';

    if (!src[0] || !dst[0]) {
        char err[] = "Usage: move <src> <dst>";
        user_print_line(err, line++);
    } else {
        int r = fs_rename(src, dst);
        if (r == 0) {
            char done[] = "Moved.";
            user_print_line(done, line++);
        } else {
            char fail[] = "Error: move failed.";
            user_print_line(fail, line++);
        }
    }

    shell_resume_line = line;
    kill_process(get_pid());
    while (1);
}

void prog_copy() {
    char* args = get_args();
    int line = shell_start_line;

    char src[256], dst[256];
    int i = 0;
    while (*args && *args != ' ' && i < 255) src[i++] = *args++;
    src[i] = '\0';
    if (*args == ' ') args++;
    i = 0;
    while (*args && i < 255) dst[i++] = *args++;
    dst[i] = '\0';

    if (!src[0] || !dst[0]) {
        char err[] = "Usage: copy <src> <dst>";
        user_print_line(err, line++);
    } else {
        int r = fs_copy(src, dst);
        if (r == 0) {
            char done[] = "Copied.";
            user_print_line(done, line++);
        } else {
            char fail[] = "Error: copy failed.";
            user_print_line(fail, line++);
        }
    }

    shell_resume_line = line;
    kill_process(get_pid());
    while (1);
}
