#ifndef JSLANG_COMPILER_AST_AST_H
#define JSLANG_COMPILER_AST_AST_H

#include "compiler/source/source.h"

#include <stddef.h>

typedef struct {
    const char *start;
    size_t length;
} JslAstText;

typedef enum {
    JSL_AST_IMPORT_DECLARATION,
    JSL_AST_FUNCTION_DECLARATION,
    JSL_AST_BLOCK_STATEMENT,
    JSL_AST_VARIABLE_STATEMENT,
    JSL_AST_RETURN_STATEMENT,
    JSL_AST_EXPRESSION_STATEMENT,
    JSL_AST_IF_STATEMENT,
    JSL_AST_IDENTIFIER_EXPRESSION,
    JSL_AST_LITERAL_EXPRESSION,
    JSL_AST_UNARY_EXPRESSION,
    JSL_AST_BINARY_EXPRESSION,
    JSL_AST_CONDITIONAL_EXPRESSION,
    JSL_AST_MEMBER_EXPRESSION,
    JSL_AST_CALL_EXPRESSION,
    JSL_AST_GROUPING_EXPRESSION
} JslAstKind;

typedef enum {
    JSL_AST_LITERAL_INTEGER,
    JSL_AST_LITERAL_FLOAT,
    JSL_AST_LITERAL_STRING,
    JSL_AST_LITERAL_TRUE,
    JSL_AST_LITERAL_FALSE,
    JSL_AST_LITERAL_NULL
} JslAstLiteralKind;

typedef struct JslAstNode JslAstNode;

typedef struct {
    JslPosition position;
    JslAstText name;
    JslAstText type_name;
} JslAstParameter;

typedef struct {
    JslAstNode **items;
    size_t count;
    size_t capacity;
} JslAstNodeList;

struct JslAstNode {
    JslAstKind kind;
    JslPosition position;
    union {
        struct {
            JslAstText *names;
            size_t name_count;
            JslAstText module_path;
        } import_declaration;
        struct {
            int is_exported;
            JslAstText name;
            JslAstParameter *parameters;
            size_t parameter_count;
            JslAstText return_type;
            JslAstNode *body;
        } function_declaration;
        struct { JslAstNodeList statements; } block_statement;
        struct {
            int is_mutable;
            JslAstText name;
            JslAstText type_name;
            JslAstNode *initializer;
        } variable_statement;
        struct { JslAstNode *value; } return_statement;
        struct { JslAstNode *expression; } expression_statement;
        struct {
            JslAstNode *condition;
            JslAstNode *then_branch;
            JslAstNode *else_branch;
        } if_statement;
        struct { JslAstText name; } identifier_expression;
        struct { JslAstLiteralKind kind; JslAstText value; } literal_expression;
        struct { JslAstText operator_text; JslAstNode *operand; } unary_expression;
        struct {
            JslAstNode *left;
            JslAstText operator_text;
            JslAstNode *right;
        } binary_expression;
        struct {
            JslAstNode *condition;
            JslAstNode *then_expression;
            JslAstNode *else_expression;
        } conditional_expression;
        struct { JslAstNode *object; JslAstText property; } member_expression;
        struct { JslAstNode *callee; JslAstNodeList arguments; } call_expression;
        struct { JslAstNode *expression; } grouping_expression;
    } as;
};

typedef struct {
    JslAstNodeList declarations;
} JslAstProgram;

JslAstNode *jsl_ast_new_node(JslAstKind kind, JslPosition position);
int jsl_ast_node_list_append(JslAstNodeList *list, JslAstNode *node);
void jsl_ast_node_free(JslAstNode *node);
void jsl_ast_program_free(JslAstProgram *program);

#endif
