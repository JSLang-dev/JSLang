#include "compiler/backend/c_backend.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

typedef enum { JSL_C_VALUE_I32, JSL_C_VALUE_BOOL, JSL_C_VALUE_STRING, JSL_C_VALUE_STRUCT, JSL_C_VALUE_INVALID } JslCValueType;
typedef struct JslLocal JslLocal;
struct JslLocal { JslAstText name; JslCValueType type; JslLocal *next; };

static int text_equals(JslAstText text, const char *expected) { return text.length == strlen(expected) && memcmp(text.start, expected, text.length) == 0; }
static int text_matches(JslAstText left, JslAstText right) { return left.length == right.length && memcmp(left.start, right.start, left.length) == 0; }
static void set_error(JslCBackend *backend, JslPosition position, const char *message) { if (backend->error_message == NULL) { backend->error_message = message; backend->error_position = position; } }
static int write_text(FILE *output, JslAstText text) { return fprintf(output, "%.*s", (int)text.length, text.start) >= 0; }
static int write_format(FILE *output, const char *format, ...) { va_list arguments; va_start(arguments, format); int result = vfprintf(output, format, arguments); va_end(arguments); return result >= 0; }
static int write_string_literal(FILE *output, JslAstText text) { return write_format(output, "\"") && write_text(output, text) && write_format(output, "\""); }

void jsl_c_backend_init(JslCBackend *backend) { *backend = (JslCBackend){NULL, {NULL, 0, 0}}; }

static JslCValueType local_type(const JslLocal *locals, JslAstText name) { for (; locals != NULL; locals = locals->next) if (text_matches(locals->name, name)) return locals->type; return JSL_C_VALUE_INVALID; }
static int add_local(JslLocal **locals, JslAstText name, JslCValueType type, JslCBackend *backend, JslPosition position) { JslLocal *local = malloc(sizeof(*local)); if (local == NULL) { set_error(backend, position, "out of memory"); return 0; } *local = (JslLocal){name, type, *locals}; *locals = local; return 1; }
static void free_locals(JslLocal *locals) { while (locals != NULL) { JslLocal *next = locals->next; free(locals); locals = next; } }
static void free_locals_until(JslLocal *locals, const JslLocal *parent) { while (locals != parent) { JslLocal *next = locals->next; free(locals); locals = next; } }

static int is_console_member(const JslAstNode *node, const char *name) {
    return node->kind == JSL_AST_MEMBER_EXPRESSION && node->as.member_expression.object->kind == JSL_AST_IDENTIFIER_EXPRESSION && text_equals(node->as.member_expression.object->as.identifier_expression.name, "console") && text_equals(node->as.member_expression.property, name);
}

static JslCValueType expression_type(const JslAstNode *node, const JslLocal *locals) {
    switch (node->kind) {
        case JSL_AST_IDENTIFIER_EXPRESSION: return local_type(locals, node->as.identifier_expression.name);
        case JSL_AST_LITERAL_EXPRESSION: return node->as.literal_expression.kind == JSL_AST_LITERAL_INTEGER ? JSL_C_VALUE_I32 : node->as.literal_expression.kind == JSL_AST_LITERAL_STRING ? JSL_C_VALUE_STRING : (node->as.literal_expression.kind == JSL_AST_LITERAL_TRUE || node->as.literal_expression.kind == JSL_AST_LITERAL_FALSE) ? JSL_C_VALUE_BOOL : JSL_C_VALUE_INVALID;
        case JSL_AST_UNARY_EXPRESSION: return node->as.unary_expression.operator_text.start[0] == '!' ? JSL_C_VALUE_BOOL : JSL_C_VALUE_I32;
        case JSL_AST_BINARY_EXPRESSION: { char operator = node->as.binary_expression.operator_text.start[0]; return operator == '=' || operator == '!' || operator == '<' || operator == '>' || operator == '&' || operator == '|' ? JSL_C_VALUE_BOOL : JSL_C_VALUE_I32; }
        case JSL_AST_CONDITIONAL_EXPRESSION: return expression_type(node->as.conditional_expression.then_expression, locals);
        case JSL_AST_GROUPING_EXPRESSION: return expression_type(node->as.grouping_expression.expression, locals);
        case JSL_AST_CALL_EXPRESSION:
            if (is_console_member(node->as.call_expression.callee, "read") || is_console_member(node->as.call_expression.callee, "readLine")) return JSL_C_VALUE_STRING;
            return node->as.call_expression.callee->kind == JSL_AST_IDENTIFIER_EXPRESSION ? JSL_C_VALUE_I32 : JSL_C_VALUE_INVALID;
        case JSL_AST_STRUCT_LITERAL_EXPRESSION: return JSL_C_VALUE_STRUCT;
        case JSL_AST_MEMBER_EXPRESSION: return JSL_C_VALUE_I32;
        default: return JSL_C_VALUE_INVALID;
    }
}

