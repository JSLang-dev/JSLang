#include "compiler/checker/checker.h"
#include "compiler/types/types.h"

#include <stdlib.h>
#include <string.h>

typedef struct JslSymbol JslSymbol;
typedef struct JslScope JslScope;
struct JslSymbol { JslAstText name; JslSymbolKind kind; JslType type; const JslAstNode *function; JslSymbol *next; };
struct JslScope { JslScope *parent; JslSymbol *symbols; };

static int text_equals(JslAstText left, JslAstText right) { return left.length == right.length && memcmp(left.start, right.start, left.length) == 0; }
static void set_error(JslChecker *checker, JslPosition position, const char *message) { if (!checker->had_error) { checker->had_error = 1; checker->error_position = position; checker->error_message = message; } }
static JslScope *scope_new(JslScope *parent) { JslScope *scope = calloc(1, sizeof(*scope)); if (scope != NULL) scope->parent = parent; return scope; }
static void scope_free(JslScope *scope) { if (scope == NULL) return; for (JslSymbol *symbol = scope->symbols; symbol != NULL;) { JslSymbol *next = symbol->next; free(symbol); symbol = next; } free(scope); }
static JslSymbol *scope_find_local(const JslScope *scope, JslAstText name) { for (JslSymbol *symbol = scope->symbols; symbol != NULL; symbol = symbol->next) if (text_equals(symbol->name, name)) return symbol; return NULL; }
static JslSymbol *scope_find(const JslScope *scope, JslAstText name) { for (; scope != NULL; scope = scope->parent) { JslSymbol *symbol = scope_find_local(scope, name); if (symbol != NULL) return symbol; } return NULL; }

static int scope_declare(JslChecker *checker, JslScope *scope, JslAstText name, JslSymbolKind kind, JslType type, const JslAstNode *function, JslPosition position) {
    if (scope_find_local(scope, name) != NULL) { set_error(checker, position, "duplicate declaration in this scope"); return 0; }
    JslSymbol *symbol = malloc(sizeof(*symbol));
    if (symbol == NULL) { set_error(checker, position, "out of memory"); return 0; }
    *symbol = (JslSymbol){name, kind, type, function, scope->symbols}; scope->symbols = symbol; return 1;
}

static JslType check_expression(JslChecker *checker, JslScope *scope, const JslAstNode *node);
static int compatible(JslType expected, JslType actual) { return expected == JSL_TYPE_UNKNOWN || actual == JSL_TYPE_UNKNOWN || expected == actual; }
static JslType named_type(JslChecker *checker, JslAstText name, JslPosition position) { JslType type = jsl_type_from_name(name); if (type == JSL_TYPE_UNKNOWN) set_error(checker, position, "unknown type"); return type; }

static JslType check_call(JslChecker *checker, JslScope *scope, const JslAstNode *node) {
    const JslAstNode *callee = node->as.call_expression.callee;
    JslSymbol *symbol = NULL;
    if (callee->kind == JSL_AST_IDENTIFIER_EXPRESSION) symbol = scope_find(scope, callee->as.identifier_expression.name);
    else check_expression(checker, scope, callee);
    if (checker->had_error) return JSL_TYPE_UNKNOWN;
    for (size_t i = 0; i < node->as.call_expression.arguments.count; i++) {
        if (check_expression(checker, scope, node->as.call_expression.arguments.items[i]) == JSL_TYPE_UNKNOWN && checker->had_error) return JSL_TYPE_UNKNOWN;
    }
    if (symbol == NULL || symbol->kind == JSL_SYMBOL_IMPORT || symbol->kind == JSL_SYMBOL_BUILTIN) return JSL_TYPE_UNKNOWN;
    if (symbol->kind != JSL_SYMBOL_FUNCTION) { set_error(checker, callee->position, "cannot call non-function value"); return JSL_TYPE_UNKNOWN; }
    const JslAstNode *function = symbol->function;
    if (function->as.function_declaration.parameter_count != node->as.call_expression.arguments.count) { set_error(checker, node->position, "incorrect number of function arguments"); return JSL_TYPE_UNKNOWN; }
    for (size_t i = 0; i < node->as.call_expression.arguments.count; i++) {
        JslType expected = named_type(checker, function->as.function_declaration.parameters[i].type_name, function->as.function_declaration.parameters[i].position);
        JslType actual = check_expression(checker, scope, node->as.call_expression.arguments.items[i]);
        if (checker->had_error) return JSL_TYPE_UNKNOWN;
        if (!compatible(expected, actual)) { set_error(checker, node->as.call_expression.arguments.items[i]->position, "function argument type does not match parameter type"); return JSL_TYPE_UNKNOWN; }
    }
    return named_type(checker, function->as.function_declaration.return_type, function->position);
}

