#include "lexer.h"
#include "parser.h"
#include "user_lib.h"
#include "user_progs.h"
#include "utils.h"

// Lexer  :  2 pages (6164 bytes)
// NodePool:  5 pages (20480 bytes, MAX_CHILDREN=16, MAX_NODES=256)
// FuncPool:  1 page  (1856 bytes)
// Source  :  1 page  (4096 bytes)

static int print_node(Node* n, int depth, int line) {
    if (!n) return line;
    char buf[80];
    int pos = 0;
    for (int i = 0; i < depth * 2 && pos < 60; i++) buf[pos++] = ' ';
    for (int i = 0; i < n->token.lexeme.len && pos < 70; i++)
        buf[pos++] = n->token.lexeme.ptr[i];
    buf[pos++] = ' ';
    const char* ts = token_type_to_string(n->token.type);
    for (int i = 0; ts[i] && pos < 79; i++) buf[pos++] = ts[i];
    buf[pos] = '\0';
    user_print_line(buf, line++);
    for (int i = 0; i < n->child_count; i++)
        line = print_node(n->children[i], depth + 1, line);
    return line;
}

static int print_function_header(FunctionNode* fn, int line) {
    char buf[80];
    int pos = 0;
    buf[pos++] = 'f'; buf[pos++] = 'u'; buf[pos++] = 'n'; buf[pos++] = ' ';
    for (int i = 0; i < fn->name.lexeme.len && pos < 40; i++)
        buf[pos++] = fn->name.lexeme.ptr[i];
    buf[pos++] = '(';
    for (int i = 0; i < fn->param_count; i++) {
        for (int j = 0; j < fn->parameters[i].lexeme.len && pos < 75; j++)
            buf[pos++] = fn->parameters[i].lexeme.ptr[j];
        if (i + 1 < fn->param_count) buf[pos++] = ',';
    }
    buf[pos++] = ')';
    buf[pos] = '\0';
    user_print_line(buf, line++);
    return line;
}

void interp() {
    int line = shell_start_line;
    char* path = get_args();

    if (!path || !path[0]) {
        char err[] = "Usage: interp <file>";
        user_print_line(err, line++);
        shell_resume_line = line;
        kill_process(get_pid());
        while (1) {}
    }

    char* source = (char*)alloc_page();

    if (fs_read(path, source) != 0) {
        char err[] = "Error: file not found.";
        user_print_line(err, line++);
        shell_resume_line = line;
        kill_process(get_pid());
        while (1) {}
    }

    int source_len = (int)strlen(source);

    Lexer* lexer = (Lexer*)alloc_page();
    alloc_page();
    lexer_init(lexer, source, source_len);
    lexer_tokenize(lexer);

    // Uncomment to debug tokens:
    // for (int i = 0; i < lexer->token_count; i++) {
    //     char buf[80];
    //     int pos = 0;
    //     const char* ts = token_type_to_string(lexer->tokens[i].type);
    //     for (int j = 0; ts[j] && pos < 40; j++) buf[pos++] = ts[j];
    //     buf[pos++] = ' ';
    //     for (int j = 0; j < lexer->tokens[i].lexeme.len && pos < 79; j++)
    //         buf[pos++] = lexer->tokens[i].lexeme.ptr[j];
    //     buf[pos] = '\0';
    //     user_print_line(buf, line++);
    // }

    NodePool* node_pool = (NodePool*)alloc_page();
    alloc_page(); alloc_page(); alloc_page(); alloc_page();
    FunctionPool* func_pool = (FunctionPool*)alloc_page();

    Parser parser;
    parser_init(&parser, lexer->tokens, lexer->token_count, node_pool, func_pool);
    int num_functions = parse_program(&parser);

    if (parser.error) {
        user_print_line(parser.error_msg, line++);
        shell_resume_line = line;
        kill_process(get_pid());
        while (1) {}
    }

    for (int i = 0; i < num_functions; i++) {
        FunctionNode* fn = &func_pool->functions[i];
        line = print_function_header(fn, line);
        line = print_node(fn->body, 1, line);
    }

    shell_resume_line = line;
    kill_process(get_pid());
    while (1) {}
}
