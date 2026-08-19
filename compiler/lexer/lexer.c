#include "compiler/lexer/lexer.h"

#include <ctype.h>

static char current(const JslLexer *lexer) { return *lexer->current; }
static char peek(const JslLexer *lexer) { return lexer->current[0] == '\0' ? '\0' : lexer->current[1]; }
static JslPosition position(const JslLexer *lexer) { return (JslPosition){lexer->filename, lexer->line, lexer->column}; }
static void advance(JslLexer *lexer) {
    if (current(lexer) == '\n') { lexer->line++; lexer->column = 1; }
    else if (current(lexer) != '\0') { lexer->column++; }
    lexer->current++;
}
static int identifier_start(char character) { return character == '_' || isalpha((unsigned char)character); }
static int identifier_part(char character) { return identifier_start(character) || isdigit((unsigned char)character); }
static JslToken make_token(JslLexer *lexer, JslTokenType type, const char *start, JslPosition start_position) {
    return (JslToken){type, start, (size_t)(lexer->current - start), start_position};
}
static JslToken error_token(JslLexer *lexer, const char *message, JslPosition error_position) {
    lexer->error_message = message;
    lexer->error_position = error_position;
    return (JslToken){JSL_TOKEN_ILLEGAL, NULL, 0, error_position};
}

void jsl_lexer_init(JslLexer *lexer, const char *filename, const char *source) {
    *lexer = (JslLexer){filename, source, source, 1, 1, NULL, {filename, 1, 1}, '\0'};
}

static int skip_trivia(JslLexer *lexer) {
    for (;;) {
        while (isspace((unsigned char)current(lexer))) advance(lexer);
        if (current(lexer) == '/' && peek(lexer) == '/') {
            while (current(lexer) != '\0' && current(lexer) != '\n') advance(lexer);
            continue;
        }
        if (current(lexer) == '/' && peek(lexer) == '*') {
            JslPosition start = position(lexer);
            advance(lexer); advance(lexer);
            while (current(lexer) != '\0' && !(current(lexer) == '*' && peek(lexer) == '/')) advance(lexer);
            if (current(lexer) == '\0') { error_token(lexer, "unterminated block comment", start); return 0; }
            advance(lexer); advance(lexer);
            continue;
        }
        return 1;
    }
}

JslToken jsl_lexer_next(JslLexer *lexer) {
    if (!skip_trivia(lexer)) return (JslToken){JSL_TOKEN_ILLEGAL, NULL, 0, lexer->error_position};
    JslPosition start_position = position(lexer);
    const char *start = lexer->current;
    char character = current(lexer);
    if (character == '\0') return make_token(lexer, JSL_TOKEN_EOF, start, start_position);
    if (identifier_start(character)) {
        do { advance(lexer); } while (identifier_part(current(lexer)));
        JslToken token = make_token(lexer, JSL_TOKEN_IDENTIFIER, start, start_position);
        token.type = jsl_keyword_type(start, token.length);
        return token;
    }
    if (isdigit((unsigned char)character)) {
        do { advance(lexer); } while (isdigit((unsigned char)current(lexer)));
        JslTokenType type = JSL_TOKEN_INTEGER;
        if (current(lexer) == '.' && isdigit((unsigned char)peek(lexer))) {
            type = JSL_TOKEN_FLOAT; advance(lexer);
            while (isdigit((unsigned char)current(lexer))) advance(lexer);
        }
        return make_token(lexer, type, start, start_position);
    }
    if (character == '"') {
        advance(lexer);
        const char *content = lexer->current;
        while (current(lexer) != '"') {
            if (current(lexer) == '\0' || current(lexer) == '\n') return error_token(lexer, "unterminated string literal", start_position);
            if (current(lexer) == '\\') { advance(lexer); if (current(lexer) == '\0') return error_token(lexer, "unterminated string literal", start_position); }
            advance(lexer);
        }
        JslToken token = {JSL_TOKEN_STRING, content, (size_t)(lexer->current - content), start_position};
        advance(lexer);
        return token;
    }
    advance(lexer);
    switch (character) {
        case '+': return make_token(lexer, JSL_TOKEN_PLUS, start, start_position);
        case '-': return make_token(lexer, JSL_TOKEN_MINUS, start, start_position);
        case '*': return make_token(lexer, JSL_TOKEN_ASTERISK, start, start_position);
        case '/': return make_token(lexer, JSL_TOKEN_SLASH, start, start_position);
        case '%': return make_token(lexer, JSL_TOKEN_PERCENT, start, start_position);
        case '(': return make_token(lexer, JSL_TOKEN_LEFT_PAREN, start, start_position);
        case ')': return make_token(lexer, JSL_TOKEN_RIGHT_PAREN, start, start_position);
        case '{': return make_token(lexer, JSL_TOKEN_LEFT_BRACE, start, start_position);
        case '}': return make_token(lexer, JSL_TOKEN_RIGHT_BRACE, start, start_position);
        case '[': return make_token(lexer, JSL_TOKEN_LEFT_BRACKET, start, start_position);
        case ']': return make_token(lexer, JSL_TOKEN_RIGHT_BRACKET, start, start_position);
        case ';': return make_token(lexer, JSL_TOKEN_SEMICOLON, start, start_position);
        case ':': return make_token(lexer, JSL_TOKEN_COLON, start, start_position);
        case ',': return make_token(lexer, JSL_TOKEN_COMMA, start, start_position);
        case '.': return make_token(lexer, JSL_TOKEN_DOT, start, start_position);
        case '=': if (current(lexer) == '=') { advance(lexer); return make_token(lexer, JSL_TOKEN_EQUAL, start, start_position); } return make_token(lexer, JSL_TOKEN_ASSIGN, start, start_position);
        case '!': if (current(lexer) == '=') { advance(lexer); return make_token(lexer, JSL_TOKEN_NOT_EQUAL, start, start_position); } return make_token(lexer, JSL_TOKEN_BANG, start, start_position);
        case '<': if (current(lexer) == '=') { advance(lexer); return make_token(lexer, JSL_TOKEN_LESS_EQUAL, start, start_position); } return make_token(lexer, JSL_TOKEN_LESS, start, start_position);
        case '>': if (current(lexer) == '=') { advance(lexer); return make_token(lexer, JSL_TOKEN_GREATER_EQUAL, start, start_position); } return make_token(lexer, JSL_TOKEN_GREATER, start, start_position);
        case '&': if (current(lexer) == '&') { advance(lexer); return make_token(lexer, JSL_TOKEN_AND, start, start_position); } break;
        case '|': if (current(lexer) == '|') { advance(lexer); return make_token(lexer, JSL_TOKEN_OR, start, start_position); } break;
    }
    lexer->error_character = character;
    return error_token(lexer, "unexpected character", start_position);
}

const char *jsl_lexer_error(const JslLexer *lexer, JslPosition *error_position) {
    if (error_position != NULL) *error_position = lexer->error_position;
    return lexer->error_message;
}

char jsl_lexer_error_character(const JslLexer *lexer) {
    return lexer->error_character;
}
