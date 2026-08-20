#include "compiler/backend/c_backend.h"

#include <stdarg.h>
#include <string.h>

static int text_equals(JslAstText text, const char *expected) { return text.length == strlen(expected) && memcmp(text.start, expected, text.length) == 0; }
static void set_error(JslCBackend *backend, JslPosition position, const char *message) { if (backend->error_message == NULL) { backend->error_message = message; backend->error_position = position; } }
static int write_text(FILE *output, JslAstText text) { return fprintf(output, "%.*s", (int)text.length, text.start) >= 0; }
static int write_format(FILE *output, const char *format, ...) { va_list arguments; va_start(arguments, format); int result = vfprintf(output, format, arguments); va_end(arguments); return result >= 0; }

void jsl_c_backend_init(JslCBackend *backend) { *backend = (JslCBackend){NULL, {NULL, 0, 0}}; }

static int emit_expression(const JslAstNode *node, FILE *output, JslCBackend *backend) {
    switch (node->kind) {
        case JSL_AST_IDENTIFIER_EXPRESSION: return write_text(output, node->as.identifier_expression.name);
        case JSL_AST_LITERAL_EXPRESSION:
            if (node->as.literal_expression.kind == JSL_AST_LITERAL_INTEGER) return write_text(output, node->as.literal_expression.value);
            set_error(backend, node->position, "v0.0.6 backend supports only integer literals"); return 0;
        case JSL_AST_GROUPING_EXPRESSION:
            return write_format(output, "(") && emit_expression(node->as.grouping_expression.expression, output, backend) && write_format(output, ")");
        case JSL_AST_UNARY_EXPRESSION:
            return write_text(output, node->as.unary_expression.operator_text) && emit_expression(node->as.unary_expression.operand, output, backend);
        case JSL_AST_BINARY_EXPRESSION:
            return write_format(output, "(") && emit_expression(node->as.binary_expression.left, output, backend) && write_format(output, " ") && write_text(output, node->as.binary_expression.operator_text) && write_format(output, " ") && emit_expression(node->as.binary_expression.right, output, backend) && write_format(output, ")");
        case JSL_AST_CALL_EXPRESSION:
            if (node->as.call_expression.callee->kind != JSL_AST_IDENTIFIER_EXPRESSION) { set_error(backend, node->position, "v0.0.6 backend supports direct function calls only"); return 0; }
            if (!emit_expression(node->as.call_expression.callee, output, backend) || !write_format(output, "(")) return 0;
            for (size_t i = 0; i < node->as.call_expression.arguments.count; i++) {
                if ((i != 0 && !write_format(output, ", ")) || !emit_expression(node->as.call_expression.arguments.items[i], output, backend)) return 0;
            }
            return write_format(output, ")");
        default: set_error(backend, node->position, "unsupported expression in v0.0.6 backend"); return 0;
    }
}

static int emit_parameters(const JslAstNode *function, FILE *output, JslCBackend *backend) {
    for (size_t i = 0; i < function->as.function_declaration.parameter_count; i++) {
        const JslAstParameter *parameter = &function->as.function_declaration.parameters[i];
        if (!text_equals(parameter->type_name, "i32")) { set_error(backend, parameter->position, "v0.0.6 backend supports i32 parameters only"); return 0; }
        if ((i != 0 && !write_format(output, ", ")) || !write_format(output, "int32_t ") || !write_text(output, parameter->name)) return 0;
    }
    return 1;
}

static int emit_function_signature(const JslAstNode *function, FILE *output, JslCBackend *backend, int prototype) {
    if (!text_equals(function->as.function_declaration.return_type, "i32")) { set_error(backend, function->position, "v0.0.6 backend supports i32 return types only"); return 0; }
    const char *return_type = text_equals(function->as.function_declaration.name, "main") ? "int" : "int32_t";
    if (!write_format(output, "%s ", return_type) || !write_text(output, function->as.function_declaration.name) || !write_format(output, "(")) return 0;
    if (function->as.function_declaration.parameter_count == 0) {
        if (!write_format(output, "void")) return 0;
    } else if (!emit_parameters(function, output, backend)) return 0;
    return write_format(output, prototype ? ");\n" : ")\n");
}

static int emit_function_body(const JslAstNode *function, FILE *output, JslCBackend *backend) {
    const JslAstNodeList *statements = &function->as.function_declaration.body->as.block_statement.statements;
    if (!write_format(output, "{\n")) return 0;
    for (size_t i = 0; i < statements->count; i++) {
        const JslAstNode *statement = statements->items[i];
        if (statement->kind == JSL_AST_VARIABLE_STATEMENT) {
            if (statement->as.variable_statement.type_name.start != NULL && !text_equals(statement->as.variable_statement.type_name, "i32")) { set_error(backend, statement->position, "v0.0.6 backend supports i32 local variables only"); return 0; }
            if (!write_format(output, "    %sint32_t ", statement->as.variable_statement.is_mutable ? "" : "const ") || !write_text(output, statement->as.variable_statement.name) || !write_format(output, " = ") || !emit_expression(statement->as.variable_statement.initializer, output, backend) || !write_format(output, ";\n")) return 0;
        } else if (statement->kind == JSL_AST_RETURN_STATEMENT) {
            if (statement->as.return_statement.value == NULL || !write_format(output, "    return ") || !emit_expression(statement->as.return_statement.value, output, backend) || !write_format(output, ";\n")) { if (backend->error_message == NULL) set_error(backend, statement->position, "v0.0.6 backend requires return values"); return 0; }
        } else if (statement->kind == JSL_AST_EXPRESSION_STATEMENT) {
            if (!write_format(output, "    ") || !emit_expression(statement->as.expression_statement.expression, output, backend) || !write_format(output, ";\n")) return 0;
        } else { set_error(backend, statement->position, "unsupported statement in v0.0.6 backend"); return 0; }
    }
    return write_format(output, "}\n\n");
}

int jsl_c_backend_emit(const JslAstProgram *program, FILE *output, JslCBackend *backend) {
    size_t main_count = 0;
    if (!write_format(output, "#include <stdint.h>\n\n")) return 0;
    for (size_t i = 0; i < program->declarations.count; i++) {
        const JslAstNode *function = program->declarations.items[i];
        if (function->kind != JSL_AST_FUNCTION_DECLARATION) { set_error(backend, function->position, "v0.0.6 backend supports function declarations only"); return 0; }
        if (text_equals(function->as.function_declaration.name, "main")) {
            main_count++;
            if (function->as.function_declaration.parameter_count != 0) { set_error(backend, function->position, "main function cannot have parameters"); return 0; }
        }
        if (!emit_function_signature(function, output, backend, 1)) return 0;
    }
    if (main_count != 1) { set_error(backend, program->declarations.count == 0 ? (JslPosition){"<input>", 1, 1} : program->declarations.items[0]->position, "v0.0.6 backend requires exactly one main function"); return 0; }
    if (!write_format(output, "\n")) return 0;
    for (size_t i = 0; i < program->declarations.count; i++) {
        const JslAstNode *function = program->declarations.items[i];
        if (!emit_function_signature(function, output, backend, 0) || !emit_function_body(function, output, backend)) return 0;
    }
    return 1;
}
