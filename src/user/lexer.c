#include "lexer.h"

static char tts_buf[20];

const char* token_type_to_string(TokenType type) {
    switch (type) {
        case TOKEN_IF:
            tts_buf[0]='I';tts_buf[1]='F';tts_buf[2]=0; break;
        case TOKEN_ELSE:
            tts_buf[0]='E';tts_buf[1]='L';tts_buf[2]='S';tts_buf[3]='E';tts_buf[4]=0; break;
        case TOKEN_WHILE:
            tts_buf[0]='W';tts_buf[1]='H';tts_buf[2]='I';tts_buf[3]='L';tts_buf[4]='E';tts_buf[5]=0; break;
        case TOKEN_IDENTIFIER:
            tts_buf[0]='I';tts_buf[1]='D';tts_buf[2]='E';tts_buf[3]='N';tts_buf[4]='T';tts_buf[5]=0; break;
        case TOKEN_NUMBER:
            tts_buf[0]='N';tts_buf[1]='U';tts_buf[2]='M';tts_buf[3]=0; break;
        case TOKEN_ADD:
            tts_buf[0]='A';tts_buf[1]='D';tts_buf[2]='D';tts_buf[3]=0; break;
        case TOKEN_SUBTRACT:
            tts_buf[0]='S';tts_buf[1]='U';tts_buf[2]='B';tts_buf[3]=0; break;
        case TOKEN_MULTIPLY:
            tts_buf[0]='M';tts_buf[1]='U';tts_buf[2]='L';tts_buf[3]=0; break;
        case TOKEN_DIVIDE:
            tts_buf[0]='D';tts_buf[1]='I';tts_buf[2]='V';tts_buf[3]=0; break;
        case TOKEN_SEMICOLON:
            tts_buf[0]='S';tts_buf[1]='E';tts_buf[2]='M';tts_buf[3]='I';tts_buf[4]=0; break;
        case TOKEN_LEFT_PAREN:
            tts_buf[0]='L';tts_buf[1]='P';tts_buf[2]='A';tts_buf[3]='R';tts_buf[4]='N';tts_buf[5]=0; break;
        case TOKEN_RIGHT_PAREN:
            tts_buf[0]='R';tts_buf[1]='P';tts_buf[2]='A';tts_buf[3]='R';tts_buf[4]='N';tts_buf[5]=0; break;
        case TOKEN_LEFT_BRACE:
            tts_buf[0]='L';tts_buf[1]='B';tts_buf[2]='R';tts_buf[3]='C';tts_buf[4]='E';tts_buf[5]=0; break;
        case TOKEN_RIGHT_BRACE:
            tts_buf[0]='R';tts_buf[1]='B';tts_buf[2]='R';tts_buf[3]='C';tts_buf[4]='E';tts_buf[5]=0; break;
        case TOKEN_EQUALS:
            tts_buf[0]='E';tts_buf[1]='Q';tts_buf[2]=0; break;
        case TOKEN_LESS:
            tts_buf[0]='L';tts_buf[1]='T';tts_buf[2]=0; break;
        case TOKEN_LESS_EQUALS:
            tts_buf[0]='L';tts_buf[1]='E';tts_buf[2]=0; break;
        case TOKEN_GREATER:
            tts_buf[0]='G';tts_buf[1]='T';tts_buf[2]=0; break;
        case TOKEN_GREATER_EQUALS:
            tts_buf[0]='G';tts_buf[1]='E';tts_buf[2]=0; break;
        case TOKEN_DOUBLE_EQUALS:
            tts_buf[0]='E';tts_buf[1]='Q';tts_buf[2]='Q';tts_buf[3]=0; break;
        case TOKEN_NOT_EQUALS:
            tts_buf[0]='N';tts_buf[1]='E';tts_buf[2]=0; break;
        case TOKEN_FUN:
            tts_buf[0]='F';tts_buf[1]='U';tts_buf[2]='N';tts_buf[3]=0; break;
        case TOKEN_COMMA:
            tts_buf[0]='C';tts_buf[1]='M';tts_buf[2]='A';tts_buf[3]=0; break;
        case TOKEN_RETURN:
            tts_buf[0]='R';tts_buf[1]='E';tts_buf[2]='T';tts_buf[3]=0; break;
        default:
            tts_buf[0]='?';tts_buf[1]=0; break;
    }
    return tts_buf;
}

static int is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static int is_digit(char c) {
    return c >= '0' && c <= '9';
}

// All comparisons use character literals (embedded in .text), not string
// literals (which would land in .rodata and read as zero due to linker layout).
static TokenType check_keyword(StringView sv) {
    if (sv.len == 2 && sv.ptr[0] == 'i' && sv.ptr[1] == 'f')
        return TOKEN_IF;
    if (sv.len == 4 && sv.ptr[0] == 'e' && sv.ptr[1] == 'l' &&
        sv.ptr[2] == 's' && sv.ptr[3] == 'e')
        return TOKEN_ELSE;
    if (sv.len == 5 && sv.ptr[0] == 'w' && sv.ptr[1] == 'h' &&
        sv.ptr[2] == 'i' && sv.ptr[3] == 'l' && sv.ptr[4] == 'e')
        return TOKEN_WHILE;
    if (sv.len == 3 && sv.ptr[0] == 'f' && sv.ptr[1] == 'u' && sv.ptr[2] == 'n')
        return TOKEN_FUN;
    if (sv.len == 6 && sv.ptr[0] == 'r' && sv.ptr[1] == 'e' &&
        sv.ptr[2] == 't' && sv.ptr[3] == 'u' && sv.ptr[4] == 'r' && sv.ptr[5] == 'n')
        return TOKEN_RETURN;
    return TOKEN_IDENTIFIER;
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
        TokenType type = check_keyword(lexeme);
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
            case '+': type = TOKEN_ADD;         break;
            case '-': type = TOKEN_SUBTRACT;    break;
            case '*': type = TOKEN_MULTIPLY;    break;
            case '/': type = TOKEN_DIVIDE;      break;
            case ';': type = TOKEN_SEMICOLON;   break;
            case '(': type = TOKEN_LEFT_PAREN;  break;
            case ')': type = TOKEN_RIGHT_PAREN; break;
            case '{': type = TOKEN_LEFT_BRACE;  break;
            case '}': type = TOKEN_RIGHT_BRACE; break;
            case ',': type = TOKEN_COMMA;       break;
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
