#include "parser.h"


// --- internal helpers ---

static Token parser_peek(Parser* p) {
    return p->tokens[p->cur];
}

static Token parser_previous(Parser* p) {
    return p->tokens[p->cur - 1];
}

static Token parser_advance(Parser* p) {
    p->cur++;
    return p->tokens[p->cur - 1];
}

static int parser_end_reached(Parser* p) {
    return p->cur >= p->token_count;
}

static int parser_match(Parser* p, TokenType type) {
    if (!parser_end_reached(p) && parser_peek(p).type == type) {
        parser_advance(p);
        return 1;
    }
    return 0;
}

static void parser_set_error(Parser* p, const char* msg) {
    p->error = 1;
    int i = 0;
    while (msg[i] && i < 63) { p->error_msg[i] = msg[i]; i++; }
    p->error_msg[i] = '\0';
}

static void parser_consume(Parser* p, TokenType type) {
    if (p->error) return;
    Token token = parser_advance(p);
    if (token.type != type) {
        const char* prefix   = "Missing ";
        const char* type_str = token_type_to_string(type);
        char msg[64];
        int i = 0;
        while (prefix[i]   && i < 32) { msg[i] = prefix[i]; i++; }
        int j = 0;
        while (type_str[j] && i < 63) { msg[i++] = type_str[j++]; }
        msg[i] = '\0';
        parser_set_error(p, msg);
    }
}

static Node* node_alloc(Parser* p) {
    if (p->node_pool->count >= MAX_NODES) {
        parser_set_error(p, "Node pool exhausted");
        return 0;
    }
    Node* n = &p->node_pool->nodes[p->node_pool->count++];
    n->child_count = 0;
    return n;
}

static void node_add_child(Parser* p, Node* parent, Node* child) {
    if (parent->child_count >= MAX_CHILDREN) {
        parser_set_error(p, "Too many children");
        return;
    }
    parent->children[parent->child_count++] = child;
}


// --- public ---

void parser_init(Parser* p, Token* tokens, int token_count,
                 NodePool* node_pool, FunctionPool* func_pool) {
    p->tokens      = tokens;
    p->token_count = token_count;
    p->cur         = 0;
    p->error       = 0;
    p->error_msg[0] = '\0';
    p->node_pool   = node_pool;
    p->func_pool   = func_pool;
    node_pool->count = 0;
    func_pool->count = 0;
}

Node* parse_factor(Parser* p) {
    if (p->error) return 0;

    if (parser_match(p, TOKEN_NUMBER)) {
        Node* n = node_alloc(p);
        if (!n) return 0;
        n->token = parser_previous(p);
        return n;
    }

    if (parser_match(p, TOKEN_IDENTIFIER)) {
        Token id = parser_previous(p);
        if (parser_match(p, TOKEN_LEFT_PAREN)) {
            Node* n = node_alloc(p);
            if (!n) return 0;
            n->token = id;
            while (!p->error && !parser_end_reached(p) &&
                   parser_peek(p).type != TOKEN_RIGHT_PAREN) {
                Node* arg = parse_expression(p);
                if (!arg) return 0;
                node_add_child(p, n, arg);
                if (parser_peek(p).type != TOKEN_RIGHT_PAREN)
                    parser_consume(p, TOKEN_COMMA);
            }
            parser_consume(p, TOKEN_RIGHT_PAREN);
            return p->error ? 0 : n;
        }
        Node* n = node_alloc(p);
        if (!n) return 0;
        n->token = id;
        return n;
    }

    if (parser_match(p, TOKEN_LEFT_PAREN)) {
        Node* n = parse_expression(p);
        parser_consume(p, TOKEN_RIGHT_PAREN);
        return p->error ? 0 : n;
    }

    parser_set_error(p, "Missing number or identifier");
    return 0;
}

Node* parse_unary(Parser* p) {
    if (p->error) return 0;
    if (parser_match(p, TOKEN_SUBTRACT)) {
        Token op = parser_previous(p);
        Node* n = node_alloc(p);
        if (!n) return 0;
        n->token = op;
        Node* right = parse_factor(p);
        if (!right) return 0;
        node_add_child(p, n, right);
        return p->error ? 0 : n;
    }
    return parse_factor(p);
}

Node* parse_term(Parser* p) {
    if (p->error) return 0;
    Node* left = parse_unary(p);
    if (!left) return 0;
    while (!p->error &&
           (parser_match(p, TOKEN_MULTIPLY) || parser_match(p, TOKEN_DIVIDE))) {
        Token op    = parser_previous(p);
        Node* right = parse_unary(p);
        if (!right) return 0;
        Node* n = node_alloc(p);
        if (!n) return 0;
        n->token = op;
        node_add_child(p, n, left);
        node_add_child(p, n, right);
        left = n;
    }
    return p->error ? 0 : left;
}

Node* parse_expression(Parser* p) {
    if (p->error) return 0;
    Node* left = parse_term(p);
    if (!left) return 0;
    while (!p->error &&
           (parser_match(p, TOKEN_ADD) || parser_match(p, TOKEN_SUBTRACT))) {
        Token op    = parser_previous(p);
        Node* right = parse_term(p);
        if (!right) return 0;
        Node* n = node_alloc(p);
        if (!n) return 0;
        n->token = op;
        node_add_child(p, n, left);
        node_add_child(p, n, right);
        left = n;
    }
    return p->error ? 0 : left;
}

