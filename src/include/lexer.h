#ifndef LEXER_H
#define LEXER_H

#define MAX_TOKENS 512

typedef enum {
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_ADD,
    TOKEN_SUBTRACT,
    TOKEN_MULTIPLY,
    TOKEN_DIVIDE,
    TOKEN_SEMICOLON,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE,
    TOKEN_EQUALS,
    TOKEN_LESS,
    TOKEN_LESS_EQUALS,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUALS,
    TOKEN_DOUBLE_EQUALS,
    TOKEN_NOT_EQUALS,
    TOKEN_FUN,
    TOKEN_COMMA,
    TOKEN_RETURN,
    TOKEN_STRING,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT
} TokenType;

// Non-owning view into the source buffer.
typedef struct {
    const char* ptr;
    int len;
} StringView;

typedef struct {
    StringView lexeme;
    TokenType type;
} Token;

typedef struct {
    const char* source;
    int source_len;
    int start;
    int cur;
    Token tokens[MAX_TOKENS];
    int token_count;
} Lexer;

const char* token_type_to_string(TokenType type);

void lexer_init(Lexer* l, const char* source, int source_len);
int  lexer_end_reached(Lexer* l);
void lexer_get_token(Lexer* l);
void lexer_tokenize(Lexer* l);

#endif  // LEXER_H
