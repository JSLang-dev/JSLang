#include "compiler/checker/checker.h"

#include <stdlib.h>
#include <string.h>

typedef struct JslSymbol JslSymbol;
typedef struct JslScope JslScope;

struct JslSymbol {
    JslAstText name;
    JslSymbolKind kind;
    JslSymbol *next;
};

struct JslScope {
    JslScope *parent;
    JslSymbol *symbols;
};

static int text_equals(JslAstText left, JslAstText right) {
    return left.length == right.length && memcmp(left.start, right.start, left.length) == 0;
}

static void set_error(JslChecker *checker, JslPosition position, const char *message) {
    if (!checker->had_error) {
        checker->had_error = 1;
        checker->error_position = position;
        checker->error_message = message;
    }
}

static JslScope *scope_new(JslScope *parent) {
    JslScope *scope = calloc(1, sizeof(*scope));
    if (scope != NULL) scope->parent = parent;
    return scope;
}

static void scope_free(JslScope *scope) {
    if (scope == NULL) return;
    JslSymbol *symbol = scope->symbols;
    while (symbol != NULL) {
        JslSymbol *next = symbol->next;
        free(symbol);
        symbol = next;
    }
    free(scope);
}

static JslSymbol *scope_find_local(const JslScope *scope, JslAstText name) {
    for (JslSymbol *symbol = scope->symbols; symbol != NULL; symbol = symbol->next) {
        if (text_equals(symbol->name, name)) return symbol;
    }
    return NULL;
}

static JslSymbol *scope_find(const JslScope *scope, JslAstText name) {
    for (; scope != NULL; scope = scope->parent) {
        JslSymbol *symbol = scope_find_local(scope, name);
        if (symbol != NULL) return symbol;
    }
    return NULL;
}

static int scope_declare(JslChecker *checker, JslScope *scope, JslAstText name, JslSymbolKind kind, JslPosition position) {
    if (scope_find_local(scope, name) != NULL) {
        set_error(checker, position, "duplicate declaration in this scope");
        return 0;
    }
    JslSymbol *symbol = malloc(sizeof(*symbol));
    if (symbol == NULL) {
        set_error(checker, position, "out of memory");
        return 0;
    }
    *symbol = (JslSymbol){name, kind, scope->symbols};
    scope->symbols = symbol;
    return 1;
}

static int check_expression(JslChecker *checker, JslScope *scope, const JslAstNode *node);

static int check_expression_list(JslChecker *checker, JslScope *scope, const JslAstNodeList *list) {
    for (size_t i = 0; i < list->count; i++) {
        if (!check_expression(checker, scope, list->items[i])) return 0;
    }
    return 1;
}

static int check_block(JslChecker *checker, JslScope *parent, const JslAstNode *block) {
    JslScope *scope = scope_new(parent);
    if (scope == NULL) { set_error(checker, block->position, "out of memory"); return 0; }
    for (size_t i = 0; i < block->as.block_statement.statements.count && !checker->had_error; i++) {
        const JslAstNode *node = block->as.block_statement.statements.items[i];
        switch (node->kind) {
            case JSL_AST_VARIABLE_STATEMENT:
                if (!check_expression(checker, scope, node->as.variable_statement.initializer)) break;
                scope_declare(checker, scope, node->as.variable_statement.name, JSL_SYMBOL_VARIABLE, node->position);
                break;
            case JSL_AST_RETURN_STATEMENT:
                if (node->as.return_statement.value != NULL) check_expression(checker, scope, node->as.return_statement.value);
                break;
            case JSL_AST_EXPRESSION_STATEMENT: check_expression(checker, scope, node->as.expression_statement.expression); break;
            case JSL_AST_IF_STATEMENT:
                if (check_expression(checker, scope, node->as.if_statement.condition)) {
                    check_block(checker, scope, node->as.if_statement.then_branch);
                    if (!checker->had_error && node->as.if_statement.else_branch != NULL) check_block(checker, scope, node->as.if_statement.else_branch);
                }
                break;
            default: set_error(checker, node->position, "invalid statement in block"); break;
        }
    }
    scope_free(scope);
    return !checker->had_error;
}

