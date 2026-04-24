#include "lexer.h"
#include "parser.h"
#include "user_lib.h"
#include "user_progs.h"
#include "utils.h"

// Lexer  :  2 pages (6164 bytes)
// NodePool:  6 pages (24576 bytes, MAX_CHILDREN=16, MAX_NODES=256) -- 5 pages is exactly 20480 bytes which leaves count field off the end
// FuncPool:  1 page  (1856 bytes)
// Source  :  1 page  (4096 bytes)

#define MAX_VAR_NAME 32
#define MAX_VARS     30

typedef struct {
    char name[MAX_VAR_NAME];
    int  value;
} Var;

static Var env[MAX_VARS];
static int env_count;
static int returning;
static int return_value;
static FunctionPool* func_pool;
static int term_line;

static int sv_eq(StringView sv, const char* s) {
    int i = 0;
    while (i < sv.len && s[i]) {
        if (sv.ptr[i] != s[i]) return 0;
        i++;
    }
    return i == sv.len && !s[i];
}

static int sv_sv_eq(StringView a, StringView b) {
    if (a.len != b.len) return 0;
    for (int i = 0; i < a.len; i++)
        if (a.ptr[i] != b.ptr[i]) return 0;
    return 1;
}

static void sv_copy(char* dst, StringView sv) {
    int n = sv.len < MAX_VAR_NAME - 1 ? sv.len : MAX_VAR_NAME - 1;
    for (int i = 0; i < n; i++) dst[i] = sv.ptr[i];
    dst[n] = '\0';
}

static int parse_int(StringView sv) {
    int result = 0;
    for (int i = 0; i < sv.len; i++)
        result = result * 10 + (sv.ptr[i] - '0');
    return result;
}

static int env_get(StringView name) {
    for (int i = 0; i < env_count; i++)
        if (sv_eq(name, env[i].name)) return env[i].value;
    // TODO: error on undefined variable
    return 0;
}

static void env_set(StringView name, int value) {
    for (int i = 0; i < env_count; i++) {
        if (sv_eq(name, env[i].name)) { env[i].value = value; return; }
    }
    if (env_count < MAX_VARS) {
        sv_copy(env[env_count].name, name);
        env[env_count].value = value;
        env_count++;
    }
}

static int eval_node(Node* n);

static int eval_braces(Node* n) {
    int result = 0;
    for (int i = 0; i < n->child_count; i++) {
        result = eval_node(n->children[i]);
        if (returning) return result;
    }
    return result;
}

