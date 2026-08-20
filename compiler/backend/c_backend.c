#include "compiler/backend/c_backend.h"

#include <string.h>

static int text_equals(JslAstText text, const char *expected) {
    return text.length == strlen(expected) && memcmp(text.start, expected, text.length) == 0;
}

static void set_error(JslCBackend *backend, JslPosition position, const char *message) {
    if (backend->error_message == NULL) {
        backend->error_message = message;
        backend->error_position = position;
    }
}

void jsl_c_backend_init(JslCBackend *backend) {
    *backend = (JslCBackend){NULL, {NULL, 0, 0}};
}

int jsl_c_backend_emit(const JslAstProgram *program, FILE *output, JslCBackend *backend) {
    if (program->declarations.count != 1) {
        set_error(backend, program->declarations.count == 0 ? (JslPosition){"<input>", 1, 1} : program->declarations.items[0]->position, "v0.0.5 backend supports exactly one main function");
        return 0;
    }
    const JslAstNode *function = program->declarations.items[0];
    if (function->kind != JSL_AST_FUNCTION_DECLARATION || !text_equals(function->as.function_declaration.name, "main") || function->as.function_declaration.parameter_count != 0 || !text_equals(function->as.function_declaration.return_type, "i32")) {
        set_error(backend, function->position, "v0.0.5 backend requires 'function main(): i32'");
        return 0;
    }
    const JslAstNodeList *statements = &function->as.function_declaration.body->as.block_statement.statements;
    if (statements->count != 1 || statements->items[0]->kind != JSL_AST_RETURN_STATEMENT) {
        set_error(backend, function->as.function_declaration.body->position, "v0.0.5 backend requires a single return statement in main");
        return 0;
    }
    const JslAstNode *value = statements->items[0]->as.return_statement.value;
    if (value == NULL || value->kind != JSL_AST_LITERAL_EXPRESSION || value->as.literal_expression.kind != JSL_AST_LITERAL_INTEGER) {
        set_error(backend, statements->items[0]->position, "v0.0.5 backend requires an integer literal return value");
        return 0;
    }
    if (fprintf(output, "#include <stdint.h>\n\nint main(void) {\n    return (int32_t)(%.*s);\n}\n", (int)value->as.literal_expression.value.length, value->as.literal_expression.value.start) < 0) {
        set_error(backend, value->position, "unable to write generated C source");
        return 0;
    }
    return 1;
}
