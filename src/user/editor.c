#include "user_lib.h"
#include "user_progs.h"
#include "utils.h"

#define EDITOR_LINES 48
#define VIEW_LINES   24
#define LINE_WIDTH   76
#define SCROLL_STEP   4

static void display_line(int screen_row, int actual_idx, char lines[][80]) {
    char display[80];
    int num = actual_idx + 1;
    int pos = 0;
    if (num < 10) display[pos++] = ' ';
    char num_str[4];
    uint_to_ascii((uint32_t)num, num_str);
    for (int i = 0; num_str[i]; i++) display[pos++] = num_str[i];
    display[pos++] = ' ';
    for (int j = 0; lines[actual_idx][j] && pos < 79; j++)
        display[pos++] = lines[actual_idx][j];
    display[pos] = '\0';
    user_print_line(display, screen_row);
}

static void redisplay(int scroll_offset, char lines[][80]) {
    for (int i = 0; i < VIEW_LINES; i++) {
        user_clear_line(i);
        display_line(i, scroll_offset + i, lines);
    }
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
            if (line_idx < EDITOR_LINES)
                lines[line_idx][col++] = buf[i];
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

    int scroll_offset = 0;
    redisplay(scroll_offset, lines);

    while (1) {
        char* input = get(VIEW_LINES);

        if (input[0] == 'e' && input[1] == '\0') break;

        if (input[0] == 's' && input[1] == '\0') {
            serialize_lines(lines, file_buf);
            fs_write(path, file_buf);
            continue;
        }

        if (input[0] == 'u' && input[1] == '\0') {
            if (scroll_offset > 0) {
                scroll_offset -= SCROLL_STEP;
                if (scroll_offset < 0) scroll_offset = 0;
                redisplay(scroll_offset, lines);
            }
            continue;
        }

        if (input[0] == 'd' && input[1] == '\0') {
            if (scroll_offset < EDITOR_LINES - VIEW_LINES) {
                scroll_offset += SCROLL_STEP;
                if (scroll_offset > EDITOR_LINES - VIEW_LINES)
                    scroll_offset = EDITOR_LINES - VIEW_LINES;
                redisplay(scroll_offset, lines);
            }
            continue;
        }

        uint32_t selected = str_to_uint(input);
        if (selected < (uint32_t)(scroll_offset + 1) ||
            selected > (uint32_t)(scroll_offset + VIEW_LINES)) continue;

        int actual_idx  = (int)(selected - 1);
        int screen_row  = actual_idx - scroll_offset;

        user_clear_line(screen_row);
        char* new_content = get_prefilled(screen_row, lines[actual_idx]);

        int j = 0;
        while (new_content[j] && j < LINE_WIDTH) {
            lines[actual_idx][j] = new_content[j];
            j++;
        }
        lines[actual_idx][j] = '\0';

        user_clear_line(screen_row);
        display_line(screen_row, actual_idx, lines);
    }

    user_clear_terminal();
    shell_resume_line = shell_start_line;
    kill_process(get_pid());
    while (1) {}
}
