#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

#define MAX_CHILDREN  16
#define MAX_PARAMS     8
#define MAX_FUNCTIONS 16
#define MAX_NODES    256

typedef struct Node {
    Token token;
    struct Node* children[MAX_CHILDREN];
    int child_count;
} Node;

typedef struct {
    Token name;
    Token parameters[MAX_PARAMS];
    int param_count;
    Node* body;
} FunctionNode;

typedef struct {
    Node nodes[MAX_NODES];
    int count;
} NodePool;

typedef struct {
    FunctionNode functions[MAX_FUNCTIONS];
    int count;
} FunctionPool;

typedef struct {
    Token* tokens;
    int token_count;
    int cur;
    int error;
    char error_msg[64];
    NodePool*     node_pool;
    FunctionPool* func_pool;
} Parser;

void parser_init(Parser* p, Token* tokens, int token_count,
                 NodePool* node_pool, FunctionPool* func_pool);

Node* parse_factor(Parser* p);
Node* parse_unary(Parser* p);
Node* parse_term(Parser* p);
Node* parse_expression(Parser* p);
Node* parse_comparison(Parser* p);
Node* parse_assignment(Parser* p);
Node* parse_braces(Parser* p);
Node* parse_if_statement(Parser* p);
Node* parse_while_statement(Parser* p);
Node* parse_return_statement(Parser* p);
Node* parse_statement(Parser* p);
int   parse_function(Parser* p);
int   parse_program(Parser* p);

#endif  // PARSER_H
