#ifndef JSLANG_COMPILER_CHECKER_CHECKER_H
#define JSLANG_COMPILER_CHECKER_CHECKER_H

#include "compiler/ast/ast.h"

typedef enum {
    JSL_SYMBOL_FUNCTION,
    JSL_SYMBOL_VARIABLE,
    JSL_SYMBOL_PARAMETER,
    JSL_SYMBOL_IMPORT,
    JSL_SYMBOL_STRUCT,
    JSL_SYMBOL_BUILTIN
} JslSymbolKind;

typedef struct {
    const char *error_message;
    JslPosition error_position;
    int had_error;
} JslChecker;

void jsl_checker_init(JslChecker *checker);
int jsl_checker_check_program(JslChecker *checker, const JslAstProgram *program);
const char *jsl_checker_error(const JslChecker *checker, JslPosition *position);

#endif