static int emit_expression(const JslAstNode *node, const JslLocal *locals, FILE *output, JslCBackend *backend) {
    switch (node->kind) {
        case JSL_AST_IDENTIFIER_EXPRESSION:
            if (local_type(locals, node->as.identifier_expression.name) == JSL_C_VALUE_INVALID) { set_error(backend, node->position, "unknown local in v0.0.9 backend"); return 0; }
            return write_text(output, node->as.identifier_expression.name);
        case JSL_AST_LITERAL_EXPRESSION:
            if (node->as.literal_expression.kind == JSL_AST_LITERAL_INTEGER) return write_text(output, node->as.literal_expression.value);
            if (node->as.literal_expression.kind == JSL_AST_LITERAL_STRING) return write_string_literal(output, node->as.literal_expression.value);
            if (node->as.literal_expression.kind == JSL_AST_LITERAL_TRUE) return write_format(output, "true");
            if (node->as.literal_expression.kind == JSL_AST_LITERAL_FALSE) return write_format(output, "false");
            set_error(backend, node->position, "v0.0.9 backend supports integer, bool, and string literals only"); return 0;
        case JSL_AST_GROUPING_EXPRESSION:
            return write_format(output, "(") && emit_expression(node->as.grouping_expression.expression, locals, output, backend) && write_format(output, ")");
        case JSL_AST_UNARY_EXPRESSION:
            return write_text(output, node->as.unary_expression.operator_text) && emit_expression(node->as.unary_expression.operand, locals, output, backend);
        case JSL_AST_BINARY_EXPRESSION:
            return write_format(output, "(") && emit_expression(node->as.binary_expression.left, locals, output, backend) && write_format(output, " ") && write_text(output, node->as.binary_expression.operator_text) && write_format(output, " ") && emit_expression(node->as.binary_expression.right, locals, output, backend) && write_format(output, ")");
        case JSL_AST_CONDITIONAL_EXPRESSION:
            return write_format(output, "(") && emit_expression(node->as.conditional_expression.condition, locals, output, backend) && write_format(output, " ? ") && emit_expression(node->as.conditional_expression.then_expression, locals, output, backend) && write_format(output, " : ") && emit_expression(node->as.conditional_expression.else_expression, locals, output, backend) && write_format(output, ")");
        case JSL_AST_CALL_EXPRESSION:
            if (is_console_member(node->as.call_expression.callee, "read") || is_console_member(node->as.call_expression.callee, "readLine")) {
                if (node->as.call_expression.arguments.count != 0) { set_error(backend, node->position, "console read methods do not accept arguments"); return 0; }
                return write_format(output, "jsl_console_read_line()");
            }
            if (node->as.call_expression.callee->kind != JSL_AST_IDENTIFIER_EXPRESSION) { set_error(backend, node->position, "v0.0.9 backend supports direct function calls only"); return 0; }
            if (!write_text(output, node->as.call_expression.callee->as.identifier_expression.name) || !write_format(output, "(")) return 0;
            for (size_t i = 0; i < node->as.call_expression.arguments.count; i++) if ((i != 0 && !write_format(output, ", ")) || !emit_expression(node->as.call_expression.arguments.items[i], locals, output, backend)) return 0;
            return write_format(output, ")");
        case JSL_AST_MEMBER_EXPRESSION:
            return emit_expression(node->as.member_expression.object, locals, output, backend) && write_format(output, ".") && write_text(output, node->as.member_expression.property);
        case JSL_AST_STRUCT_LITERAL_EXPRESSION:
            if (!write_format(output, "(") || !write_text(output, node->as.struct_literal_expression.type_name) || !write_format(output, "){ ")) return 0;
            for (size_t i = 0; i < node->as.struct_literal_expression.field_count; i++) {
                const JslAstStructLiteralField *field = &node->as.struct_literal_expression.fields[i];
                if ((i != 0 && !write_format(output, ", ")) || !write_format(output, ".") || !write_text(output, field->name) || !write_format(output, " = ") || !emit_expression(field->value, locals, output, backend)) return 0;
            }
            return write_format(output, " }");
        default: set_error(backend, node->position, "unsupported expression in v0.0.9 backend"); return 0;
    }
}

