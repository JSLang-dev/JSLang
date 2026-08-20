#ifndef JSLANG_COMPILER_TYPES_TYPES_H
#define JSLANG_COMPILER_TYPES_TYPES_H

#include "compiler/ast/ast.h"

typedef enum {
    JSL_TYPE_UNKNOWN,
    JSL_TYPE_VOID,
    JSL_TYPE_BOOL,
    JSL_TYPE_CHAR,
    JSL_TYPE_STRING,
    JSL_TYPE_NULL,
    JSL_TYPE_I8, JSL_TYPE_I16, JSL_TYPE_I32, JSL_TYPE_I64,
    JSL_TYPE_U8, JSL_TYPE_U16, JSL_TYPE_U32, JSL_TYPE_U64,
    JSL_TYPE_ISIZE, JSL_TYPE_USIZE,
    JSL_TYPE_F32, JSL_TYPE_F64
} JslType;

JslType jsl_type_from_name(JslAstText name);
const char *jsl_type_name(JslType type);
int jsl_type_is_numeric(JslType type);
int jsl_type_is_known(JslType type);

#endif
