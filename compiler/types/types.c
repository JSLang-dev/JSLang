#include "compiler/types/types.h"

#include <string.h>

typedef struct { const char *name; JslType type; } TypeName;

static const TypeName types[] = {
    {"void", JSL_TYPE_VOID}, {"bool", JSL_TYPE_BOOL}, {"char", JSL_TYPE_CHAR}, {"string", JSL_TYPE_STRING},
    {"i8", JSL_TYPE_I8}, {"i16", JSL_TYPE_I16}, {"i32", JSL_TYPE_I32}, {"i64", JSL_TYPE_I64},
    {"u8", JSL_TYPE_U8}, {"u16", JSL_TYPE_U16}, {"u32", JSL_TYPE_U32}, {"u64", JSL_TYPE_U64},
    {"isize", JSL_TYPE_ISIZE}, {"usize", JSL_TYPE_USIZE}, {"f32", JSL_TYPE_F32}, {"f64", JSL_TYPE_F64},
};

JslType jsl_type_from_name(JslAstText name) {
    for (size_t i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
        if (strlen(types[i].name) == name.length && memcmp(types[i].name, name.start, name.length) == 0) return types[i].type;
    }
    return JSL_TYPE_UNKNOWN;
}

const char *jsl_type_name(JslType type) {
    static const char *names[] = {"unknown", "void", "bool", "char", "string", "null", "i8", "i16", "i32", "i64", "u8", "u16", "u32", "u64", "isize", "usize", "f32", "f64"};
    return names[type];
}

int jsl_type_is_numeric(JslType type) { return type >= JSL_TYPE_I8 && type <= JSL_TYPE_F64; }
int jsl_type_is_known(JslType type) { return type != JSL_TYPE_UNKNOWN; }