static int emit_console_statement(const JslAstNode *call, const JslLocal *locals, FILE *output, JslCBackend *backend) {
    const char *methods[] = {"log", "info", "warn", "error", "write"};
    const char *method = NULL;
    for (size_t i = 0; i < sizeof(methods) / sizeof(methods[0]); i++) if (is_console_member(call->as.call_expression.callee, methods[i])) method = methods[i];
    if (method == NULL) return 0;
    if (call->as.call_expression.arguments.count != 1) { set_error(backend, call->position, "console output methods require exactly one argument"); return -1; }
    const JslAstNode *argument = call->as.call_expression.arguments.items[0];
    JslCValueType type = expression_type(argument, locals);
    if (type != JSL_C_VALUE_I32 && type != JSL_C_VALUE_STRING) { set_error(backend, argument->position, "console output supports i32 and string values only"); return -1; }
    return write_format(output, "    jsl_console_%s_%s(", method, type == JSL_C_VALUE_I32 ? "i32" : "string") && emit_expression(argument, locals, output, backend) && write_format(output, ");\n") ? 1 : -1;
}

static int emit_parameters(const JslAstNode *function, FILE *output, JslCBackend *backend) {
    for (size_t i = 0; i < function->as.function_declaration.parameter_count; i++) {
        const JslAstParameter *parameter = &function->as.function_declaration.parameters[i];
        if (!text_equals(parameter->type_name, "i32")) { set_error(backend, parameter->position, "v0.0.9 backend supports i32 parameters only"); return 0; }
        if ((i != 0 && !write_format(output, ", ")) || !write_format(output, "int32_t ") || !write_text(output, parameter->name)) return 0;
    }
    return 1;
}

static int emit_function_signature(const JslAstNode *function, FILE *output, JslCBackend *backend, int prototype) {
    if (!text_equals(function->as.function_declaration.return_type, "i32")) { set_error(backend, function->position, "v0.0.9 backend supports i32 return types only"); return 0; }
    if (!write_format(output, "%s ", text_equals(function->as.function_declaration.name, "main") ? "int" : "int32_t") || !write_text(output, function->as.function_declaration.name) || !write_format(output, "(")) return 0;
    if (function->as.function_declaration.parameter_count == 0) { if (!write_format(output, "void")) return 0; }
    else if (!emit_parameters(function, output, backend)) return 0;
    return write_format(output, prototype ? ");\n" : ")\n");
}