Node* parse_comparison(Parser* p) {
    if (p->error) return 0;
    Node* left = parse_expression(p);
    if (!left) return 0;
    if (!p->error && (
            parser_match(p, TOKEN_LESS)          ||
            parser_match(p, TOKEN_LESS_EQUALS)   ||
            parser_match(p, TOKEN_GREATER)       ||
            parser_match(p, TOKEN_GREATER_EQUALS)||
            parser_match(p, TOKEN_DOUBLE_EQUALS) ||
            parser_match(p, TOKEN_NOT_EQUALS))) {
        Token op    = parser_previous(p);
        Node* right = parse_expression(p);
        if (!right) return 0;
        Node* n = node_alloc(p);
        if (!n) return 0;
        n->token = op;
        node_add_child(p, n, left);
        node_add_child(p, n, right);
        return p->error ? 0 : n;
    }
    return left;
}

Node* parse_assignment(Parser* p) {
    if (p->error) return 0;
    Node* left = parse_comparison(p);
    if (!left) return 0;
    if (!p->error && parser_match(p, TOKEN_EQUALS)) {
        if (left->token.type != TOKEN_IDENTIFIER) {
            parser_set_error(p, "Invalid assignment");
            return 0;
        }
        Token op    = parser_previous(p);
        Node* right = parse_comparison(p);
        if (!right) return 0;
        Node* n = node_alloc(p);
        if (!n) return 0;
        n->token = op;
        node_add_child(p, n, left);
        node_add_child(p, n, right);
        return p->error ? 0 : n;
    }
    return left;
}

Node* parse_braces(Parser* p) {
    if (p->error) return 0;
    parser_consume(p, TOKEN_LEFT_BRACE);
    if (p->error) return 0;
    Node* n = node_alloc(p);
    if (!n) return 0;
    StringView sv = {"{", 1};
    n->token = (Token){sv, TOKEN_LEFT_BRACE};
    while (!p->error && !parser_end_reached(p) &&
           parser_peek(p).type != TOKEN_RIGHT_BRACE) {
        Node* stmt = parse_statement(p);
        if (!stmt) return 0;
        node_add_child(p, n, stmt);
    }
    parser_consume(p, TOKEN_RIGHT_BRACE);
    return p->error ? 0 : n;
}

Node* parse_if_statement(Parser* p) {
    if (p->error) return 0;
    parser_match(p, TOKEN_IF);
    Token token = parser_previous(p);
    parser_consume(p, TOKEN_LEFT_PAREN);
    Node* cond = parse_comparison(p);
    if (!cond) return 0;
    parser_consume(p, TOKEN_RIGHT_PAREN);
    Node* body = parse_braces(p);
    if (!body) return 0;
    Node* n = node_alloc(p);
    if (!n) return 0;
    n->token = token;
    node_add_child(p, n, cond);
    node_add_child(p, n, body);
    return p->error ? 0 : n;
}

Node* parse_while_statement(Parser* p) {
    if (p->error) return 0;
    parser_match(p, TOKEN_WHILE);
    Token token = parser_previous(p);
    parser_consume(p, TOKEN_LEFT_PAREN);
    Node* cond = parse_comparison(p);
    if (!cond) return 0;
    parser_consume(p, TOKEN_RIGHT_PAREN);
    Node* body = parse_braces(p);
    if (!body) return 0;
    Node* n = node_alloc(p);
    if (!n) return 0;
    n->token = token;
    node_add_child(p, n, cond);
    node_add_child(p, n, body);
    return p->error ? 0 : n;
}

Node* parse_return_statement(Parser* p) {
    if (p->error) return 0;
    parser_match(p, TOKEN_RETURN);
    Token token = parser_previous(p);
    Node* child = parse_comparison(p);
    if (!child) return 0;
    parser_consume(p, TOKEN_SEMICOLON);
    Node* n = node_alloc(p);
    if (!n) return 0;
    n->token = token;
    node_add_child(p, n, child);
    return p->error ? 0 : n;
}

Node* parse_statement(Parser* p) {
    if (p->error) return 0;
    TokenType type = parser_peek(p).type;
    if (type == TOKEN_IF)          return parse_if_statement(p);
    if (type == TOKEN_WHILE)       return parse_while_statement(p);
    if (type == TOKEN_RETURN)      return parse_return_statement(p);
    if (type == TOKEN_LEFT_BRACE)  return parse_braces(p);
    Node* out = parse_assignment(p);
    if (!out) return 0;
    parser_consume(p, TOKEN_SEMICOLON);
    return p->error ? 0 : out;
}

int parse_function(Parser* p) {
    if (p->error) return 0;
    if (p->func_pool->count >= MAX_FUNCTIONS) {
        parser_set_error(p, "Too many functions");
        return 0;
    }
    parser_consume(p, TOKEN_FUN);
    parser_consume(p, TOKEN_IDENTIFIER);
    if (p->error) return 0;

    Token name = parser_previous(p);
    Token params[MAX_PARAMS];
    int param_count = 0;

    parser_consume(p, TOKEN_LEFT_PAREN);
    while (!p->error && !parser_end_reached(p) &&
           parser_peek(p).type != TOKEN_RIGHT_PAREN) {
        parser_consume(p, TOKEN_IDENTIFIER);
        if (p->error) return 0;
        if (param_count < MAX_PARAMS)
            params[param_count++] = parser_previous(p);
        if (parser_peek(p).type != TOKEN_RIGHT_PAREN)
            parser_consume(p, TOKEN_COMMA);
    }
    parser_consume(p, TOKEN_RIGHT_PAREN);
    if (p->error) return 0;

    Node* body = parse_braces(p);
    if (p->error || !body) return 0;

    FunctionNode* fn = &p->func_pool->functions[p->func_pool->count++];
    fn->name = name;
    fn->param_count = param_count;
    for (int i = 0; i < param_count; i++)
        fn->parameters[i] = params[i];
    fn->body = body;
    return 1;
}

int parse_program(Parser* p) {
    while (!p->error && !parser_end_reached(p))
        parse_function(p);
    return p->func_pool->count;
}
