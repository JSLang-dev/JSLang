#include "compiler/source/source.h"

#include <stdio.h>

void jsl_position_format(const JslPosition *position, char *buffer, size_t buffer_size) {
    snprintf(buffer, buffer_size, "%s:%zu:%zu", position->filename, position->line, position->column);
}