static int emit_block(const JslAstNode *block, const JslLocal *parent, FILE *output, JslCBackend *backend) {
    JslLocal *locals = (JslLocal *)parent;
    const JslAstNodeList *statements = &block->as.block_statement.statements;
    int success = write_format(output, "{\n");
    for (size_t i = 0; success && i < statements->count; i++) {
        const JslAstNode *statement = statements->items[i];
        if (statement->kind == JSL_AST_VARIABLE_STATEMENT) {
            JslCValueType type = expression_type(statement->as.variable_statement.initializer, locals);
            if (statement->as.variable_statement.type_name.start != NULL) type = text_equals(statement->as.variable_statement.type_name, "i32") ? JSL_C_VALUE_I32 : text_equals(statement->as.variable_statement.type_name, "bool") ? JSL_C_VALUE_BOOL : text_equals(statement->as.variable_statement.type_name, "string") ? JSL_C_VALUE_STRING : JSL_C_VALUE_STRUCT;
            if (type == JSL_C_VALUE_INVALID) { set_error(backend, statement->position, "unsupported local variable in v0.0.9 backend"); success = 0; break; }
            const char *qualifier = !statement->as.variable_statement.is_mutable && type == JSL_C_VALUE_I32 ? "const " : "";
            const char *c_type = type == JSL_C_VALUE_I32 ? "int32_t" : type == JSL_C_VALUE_BOOL ? "bool" : type == JSL_C_VALUE_STRING ? "const char *" : NULL;
            success = write_format(output, "    %s", qualifier) && (c_type != NULL ? write_format(output, "%s ", c_type) : write_text(output, statement->as.variable_statement.type_name) && write_format(output, " ")) && write_text(output, statement->as.variable_statement.name) && write_format(output, " = ") && emit_expression(statement->as.variable_statement.initializer, locals, output, backend) && write_format(output, ";\n") && add_local(&locals, statement->as.variable_statement.name, type, backend, statement->position);
        } else if (statement->kind == JSL_AST_RETURN_STATEMENT) {
            success = statement->as.return_statement.value != NULL && write_format(output, "    return ") && emit_expression(statement->as.return_statement.value, locals, output, backend) && write_format(output, ";\n");
            if (!success && backend->error_message == NULL) set_error(backend, statement->position, "v0.0.9 backend requires return values");
        } else if (statement->kind == JSL_AST_EXPRESSION_STATEMENT && statement->as.expression_statement.expression->kind == JSL_AST_CALL_EXPRESSION) {
            int result = emit_console_statement(statement->as.expression_statement.expression, locals, output, backend);
            if (result == 0) { set_error(backend, statement->position, "v0.0.9 backend supports console output calls only as expression statements"); success = 0; }
            else success = result > 0;
        } else if (statement->kind == JSL_AST_IF_STATEMENT) {
            if (expression_type(statement->as.if_statement.condition, locals) != JSL_C_VALUE_BOOL) { set_error(backend, statement->as.if_statement.condition->position, "if condition must be bool in v0.0.9 backend"); success = 0; }
            else {
                success = write_format(output, "    if (") && emit_expression(statement->as.if_statement.condition, locals, output, backend) && write_format(output, ") ");
                if (success) success = emit_block(statement->as.if_statement.then_branch, locals, output, backend);
                if (success && statement->as.if_statement.else_branch != NULL) success = write_format(output, "    else ") && emit_block(statement->as.if_statement.else_branch, locals, output, backend);
            }
        } else { set_error(backend, statement->position, "unsupported statement in v0.0.9 backend"); success = 0; }
    }
    if (success) success = write_format(output, "}\n");
    free_locals_until(locals, parent);
    return success;
}

static int emit_function_body(const JslAstNode *function, FILE *output, JslCBackend *backend) {
    JslLocal *locals = NULL;
    for (size_t i = 0; i < function->as.function_declaration.parameter_count; i++) if (!add_local(&locals, function->as.function_declaration.parameters[i].name, JSL_C_VALUE_I32, backend, function->as.function_declaration.parameters[i].position)) { free_locals(locals); return 0; }
    int success = emit_block(function->as.function_declaration.body, locals, output, backend) && write_format(output, "\n");
    free_locals(locals);
    return success;
}