static int check_expression(JslChecker *checker, JslScope *scope, const JslAstNode *node) {
    switch (node->kind) {
        case JSL_AST_IDENTIFIER_EXPRESSION:
            if (scope_find(scope, node->as.identifier_expression.name) == NULL) {
                set_error(checker, node->position, "undefined variable");
                return 0;
            }
            return 1;
        case JSL_AST_LITERAL_EXPRESSION: return 1;
        case JSL_AST_UNARY_EXPRESSION: return check_expression(checker, scope, node->as.unary_expression.operand);
        case JSL_AST_BINARY_EXPRESSION:
            return check_expression(checker, scope, node->as.binary_expression.left) && check_expression(checker, scope, node->as.binary_expression.right);
        case JSL_AST_CONDITIONAL_EXPRESSION:
            return check_expression(checker, scope, node->as.conditional_expression.condition) && check_expression(checker, scope, node->as.conditional_expression.then_expression) && check_expression(checker, scope, node->as.conditional_expression.else_expression);
        case JSL_AST_MEMBER_EXPRESSION: return check_expression(checker, scope, node->as.member_expression.object);
        case JSL_AST_CALL_EXPRESSION:
            return check_expression(checker, scope, node->as.call_expression.callee) && check_expression_list(checker, scope, &node->as.call_expression.arguments);
        case JSL_AST_GROUPING_EXPRESSION: return check_expression(checker, scope, node->as.grouping_expression.expression);
        default: set_error(checker, node->position, "invalid expression"); return 0;
    }
}

static int declare_global_symbols(JslChecker *checker, JslScope *global, const JslAstProgram *program) {
    for (size_t i = 0; i < program->declarations.count && !checker->had_error; i++) {
        const JslAstNode *node = program->declarations.items[i];
        if (node->kind == JSL_AST_IMPORT_DECLARATION) {
            for (size_t j = 0; j < node->as.import_declaration.name_count; j++) {
                scope_declare(checker, global, node->as.import_declaration.names[j], JSL_SYMBOL_IMPORT, node->position);
                if (checker->had_error) break;
            }
        } else if (node->kind == JSL_AST_FUNCTION_DECLARATION) {
            scope_declare(checker, global, node->as.function_declaration.name, JSL_SYMBOL_FUNCTION, node->position);
        } else {
            set_error(checker, node->position, "invalid top-level declaration");
        }
    }
    return !checker->had_error;
}

static int check_function(JslChecker *checker, JslScope *global, const JslAstNode *function) {
    JslScope *scope = scope_new(global);
    if (scope == NULL) { set_error(checker, function->position, "out of memory"); return 0; }
    for (size_t i = 0; i < function->as.function_declaration.parameter_count && !checker->had_error; i++) {
        const JslAstParameter *parameter = &function->as.function_declaration.parameters[i];
        scope_declare(checker, scope, parameter->name, JSL_SYMBOL_PARAMETER, parameter->position);
    }
    if (!checker->had_error) check_block(checker, scope, function->as.function_declaration.body);
    scope_free(scope);
    return !checker->had_error;
}

void jsl_checker_init(JslChecker *checker) {
    *checker = (JslChecker){NULL, {NULL, 0, 0}, 0};
}

int jsl_checker_check_program(JslChecker *checker, const JslAstProgram *program) {
    JslScope *global = scope_new(NULL);
    if (global == NULL) { set_error(checker, (JslPosition){"<unknown>", 0, 0}, "out of memory"); return 0; }
    const JslAstText console_name = {"console", 7};
    scope_declare(checker, global, console_name, JSL_SYMBOL_BUILTIN, (JslPosition){"<builtin>", 1, 1});
    if (!checker->had_error) declare_global_symbols(checker, global, program);
    for (size_t i = 0; i < program->declarations.count && !checker->had_error; i++) {
        const JslAstNode *node = program->declarations.items[i];
        if (node->kind == JSL_AST_FUNCTION_DECLARATION) check_function(checker, global, node);
    }
    scope_free(global);
    return !checker->had_error;
}

const char *jsl_checker_error(const JslChecker *checker, JslPosition *position) {
    if (position != NULL) *position = checker->error_position;
    return checker->error_message;
}
