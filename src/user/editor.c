#include "user_lib.h"
#include "user_progs.h"
#include "utils.h"

#define EDITOR_LINES 24

void editor() {
    char (*lines)[80] = (char (*)[80])alloc_page();
    for (int i = 0; i < EDITOR_LINES; i++) {
        uint_to_ascii(i + 1, lines[i]);
    }

    for (int i = 0; i < EDITOR_LINES; i++) {
        user_clear_line(i);
        user_print_line(lines[i], i);
    }

    while (1) {
        char* input = get(24);

        if (input[0] == 'e' && input[1] == '\0') break;

        uint32_t selected = str_to_uint(input);
        if (selected < 1 || selected > EDITOR_LINES) continue;

        int sel_idx = (int)(selected - 1);

        user_clear_line(sel_idx);
        char* new_content = get_prefilled(sel_idx, lines[sel_idx]);

        int j = 0;
        while (new_content[j] && j < 79) {
            lines[sel_idx][j] = new_content[j];
            j++;
        }
        lines[sel_idx][j] = '\0';

        user_clear_line(sel_idx);
        user_print_line(lines[sel_idx], sel_idx);
    }

    user_clear_terminal();
    shell_resume_line = shell_start_line;
    kill_process(get_pid());
    while (1) {}
}