static int eval_node(Node* n) {
    if (!n) return 0;
    switch (n->token.type) {
        case TOKEN_NUMBER:
            return parse_int(n->token.lexeme);

        case TOKEN_IDENTIFIER: {
            // Function call: find callee in func_pool.
            // Must check before env_get: a zero-arg call has child_count==0,
            // identical to a variable reference, so we use func_pool as the discriminator.
            FunctionNode* callee = 0;
            for (int i = 0; i < func_pool->count; i++) {
                if (sv_sv_eq(func_pool->functions[i].name.lexeme, n->token.lexeme)) {
                    callee = &func_pool->functions[i];
                    break;
                }
            }

            if (!callee) {
                char s_print[] = {'p','r','i','n','t',0};
                char s_input[] = {'i','n','p','u','t',0};

                if (sv_eq(n->token.lexeme, s_print)) {
                    if (n->child_count > 0 && n->children[0]->token.type == TOKEN_STRING) {
                        StringView sv = n->children[0]->token.lexeme;
                        char buf[128];
                        int len = sv.len - 2;
                        if (len > 127) len = 127;
                        if (len < 0) len = 0;
                        for (int i = 0; i < len; i++) buf[i] = sv.ptr[i + 1];
                        buf[len] = '\0';
                        user_print_line(buf, term_line++);
                    }
                    return 0;
                }

                if (sv_eq(n->token.lexeme, s_input)) {
                    while (1) {
                        char* raw = get(term_line++);
                        int i = 0;
                        if (raw[i] == '-') i++;
                        int start = i;
                        int valid = raw[i] != '\0';
                        while (raw[i]) {
                            if (raw[i] < '0' || raw[i] > '9') { valid = 0; break; }
                            i++;
                        }
                        if (valid) {
                            int neg = (raw[0] == '-');
                            int result = 0;
                            for (int j = start; raw[j]; j++)
                                result = result * 10 + (raw[j] - '0');
                            return neg ? -result : result;
                        }
                        char err[] = {'E','n','t','e','r',' ','a',' ','n','u','m','b','e','r',0};
                        user_print_line(err, term_line++);
                    }
                }

                return env_get(n->token.lexeme);
            }

            // Evaluate arguments before modifying env.
            int args[MAX_PARAMS];
            for (int i = 0; i < n->child_count && i < MAX_PARAMS; i++)
                args[i] = eval_node(n->children[i]);

            // Save caller scope, push parameters as new variables.
            int saved_count = env_count;
            for (int i = 0; i < callee->param_count; i++) {
                // TODO: error on argument count mismatch
                int val = i < n->child_count ? args[i] : 0;
                env_set(callee->parameters[i].lexeme, val);
            }

            int saved_return = return_value;
            returning = 0;
            eval_node(callee->body);
            int result = return_value;

            // Restore caller scope and return state.
            env_count = saved_count;
            return_value = saved_return;
            returning = 0;

            return result;
        }

        case TOKEN_LEFT_BRACE:
            return eval_braces(n);

        case TOKEN_EQUALS: {
            int val = eval_node(n->children[1]);
            env_set(n->children[0]->token.lexeme, val);
            return val;
        }

        case TOKEN_ADD:
            return eval_node(n->children[0]) + eval_node(n->children[1]);

        case TOKEN_SUBTRACT:
            // child_count == 1: unary negation; child_count == 2: binary subtraction
            if (n->child_count == 1) return -eval_node(n->children[0]);
            return eval_node(n->children[0]) - eval_node(n->children[1]);

        case TOKEN_MULTIPLY:
            return eval_node(n->children[0]) * eval_node(n->children[1]);

        case TOKEN_DIVIDE:
            return eval_node(n->children[0]) / eval_node(n->children[1]);

        case TOKEN_LESS:
            return eval_node(n->children[0]) < eval_node(n->children[1]);
        case TOKEN_LESS_EQUALS:
            return eval_node(n->children[0]) <= eval_node(n->children[1]);
        case TOKEN_GREATER:
            return eval_node(n->children[0]) > eval_node(n->children[1]);
        case TOKEN_GREATER_EQUALS:
            return eval_node(n->children[0]) >= eval_node(n->children[1]);
        case TOKEN_DOUBLE_EQUALS:
            return eval_node(n->children[0]) == eval_node(n->children[1]);
        case TOKEN_NOT_EQUALS:
            return eval_node(n->children[0]) != eval_node(n->children[1]);

        case TOKEN_IF: {
            if (eval_node(n->children[0]))
                eval_node(n->children[1]);
            else if (n->child_count > 2)
                eval_node(n->children[2]);
            return 0;
        }

        case TOKEN_WHILE: {
            while (!returning && eval_node(n->children[0]))
                eval_node(n->children[1]);
            return 0;
        }

        case TOKEN_RETURN: {
            return_value = eval_node(n->children[0]);
            returning = 1;
            return return_value;
        }

        default:
            return 0;
    }
}

void interp() {
    int line = shell_start_line;
    char* path = get_args();

    if (!path || !path[0]) {
        char err[] = {'U','s','a','g','e',':',' ','i','n','t','e','r','p',' ','<','f','i','l','e','>',0};
        user_print_line(err, line++);
        shell_resume_line = line;
        kill_process(get_pid());
        while (1) {}
    }

    char* source = (char*)alloc_page();
    memset(source, 0, 4096);

    if (fs_read(path, source) != 0) {
        char err[] = {'E','r','r','o','r',':',' ','f','i','l','e',' ','n','o','t',' ','f','o','u','n','d','.',0};
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

    NodePool* node_pool = (NodePool*)alloc_page();
    alloc_page(); alloc_page(); alloc_page(); alloc_page(); alloc_page();
    func_pool = (FunctionPool*)alloc_page();

    Parser parser;
    parser_init(&parser, lexer->tokens, lexer->token_count, node_pool, func_pool);
    int num_functions = parse_program(&parser);

    if (parser.error) {
        user_print_line(parser.error_msg, line++);
        shell_resume_line = line;
        kill_process(get_pid());
        while (1) {}
    }

    char main_name[] = {'m','a','i','n',0};
    FunctionNode* main_fn = 0;
    for (int i = 0; i < num_functions; i++) {
        if (sv_eq(func_pool->functions[i].name.lexeme, main_name)) {
            main_fn = &func_pool->functions[i];
            break;
        }
    }

    if (!main_fn) {
        char err[] = {'E','r','r','o','r',':',' ','n','o',' ','m','a','i','n',0};
        user_print_line(err, line++);
        shell_resume_line = line;
        kill_process(get_pid());
        while (1) {}
    }

    env_count = 0;
    returning = 0;
    return_value = 0;
    term_line = line;
    eval_node(main_fn->body);
    line = term_line;
    int result = return_value;

    char buf[32];
    char prefix[] = {'r','e','s','u','l','t',':',' ',0};
    int pos = 0;
    for (int i = 0; prefix[i]; i++) buf[pos++] = prefix[i];
    if (result < 0) {
        buf[pos++] = '-';
        uint_to_ascii((uint32_t)(-result), buf + pos);
    } else {
        uint_to_ascii((uint32_t)result, buf + pos);
    }
    user_print_line(buf, line++);

    shell_resume_line = line;
    kill_process(get_pid());
    while (1) {}
}