static int emit_console_runtime(FILE *output) {
    return write_format(output,
        "static void jsl_console_log_i32(int32_t value) { printf(\"%%d\\n\", (int)value); }\n"
        "static void jsl_console_log_string(const char *value) { printf(\"%%s\\n\", value); }\n"
        "static void jsl_console_info_i32(int32_t value) { printf(\"%%d\\n\", (int)value); }\n"
        "static void jsl_console_info_string(const char *value) { printf(\"%%s\\n\", value); }\n"
        "static void jsl_console_warn_i32(int32_t value) { fprintf(stderr, \"%%d\\n\", (int)value); }\n"
        "static void jsl_console_warn_string(const char *value) { fprintf(stderr, \"%%s\\n\", value); }\n"
        "static void jsl_console_error_i32(int32_t value) { fprintf(stderr, \"%%d\\n\", (int)value); }\n"
        "static void jsl_console_error_string(const char *value) { fprintf(stderr, \"%%s\\n\", value); }\n"
        "static void jsl_console_write_i32(int32_t value) { printf(\"%%d\", (int)value); }\n"
        "static void jsl_console_write_string(const char *value) { printf(\"%%s\", value); }\n"
        "static char *jsl_console_read_line(void) { char *value = malloc(4096); if (value == NULL) return NULL; if (fgets(value, 4096, stdin) == NULL) { value[0] = '\\0'; return value; } value[strcspn(value, \"\\n\")] = '\\0'; return value; }\n\n");
}

static int emit_struct_declaration(const JslAstNode *declaration, FILE *output, JslCBackend *backend) {
    if (!write_format(output, "typedef struct ") || !write_text(output, declaration->as.struct_declaration.name) || !write_format(output, " {\n")) return 0;
    for (size_t i = 0; i < declaration->as.struct_declaration.field_count; i++) {
        const JslAstField *field = &declaration->as.struct_declaration.fields[i];
        const char *type = text_equals(field->type_name, "i32") ? "int32_t" : text_equals(field->type_name, "string") ? "const char *" : NULL;
        if (!write_format(output, "    ") || (type != NULL ? !write_format(output, "%s ", type) : !write_text(output, field->type_name)) || !write_text(output, field->name) || !write_format(output, ";\n")) { set_error(backend, field->position, "unable to emit struct field"); return 0; }
    }
    return write_format(output, "} ") && write_text(output, declaration->as.struct_declaration.name) && write_format(output, ";\n\n");
}

int jsl_c_backend_emit(const JslAstProgram *program, FILE *output, JslCBackend *backend) {
    size_t main_count = 0;
    if (!write_format(output, "#include <stdbool.h>\n#include <stdint.h>\n#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n\n") || !emit_console_runtime(output)) return 0;
    for (size_t i = 0; i < program->declarations.count; i++) if (program->declarations.items[i]->kind == JSL_AST_STRUCT_DECLARATION && !emit_struct_declaration(program->declarations.items[i], output, backend)) return 0;
    for (size_t i = 0; i < program->declarations.count; i++) {
        const JslAstNode *function = program->declarations.items[i];
        if (function->kind == JSL_AST_STRUCT_DECLARATION) continue;
        if (function->kind != JSL_AST_FUNCTION_DECLARATION) { set_error(backend, function->position, "v0.0.9 backend supports structs and function declarations only"); return 0; }
        if (text_equals(function->as.function_declaration.name, "main")) { main_count++; if (function->as.function_declaration.parameter_count != 0) { set_error(backend, function->position, "main function cannot have parameters"); return 0; } }
        if (!emit_function_signature(function, output, backend, 1)) return 0;
    }
    if (main_count != 1) { set_error(backend, program->declarations.count == 0 ? (JslPosition){"<input>", 1, 1} : program->declarations.items[0]->position, "v0.0.9 backend requires exactly one main function"); return 0; }
    if (!write_format(output, "\n")) return 0;
    for (size_t i = 0; i < program->declarations.count; i++) if (program->declarations.items[i]->kind == JSL_AST_FUNCTION_DECLARATION && (!emit_function_signature(program->declarations.items[i], output, backend, 0) || !emit_function_body(program->declarations.items[i], output, backend))) return 0;
    return 1;
}