static JslType check_expression(JslChecker *checker, JslScope *scope, const JslAstNode *node) {
    JslType left, right;
    switch (node->kind) {
        case JSL_AST_IDENTIFIER_EXPRESSION: {
            JslSymbol *symbol = scope_find(scope, node->as.identifier_expression.name);
            if (symbol == NULL) { set_error(checker, node->position, "undefined variable"); return JSL_TYPE_UNKNOWN; }
            return symbol->kind == JSL_SYMBOL_FUNCTION ? named_type(checker, symbol->function->as.function_declaration.return_type, symbol->function->position) : symbol->type;
        }
        case JSL_AST_LITERAL_EXPRESSION:
            switch (node->as.literal_expression.kind) { case JSL_AST_LITERAL_INTEGER: return JSL_TYPE_I32; case JSL_AST_LITERAL_FLOAT: return JSL_TYPE_F64; case JSL_AST_LITERAL_STRING: return JSL_TYPE_STRING; case JSL_AST_LITERAL_TRUE: case JSL_AST_LITERAL_FALSE: return JSL_TYPE_BOOL; case JSL_AST_LITERAL_NULL: return JSL_TYPE_NULL; }
            break;
        case JSL_AST_UNARY_EXPRESSION:
            left = check_expression(checker, scope, node->as.unary_expression.operand); if (checker->had_error) return JSL_TYPE_UNKNOWN;
            if (node->as.unary_expression.operator_text.start[0] == '!') { if (left != JSL_TYPE_UNKNOWN && left != JSL_TYPE_BOOL) set_error(checker, node->position, "'!' requires a bool operand"); return JSL_TYPE_BOOL; }
            if (left != JSL_TYPE_UNKNOWN && !jsl_type_is_numeric(left)) set_error(checker, node->position, "unary numeric operator requires a numeric operand"); return left;
        case JSL_AST_BINARY_EXPRESSION: {
            left = check_expression(checker, scope, node->as.binary_expression.left); right = check_expression(checker, scope, node->as.binary_expression.right); if (checker->had_error) return JSL_TYPE_UNKNOWN;
            char operator = node->as.binary_expression.operator_text.start[0];
            if (operator == '&' || operator == '|') { if ((left != JSL_TYPE_UNKNOWN && left != JSL_TYPE_BOOL) || (right != JSL_TYPE_UNKNOWN && right != JSL_TYPE_BOOL)) set_error(checker, node->position, "logical operator requires bool operands"); return JSL_TYPE_BOOL; }
            if (operator == '=' || operator == '!') { if (!compatible(left, right)) set_error(checker, node->position, "equality operator requires matching operand types"); return JSL_TYPE_BOOL; }
            if (operator == '<' || operator == '>') { if ((left != JSL_TYPE_UNKNOWN && !jsl_type_is_numeric(left)) || (right != JSL_TYPE_UNKNOWN && !jsl_type_is_numeric(right)) || !compatible(left, right)) set_error(checker, node->position, "comparison operator requires matching numeric operands"); return JSL_TYPE_BOOL; }
            if ((left != JSL_TYPE_UNKNOWN && !jsl_type_is_numeric(left)) || (right != JSL_TYPE_UNKNOWN && !jsl_type_is_numeric(right)) || !compatible(left, right)) set_error(checker, node->position, "arithmetic operator requires matching numeric operands");
            return left;
        }
        case JSL_AST_CONDITIONAL_EXPRESSION:
            left = check_expression(checker, scope, node->as.conditional_expression.condition); if (left != JSL_TYPE_UNKNOWN && left != JSL_TYPE_BOOL) set_error(checker, node->as.conditional_expression.condition->position, "conditional expression requires a bool condition");
            right = check_expression(checker, scope, node->as.conditional_expression.then_expression); JslType alternate = check_expression(checker, scope, node->as.conditional_expression.else_expression);
            if (!checker->had_error && !compatible(right, alternate)) set_error(checker, node->position, "conditional branches must have matching types"); return right;
        case JSL_AST_MEMBER_EXPRESSION: check_expression(checker, scope, node->as.member_expression.object); return JSL_TYPE_UNKNOWN;
        case JSL_AST_CALL_EXPRESSION: return check_call(checker, scope, node);
        case JSL_AST_GROUPING_EXPRESSION: return check_expression(checker, scope, node->as.grouping_expression.expression);
        default: set_error(checker, node->position, "invalid expression"); return JSL_TYPE_UNKNOWN;
    }
    set_error(checker, node->position, "invalid literal"); return JSL_TYPE_UNKNOWN;
}

