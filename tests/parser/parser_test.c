#include "compiler/parser/parser.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static int text_equals(JslAstText text, const char *expected) {
    return text.length == strlen(expected) && strncmp(text.start, expected, text.length) == 0;
}

static void test_function_declaration(void) {
    const char *source = "function add(left: i32, right: i32): i32 {\n"
                         "  const total: i32 = left + right * 2;\n"
                         "  return total;\n"
                         "}\n";
    JslParser parser;
    JslAstProgram program;
    jsl_parser_init(&parser, "add.jsl", source);
    assert(jsl_parser_parse_program(&parser, &program));
    assert(program.declarations.count == 1);

    JslAstNode *function = program.declarations.items[0];
    assert(function->kind == JSL_AST_FUNCTION_DECLARATION);
    assert(function->position.line == 1 && function->position.column == 1);
    assert(text_equals(function->as.function_declaration.name, "add"));
    assert(function->as.function_declaration.parameter_count == 2);
    assert(text_equals(function->as.function_declaration.parameters[0].name, "left"));
    assert(text_equals(function->as.function_declaration.parameters[0].type_name, "i32"));
    assert(text_equals(function->as.function_declaration.return_type, "i32"));

    JslAstNode *body = function->as.function_declaration.body;
    assert(body->kind == JSL_AST_BLOCK_STATEMENT);
    assert(body->as.block_statement.statements.count == 2);
    JslAstNode *variable = body->as.block_statement.statements.items[0];
    assert(variable->kind == JSL_AST_VARIABLE_STATEMENT);
    assert(!variable->as.variable_statement.is_mutable);
    assert(variable->as.variable_statement.initializer->kind == JSL_AST_BINARY_EXPRESSION);
    assert(text_equals(variable->as.variable_statement.initializer->as.binary_expression.operator_text, "+"));
    assert(variable->as.variable_statement.initializer->as.binary_expression.right->kind == JSL_AST_BINARY_EXPRESSION);
    assert(text_equals(variable->as.variable_statement.initializer->as.binary_expression.right->as.binary_expression.operator_text, "*"));
    assert(body->as.block_statement.statements.items[1]->kind == JSL_AST_RETURN_STATEMENT);
    jsl_ast_program_free(&program);
}

static void test_if_calls_and_unary_expressions(void) {
    const char *source = "function main(): i32 {\n"
                         "  console.info(\"starting\");\n"
                         "  let result = calculate(20, -1);\n"
                         "  if (result > 0 && !false) { return result; } else { return 0; }\n"
                         "}\n";
    JslParser parser;
    JslAstProgram program;
    jsl_parser_init(&parser, "main.jsl", source);
    assert(jsl_parser_parse_program(&parser, &program));
    JslAstNode *body = program.declarations.items[0]->as.function_declaration.body;
    JslAstNode *log = body->as.block_statement.statements.items[0];
    assert(log->kind == JSL_AST_EXPRESSION_STATEMENT);
    assert(log->as.expression_statement.expression->kind == JSL_AST_CALL_EXPRESSION);
    assert(log->as.expression_statement.expression->as.call_expression.callee->kind == JSL_AST_MEMBER_EXPRESSION);
    JslAstNode *variable = body->as.block_statement.statements.items[1];
    assert(variable->as.variable_statement.is_mutable);
    JslAstNode *call = variable->as.variable_statement.initializer;
    assert(call->kind == JSL_AST_CALL_EXPRESSION);
    assert(call->as.call_expression.arguments.count == 2);
    assert(call->as.call_expression.arguments.items[1]->kind == JSL_AST_UNARY_EXPRESSION);
    JslAstNode *conditional = body->as.block_statement.statements.items[2];
    assert(conditional->kind == JSL_AST_IF_STATEMENT);
    assert(conditional->as.if_statement.condition->kind == JSL_AST_BINARY_EXPRESSION);
    assert(conditional->as.if_statement.else_branch != NULL);
    jsl_ast_program_free(&program);
}

static void test_conditional_expression(void) {
    JslParser parser;
    JslAstProgram program;
    jsl_parser_init(&parser, "conditional.jsl", "function select(value: i32): i32 { return value > 0 ? value : -value; }");
    assert(jsl_parser_parse_program(&parser, &program));
    JslAstNode *value = program.declarations.items[0]->as.function_declaration.body->as.block_statement.statements.items[0]->as.return_statement.value;
    assert(value->kind == JSL_AST_CONDITIONAL_EXPRESSION);
    assert(value->as.conditional_expression.condition->kind == JSL_AST_BINARY_EXPRESSION);
    assert(value->as.conditional_expression.then_expression->kind == JSL_AST_IDENTIFIER_EXPRESSION);
    assert(value->as.conditional_expression.else_expression->kind == JSL_AST_UNARY_EXPRESSION);
    jsl_ast_program_free(&program);
}

static void test_import_and_export(void) {
    JslParser parser;
    JslAstProgram program;
    jsl_parser_init(&parser, "main.jsl", "import { add, subtract } from \"./math.jsl\"; export function main(): i32 { return add(20, 22); }");
    assert(jsl_parser_parse_program(&parser, &program));
    assert(program.declarations.count == 2);
    JslAstNode *import_declaration = program.declarations.items[0];
    assert(import_declaration->kind == JSL_AST_IMPORT_DECLARATION);
    assert(import_declaration->as.import_declaration.name_count == 2);
    assert(text_equals(import_declaration->as.import_declaration.names[0], "add"));
    assert(text_equals(import_declaration->as.import_declaration.module_path, "./math.jsl"));
    JslAstNode *function = program.declarations.items[1];
    assert(function->kind == JSL_AST_FUNCTION_DECLARATION);
    assert(function->as.function_declaration.is_exported);
    jsl_ast_program_free(&program);
}

static void test_parser_error_has_position(void) {
    JslParser parser;
    JslAstProgram program;
    jsl_parser_init(&parser, "bad.jsl", "function main(): i32 { return 1 }");
    assert(!jsl_parser_parse_program(&parser, &program));
    JslPosition position;
    assert(strcmp(jsl_parser_error(&parser, &position), "expected ';' after return value") == 0);
    assert(strcmp(position.filename, "bad.jsl") == 0);
    assert(position.line == 1 && position.column == 33);
}

int main(void) {
    test_function_declaration();
    test_if_calls_and_unary_expressions();
    test_conditional_expression();
    test_import_and_export();
    test_parser_error_has_position();
    puts("parser tests passed");
    return 0;
}
