#include "compiler/ast/ast.h"

#include <stdlib.h>

JslAstNode *jsl_ast_new_node(JslAstKind kind, JslPosition position) {
    JslAstNode *node = calloc(1, sizeof(*node));
    if (node != NULL) {
        node->kind = kind;
        node->position = position;
    }
    return node;
}

int jsl_ast_node_list_append(JslAstNodeList *list, JslAstNode *node) {
    if (list->count == list->capacity) {
        size_t capacity = list->capacity == 0 ? 4 : list->capacity * 2;
        JslAstNode **items = realloc(list->items, capacity * sizeof(*items));
        if (items == NULL) return 0;
        list->items = items;
        list->capacity = capacity;
    }
    list->items[list->count++] = node;
    return 1;
}

static void free_list(JslAstNodeList *list) {
    for (size_t i = 0; i < list->count; i++) jsl_ast_node_free(list->items[i]);
    free(list->items);
}

void jsl_ast_node_free(JslAstNode *node) {
    if (node == NULL) return;
    switch (node->kind) {
        case JSL_AST_IMPORT_DECLARATION: free(node->as.import_declaration.names); break;
        case JSL_AST_STRUCT_DECLARATION: free(node->as.struct_declaration.fields); break;
        case JSL_AST_FUNCTION_DECLARATION:
            free(node->as.function_declaration.parameters);
            jsl_ast_node_free(node->as.function_declaration.body);
            break;
        case JSL_AST_BLOCK_STATEMENT: free_list(&node->as.block_statement.statements); break;
        case JSL_AST_VARIABLE_STATEMENT: jsl_ast_node_free(node->as.variable_statement.initializer); break;
        case JSL_AST_ASSIGNMENT_STATEMENT: jsl_ast_node_free(node->as.assignment_statement.value); break;
        case JSL_AST_RETURN_STATEMENT: jsl_ast_node_free(node->as.return_statement.value); break;
        case JSL_AST_EXPRESSION_STATEMENT: jsl_ast_node_free(node->as.expression_statement.expression); break;
        case JSL_AST_IF_STATEMENT:
            jsl_ast_node_free(node->as.if_statement.condition);
            jsl_ast_node_free(node->as.if_statement.then_branch);
            jsl_ast_node_free(node->as.if_statement.else_branch);
            break;
        case JSL_AST_UNARY_EXPRESSION: jsl_ast_node_free(node->as.unary_expression.operand); break;
        case JSL_AST_BINARY_EXPRESSION:
            jsl_ast_node_free(node->as.binary_expression.left);
            jsl_ast_node_free(node->as.binary_expression.right);
            break;
        case JSL_AST_CONDITIONAL_EXPRESSION:
            jsl_ast_node_free(node->as.conditional_expression.condition);
            jsl_ast_node_free(node->as.conditional_expression.then_expression);
            jsl_ast_node_free(node->as.conditional_expression.else_expression);
            break;
        case JSL_AST_MEMBER_EXPRESSION: jsl_ast_node_free(node->as.member_expression.object); break;
        case JSL_AST_STRUCT_LITERAL_EXPRESSION:
            for (size_t i = 0; i < node->as.struct_literal_expression.field_count; i++) jsl_ast_node_free(node->as.struct_literal_expression.fields[i].value);
            free(node->as.struct_literal_expression.fields);
            break;
        case JSL_AST_CALL_EXPRESSION:
            jsl_ast_node_free(node->as.call_expression.callee);
            free_list(&node->as.call_expression.arguments);
            break;
        case JSL_AST_GROUPING_EXPRESSION: jsl_ast_node_free(node->as.grouping_expression.expression); break;
        case JSL_AST_IDENTIFIER_EXPRESSION:
        case JSL_AST_LITERAL_EXPRESSION: break;
    }
    free(node);
}

void jsl_ast_program_free(JslAstProgram *program) {
    if (program == NULL) return;
    free_list(&program->declarations);
    program->declarations = (JslAstNodeList){0};
}
