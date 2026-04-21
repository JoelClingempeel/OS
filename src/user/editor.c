#include "user_lib.h"
#include "user_progs.h"
#include "utils.h"

#define EDITOR_LINES 24
// Max content chars per line: 80 columns minus the 4-char "os% " prompt.
#define LINE_WIDTH 76

static void display_line(int idx, char lines[][80]) {
    char display[80];
    int num = idx + 1;
    int pos = 0;
    if (num < 10) display[pos++] = ' ';
    char num_str[4];
    uint_to_ascii((uint32_t)num, num_str);
    for (int i = 0; num_str[i]; i++) display[pos++] = num_str[i];
    display[pos++] = ' ';
    for (int j = 0; lines[idx][j] && pos < 79; j++)
        display[pos++] = lines[idx][j];
    display[pos] = '\0';
    user_print_line(display, idx);
}

static void parse_into_lines(char* buf, char lines[][80]) {
    int line_idx = 0;
    int col = 0;
    for (int i = 0; buf[i] && line_idx < EDITOR_LINES; i++) {
        if (buf[i] == '\n') {
            lines[line_idx][col] = '\0';
            line_idx++;
            col = 0;
        } else if (col >= LINE_WIDTH) {
            lines[line_idx][col] = '\0';
            line_idx++;
            col = 0;
            if (line_idx < EDITOR_LINES) {
                lines[line_idx][col++] = buf[i];
            }
        } else {
            lines[line_idx][col++] = buf[i];
        }
    }
    if (line_idx < EDITOR_LINES)
        lines[line_idx][col] = '\0';
}

static void serialize_lines(char lines[][80], char* buf) {
    int last = -1;
    for (int i = 0; i < EDITOR_LINES; i++)
        if (lines[i][0]) last = i;

    int pos = 0;
    for (int i = 0; i <= last; i++) {
        for (int j = 0; lines[i][j]; j++)
            buf[pos++] = lines[i][j];
        if (i < last)
            buf[pos++] = '\n';
    }
    buf[pos] = '\0';
}

void editor() {
    char* path = get_args();

    char (*lines)[80] = (char (*)[80])alloc_page();
    char* file_buf    = (char*)alloc_page();

    for (int i = 0; i < EDITOR_LINES; i++)
        lines[i][0] = '\0';

    if (path && path[0] && fs_read(path, file_buf) == 0)
        parse_into_lines(file_buf, lines);

    for (int i = 0; i < EDITOR_LINES; i++) {
        user_clear_line(i);
        display_line(i, lines);
    }

    while (1) {
        char* input = get(24);

        if (input[0] == 'e' && input[1] == '\0') break;

        if (input[0] == 's' && input[1] == '\0') {
            serialize_lines(lines, file_buf);
            fs_write(path, file_buf);
            continue;
        }

        uint32_t selected = str_to_uint(input);
        if (selected < 1 || selected > EDITOR_LINES) continue;

        int sel_idx = (int)(selected - 1);

        user_clear_line(sel_idx);
        char* new_content = get_prefilled(sel_idx, lines[sel_idx]);

        int j = 0;
        while (new_content[j] && j < LINE_WIDTH) {
            lines[sel_idx][j] = new_content[j];
            j++;
        }
        lines[sel_idx][j] = '\0';

        user_clear_line(sel_idx);
        display_line(sel_idx, lines);
    }

    user_clear_terminal();
    shell_resume_line = shell_start_line;
    kill_process(get_pid());
    while (1) {}
}
