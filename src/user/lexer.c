#include "lexer.h"


const char* token_type_to_string(TokenType type) {
    switch (type) {
        case TOKEN_IF:             return "IF";
        case TOKEN_ELSE:           return "ELSE";
        case TOKEN_WHILE:          return "WHILE";
        case TOKEN_IDENTIFIER:     return "IDENTIFIER";
        case TOKEN_NUMBER:         return "NUMBER";
        case TOKEN_ADD:            return "ADD";
        case TOKEN_SUBTRACT:       return "SUBTRACT";
        case TOKEN_MULTIPLY:       return "MULTIPLY";
        case TOKEN_DIVIDE:         return "DIVIDE";
        case TOKEN_SEMICOLON:      return "SEMICOLON";
        case TOKEN_LEFT_PAREN:     return "LEFT_PAREN";
        case TOKEN_RIGHT_PAREN:    return "RIGHT_PAREN";
        case TOKEN_LEFT_BRACE:     return "LEFT_BRACE";
        case TOKEN_RIGHT_BRACE:    return "RIGHT_BRACE";
        case TOKEN_EQUALS:         return "EQUALS";
        case TOKEN_LESS:           return "LESS";
        case TOKEN_LESS_EQUALS:    return "LESS_EQUALS";
        case TOKEN_GREATER:        return "GREATER";
        case TOKEN_GREATER_EQUALS: return "GREATER_EQUALS";
        case TOKEN_DOUBLE_EQUALS:  return "DOUBLE_EQUALS";
        case TOKEN_NOT_EQUALS:     return "NOT_EQUALS";
        case TOKEN_FUN:            return "FUN";
        case TOKEN_COMMA:          return "COMMA";
        case TOKEN_RETURN:         return "RETURN";
        default:                   return "UNKNOWN";
    }
}

typedef struct {
    const char* word;
    TokenType   type;
} Keyword;

static Keyword keywords[] = {
    {"if",     TOKEN_IF},
    {"else",   TOKEN_ELSE},
    {"while",  TOKEN_WHILE},
    {"fun",    TOKEN_FUN},
    {"return", TOKEN_RETURN},
};
#define NUM_KEYWORDS ((int)(sizeof(keywords) / sizeof(keywords[0])))

static int is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static int is_digit(char c) {
    return c >= '0' && c <= '9';
}

static int sv_equals(StringView sv, const char* str) {
    int i = 0;
    while (i < sv.len && str[i] && sv.ptr[i] == str[i]) i++;
    return i == sv.len && str[i] == '\0';
}

void lexer_init(Lexer* l, const char* source, int source_len) {
    l->source      = source;
    l->source_len  = source_len;
    l->start       = 0;
    l->cur         = 0;
    l->token_count = 0;
}

int lexer_end_reached(Lexer* l) {
    return l->cur >= l->source_len;
}

void lexer_get_token(Lexer* l) {
    if (lexer_end_reached(l)) return;

    l->start = l->cur;
    char c = l->source[l->cur];

    if (is_alpha(c)) {
        while (!lexer_end_reached(l) && is_alpha(l->source[l->cur]))
            l->cur++;
        StringView lexeme = {l->source + l->start, l->cur - l->start};
        TokenType type = TOKEN_IDENTIFIER;
        for (int i = 0; i < NUM_KEYWORDS; i++) {
            if (sv_equals(lexeme, keywords[i].word)) {
                type = keywords[i].type;
                break;
            }
        }
        if (l->token_count < MAX_TOKENS)
            l->tokens[l->token_count++] = (Token){lexeme, type};

    } else if (is_digit(c)) {
        while (!lexer_end_reached(l) && is_digit(l->source[l->cur]))
            l->cur++;
        StringView lexeme = {l->source + l->start, l->cur - l->start};
        if (l->token_count < MAX_TOKENS)
            l->tokens[l->token_count++] = (Token){lexeme, TOKEN_NUMBER};

    } else if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
        l->cur++;

    } else {
        StringView lexeme = {l->source + l->start, 1};
        TokenType type;
        int push = 1;

        switch (c) {
            case '+': type = TOKEN_ADD;       break;
            case '-': type = TOKEN_SUBTRACT;  break;
            case '*': type = TOKEN_MULTIPLY;  break;
            case '/': type = TOKEN_DIVIDE;    break;
            case ';': type = TOKEN_SEMICOLON; break;
            case '(': type = TOKEN_LEFT_PAREN;  break;
            case ')': type = TOKEN_RIGHT_PAREN; break;
            case '{': type = TOKEN_LEFT_BRACE;  break;
            case '}': type = TOKEN_RIGHT_BRACE; break;
            case ',': type = TOKEN_COMMA;     break;
            case '=':
                if (l->cur + 1 < l->source_len && l->source[l->cur + 1] == '=') {
                    lexeme = (StringView){l->source + l->start, 2};
                    type = TOKEN_DOUBLE_EQUALS;
                    l->cur++;
                } else {
                    type = TOKEN_EQUALS;
                }
                break;
            case '<':
                if (l->cur + 1 < l->source_len && l->source[l->cur + 1] == '=') {
                    lexeme = (StringView){l->source + l->start, 2};
                    type = TOKEN_LESS_EQUALS;
                    l->cur++;
                } else {
                    type = TOKEN_LESS;
                }
                break;
            case '>':
                if (l->cur + 1 < l->source_len && l->source[l->cur + 1] == '=') {
                    lexeme = (StringView){l->source + l->start, 2};
                    type = TOKEN_GREATER_EQUALS;
                    l->cur++;
                } else {
                    type = TOKEN_GREATER;
                }
                break;
            case '!':
                if (l->cur + 1 < l->source_len && l->source[l->cur + 1] == '=') {
                    lexeme = (StringView){l->source + l->start, 2};
                    type = TOKEN_NOT_EQUALS;
                    l->cur++;
                } else {
                    push = 0;
                }
                break;
            default:
                push = 0;
                break;
        }

        if (push && l->token_count < MAX_TOKENS)
            l->tokens[l->token_count++] = (Token){lexeme, type};
        l->cur++;
    }
}

void lexer_tokenize(Lexer* l) {
    while (!lexer_end_reached(l))
        lexer_get_token(l);
}
