#ifndef JSLANG_COMPILER_LEXER_LEXER_H
#define JSLANG_COMPILER_LEXER_LEXER_H

#include "compiler/token/token.h"

typedef struct {
    const char *filename;
    const char *source;
    const char *current;
    size_t line;
    size_t column;
    const char *error_message;
    JslPosition error_position;
	char error_character;
} JslLexer;

void jsl_lexer_init(JslLexer *lexer, const char *filename, const char *source);
JslToken jsl_lexer_next(JslLexer *lexer);
const char *jsl_lexer_error(const JslLexer *lexer, JslPosition *position);
char jsl_lexer_error_character(const JslLexer *lexer);

#endif