static int check_block(JslChecker *checker, JslScope *parent, const JslAstNode *block, JslType return_type) {
    JslScope *scope = scope_new(parent); if (scope == NULL) { set_error(checker, block->position, "out of memory"); return 0; }
    for (size_t i = 0; i < block->as.block_statement.statements.count && !checker->had_error; i++) {
        const JslAstNode *node = block->as.block_statement.statements.items[i]; JslType actual, expected;
        switch (node->kind) {
            case JSL_AST_VARIABLE_STATEMENT:
                actual = check_expression(checker, scope, node->as.variable_statement.initializer); if (checker->had_error) break;
                expected = node->as.variable_statement.type_name.start == NULL ? actual : named_type(checker, node->as.variable_statement.type_name, node->position);
                if (!checker->had_error && !compatible(expected, actual)) set_error(checker, node->position, "variable initializer type does not match declaration type");
                if (!checker->had_error) scope_declare(checker, scope, node->as.variable_statement.name, JSL_SYMBOL_VARIABLE, expected, NULL, node->position);
                break;
            case JSL_AST_RETURN_STATEMENT:
                actual = node->as.return_statement.value == NULL ? JSL_TYPE_VOID : check_expression(checker, scope, node->as.return_statement.value);
                if (!checker->had_error && !compatible(return_type, actual)) set_error(checker, node->position, "return type does not match function return type"); break;
            case JSL_AST_EXPRESSION_STATEMENT: check_expression(checker, scope, node->as.expression_statement.expression); break;
            case JSL_AST_IF_STATEMENT:
                actual = check_expression(checker, scope, node->as.if_statement.condition);
                if (!checker->had_error && actual != JSL_TYPE_UNKNOWN && actual != JSL_TYPE_BOOL) set_error(checker, node->as.if_statement.condition->position, "if condition must be bool");
                if (!checker->had_error) check_block(checker, scope, node->as.if_statement.then_branch, return_type);
                if (!checker->had_error && node->as.if_statement.else_branch != NULL) check_block(checker, scope, node->as.if_statement.else_branch, return_type);
                break;
            default: set_error(checker, node->position, "invalid statement in block"); break;
        }
    }
    scope_free(scope); return !checker->had_error;
}

static int declare_global_symbols(JslChecker *checker, JslScope *global, const JslAstProgram *program) {
    for (size_t i = 0; i < program->declarations.count && !checker->had_error; i++) {
        const JslAstNode *node = program->declarations.items[i];
        if (node->kind == JSL_AST_IMPORT_DECLARATION) for (size_t j = 0; j < node->as.import_declaration.name_count && !checker->had_error; j++) scope_declare(checker, global, node->as.import_declaration.names[j], JSL_SYMBOL_IMPORT, JSL_TYPE_UNKNOWN, NULL, node->position);
        else if (node->kind == JSL_AST_FUNCTION_DECLARATION) {
            named_type(checker, node->as.function_declaration.return_type, node->position);
            for (size_t j = 0; j < node->as.function_declaration.parameter_count && !checker->had_error; j++) named_type(checker, node->as.function_declaration.parameters[j].type_name, node->as.function_declaration.parameters[j].position);
            if (!checker->had_error) scope_declare(checker, global, node->as.function_declaration.name, JSL_SYMBOL_FUNCTION, JSL_TYPE_UNKNOWN, node, node->position);
        } else set_error(checker, node->position, "invalid top-level declaration");
    }
    return !checker->had_error;
}

static int check_function(JslChecker *checker, JslScope *global, const JslAstNode *function) {
    JslScope *scope = scope_new(global); if (scope == NULL) { set_error(checker, function->position, "out of memory"); return 0; }
    for (size_t i = 0; i < function->as.function_declaration.parameter_count && !checker->had_error; i++) { const JslAstParameter *parameter = &function->as.function_declaration.parameters[i]; scope_declare(checker, scope, parameter->name, JSL_SYMBOL_PARAMETER, named_type(checker, parameter->type_name, parameter->position), NULL, parameter->position); }
    if (!checker->had_error) check_block(checker, scope, function->as.function_declaration.body, named_type(checker, function->as.function_declaration.return_type, function->position));
    scope_free(scope); return !checker->had_error;
}

void jsl_checker_init(JslChecker *checker) { *checker = (JslChecker){NULL, {NULL, 0, 0}, 0}; }
int jsl_checker_check_program(JslChecker *checker, const JslAstProgram *program) {
    JslScope *global = scope_new(NULL); if (global == NULL) { set_error(checker, (JslPosition){"<unknown>", 0, 0}, "out of memory"); return 0; }
    scope_declare(checker, global, (JslAstText){"console", 7}, JSL_SYMBOL_BUILTIN, JSL_TYPE_UNKNOWN, NULL, (JslPosition){"<builtin>", 1, 1});
    if (!checker->had_error) declare_global_symbols(checker, global, program);
    for (size_t i = 0; i < program->declarations.count && !checker->had_error; i++) if (program->declarations.items[i]->kind == JSL_AST_FUNCTION_DECLARATION) check_function(checker, global, program->declarations.items[i]);
    scope_free(global); return !checker->had_error;
}
const char *jsl_checker_error(const JslChecker *checker, JslPosition *position) { if (position != NULL) *position = checker->error_position; return checker->error_message; }
