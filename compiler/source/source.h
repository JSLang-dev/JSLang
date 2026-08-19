#ifndef JSLANG_COMPILER_SOURCE_SOURCE_H
#define JSLANG_COMPILER_SOURCE_SOURCE_H

#include <stddef.h>

typedef struct {
    const char *filename;
    size_t line;
    size_t column;
} JslPosition;

void jsl_position_format(const JslPosition *position, char *buffer, size_t buffer_size);

#endif
