#ifndef JSLANG_COMPILER_PARSER_PARSER_H
#define JSLANG_COMPILER_PARSER_PARSER_H

#include "compiler/ast/ast.h"
#include "compiler/lexer/lexer.h"

typedef struct {
    JslLexer lexer;
    JslToken current;
    JslToken previous;
    const char *error_message;
    JslPosition error_position;
    int had_error;
} JslParser;

void jsl_parser_init(JslParser *parser, const char *filename, const char *source);
int jsl_parser_parse_program(JslParser *parser, JslAstProgram *program);
const char *jsl_parser_error(const JslParser *parser, JslPosition *position);
void jsl_ast_print_program(const JslAstProgram *program);

#endif
