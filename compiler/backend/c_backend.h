#ifndef JSLANG_COMPILER_BACKEND_C_BACKEND_H
#define JSLANG_COMPILER_BACKEND_C_BACKEND_H

#include "compiler/ast/ast.h"

#include <stdio.h>

typedef struct {
    const char *error_message;
    JslPosition error_position;
} JslCBackend;

void jsl_c_backend_init(JslCBackend *backend);
int jsl_c_backend_emit(const JslAstProgram *program, FILE *output, JslCBackend *backend);

#endif
