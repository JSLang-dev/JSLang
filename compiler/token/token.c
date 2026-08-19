#include "compiler/token/token.h"

#include <string.h>

typedef struct { const char *name; JslTokenType type; } Keyword;

static const Keyword keywords[] = {
    {"function", JSL_TOKEN_FUNCTION}, {"return", JSL_TOKEN_RETURN}, {"const", JSL_TOKEN_CONST},
    {"let", JSL_TOKEN_LET}, {"struct", JSL_TOKEN_STRUCT}, {"interface", JSL_TOKEN_INTERFACE},
    {"if", JSL_TOKEN_IF}, {"else", JSL_TOKEN_ELSE}, {"while", JSL_TOKEN_WHILE},
    {"for", JSL_TOKEN_FOR}, {"true", JSL_TOKEN_TRUE}, {"false", JSL_TOKEN_FALSE},
    {"null", JSL_TOKEN_NULL}, {"import", JSL_TOKEN_IMPORT}, {"from", JSL_TOKEN_FROM},
};

const char *jsl_token_type_name(JslTokenType type) {
    static const char *names[] = {
        "ILLEGAL", "EOF", "IDENTIFIER", "INTEGER", "FLOAT", "STRING", "FUNCTION", "RETURN",
        "CONST", "LET", "STRUCT", "INTERFACE", "IF", "ELSE", "WHILE", "FOR", "TRUE", "FALSE",
        "NULL", "IMPORT", "FROM", "+", "-", "*", "/", "%", "=", "==", "!=", "<", ">",
        "<=", ">=", "&&", "||", "!", "(", ")", "{", "}", "[", "]", ";", "?", ":", ",", "."
    };
    return names[type];
}

JslTokenType jsl_keyword_type(const char *start, size_t length) {
    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
        if (strlen(keywords[i].name) == length && memcmp(keywords[i].name, start, length) == 0) {
            return keywords[i].type;
        }
    }
    return JSL_TOKEN_IDENTIFIER;
}
