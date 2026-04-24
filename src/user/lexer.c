#include "lexer.h"

static char tts_buf[20];

static void buf_copy(char* dst, char* src) {
    int i = 0;
    while ((dst[i] = src[i])) i++;
}

const char* token_type_to_string(TokenType type) {
    switch (type) {
        case TOKEN_IF:             { char s[] = "IF";           buf_copy(tts_buf, s); break; }
        case TOKEN_ELSE:           { char s[] = "ELSE";         buf_copy(tts_buf, s); break; }
        case TOKEN_WHILE:          { char s[] = "WHILE";        buf_copy(tts_buf, s); break; }
        case TOKEN_IDENTIFIER:     { char s[] = "IDENT";        buf_copy(tts_buf, s); break; }
        case TOKEN_NUMBER:         { char s[] = "NUM";          buf_copy(tts_buf, s); break; }
        case TOKEN_ADD:            { char s[] = "ADD";          buf_copy(tts_buf, s); break; }
        case TOKEN_SUBTRACT:       { char s[] = "SUB";          buf_copy(tts_buf, s); break; }
        case TOKEN_MULTIPLY:       { char s[] = "MUL";          buf_copy(tts_buf, s); break; }
        case TOKEN_DIVIDE:         { char s[] = "DIV";          buf_copy(tts_buf, s); break; }
        case TOKEN_SEMICOLON:      { char s[] = "SEMI";         buf_copy(tts_buf, s); break; }
        case TOKEN_LEFT_PAREN:     { char s[] = "LPARN";        buf_copy(tts_buf, s); break; }
        case TOKEN_RIGHT_PAREN:    { char s[] = "RPARN";        buf_copy(tts_buf, s); break; }
        case TOKEN_LEFT_BRACE:     { char s[] = "LBRCE";        buf_copy(tts_buf, s); break; }
        case TOKEN_RIGHT_BRACE:    { char s[] = "RBRCE";        buf_copy(tts_buf, s); break; }
        case TOKEN_EQUALS:         { char s[] = "EQ";           buf_copy(tts_buf, s); break; }
        case TOKEN_LESS:           { char s[] = "LT";           buf_copy(tts_buf, s); break; }
        case TOKEN_LESS_EQUALS:    { char s[] = "LE";           buf_copy(tts_buf, s); break; }
        case TOKEN_GREATER:        { char s[] = "GT";           buf_copy(tts_buf, s); break; }
        case TOKEN_GREATER_EQUALS: { char s[] = "GE";           buf_copy(tts_buf, s); break; }
        case TOKEN_DOUBLE_EQUALS:  { char s[] = "EQQ";          buf_copy(tts_buf, s); break; }
        case TOKEN_NOT_EQUALS:     { char s[] = "NE";           buf_copy(tts_buf, s); break; }
        case TOKEN_FUN:            { char s[] = "FUN";          buf_copy(tts_buf, s); break; }
        case TOKEN_COMMA:          { char s[] = "CMA";          buf_copy(tts_buf, s); break; }
        case TOKEN_RETURN:         { char s[] = "RET";          buf_copy(tts_buf, s); break; }
        case TOKEN_STRING:         { char s[] = "STR";          buf_copy(tts_buf, s); break; }
        default:                   { tts_buf[0] = '?'; tts_buf[1] = 0;               break; }
    }
    return tts_buf;
}

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

static TokenType check_keyword(StringView sv) {
    char kw_if[]     = "if";
    char kw_else[]   = "else";
    char kw_while[]  = "while";
    char kw_fun[]    = "fun";
    char kw_return[] = "return";
    if (sv_equals(sv, kw_if))     return TOKEN_IF;
    if (sv_equals(sv, kw_else))   return TOKEN_ELSE;
    if (sv_equals(sv, kw_while))  return TOKEN_WHILE;
    if (sv_equals(sv, kw_fun))    return TOKEN_FUN;
    if (sv_equals(sv, kw_return)) return TOKEN_RETURN;
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

    } else if (c == '"') {
        l->cur++;
        while (!lexer_end_reached(l) && l->source[l->cur] != '"')
            l->cur++;
        if (!lexer_end_reached(l))
            l->cur++;
        StringView lexeme = {l->source + l->start, l->cur - l->start};
        if (l->token_count < MAX_TOKENS)
            l->tokens[l->token_count++] = (Token){lexeme, TOKEN_STRING};

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
