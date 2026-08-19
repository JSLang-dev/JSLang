#include "compiler/parser/parser.h"

#include <stdio.h>
#include <stdlib.h>

static JslAstText token_text(JslToken token) { return (JslAstText){token.start, token.length}; }

static void set_error(JslParser *parser, JslPosition position, const char *message) {
    if (!parser->had_error) {
        parser->had_error = 1;
        parser->error_position = position;
        parser->error_message = message;
    }
}

static void advance(JslParser *parser) {
    parser->previous = parser->current;
    parser->current = jsl_lexer_next(&parser->lexer);
    if (parser->current.type == JSL_TOKEN_ILLEGAL) {
        JslPosition position;
        const char *message = jsl_lexer_error(&parser->lexer, &position);
        set_error(parser, position, message);
    }
}

static int check(const JslParser *parser, JslTokenType type) { return parser->current.type == type; }
static int match(JslParser *parser, JslTokenType type) {
    if (!check(parser, type)) return 0;
    advance(parser);
    return 1;
}

static int consume(JslParser *parser, JslTokenType type, const char *message) {
    if (match(parser, type)) return 1;
    set_error(parser, parser->current.position, message);
    return 0;
}

static JslAstNode *new_node(JslParser *parser, JslAstKind kind, JslPosition position) {
    JslAstNode *node = jsl_ast_new_node(kind, position);
    if (node == NULL) set_error(parser, position, "out of memory");
    return node;
}

static JslAstNode *parse_expression(JslParser *parser);
static JslAstNode *parse_statement(JslParser *parser);

static JslAstNode *parse_primary(JslParser *parser) {
    JslToken token = parser->current;
    if (match(parser, JSL_TOKEN_IDENTIFIER)) {
        JslAstNode *node = new_node(parser, JSL_AST_IDENTIFIER_EXPRESSION, token.position);
        if (node != NULL) node->as.identifier_expression.name = token_text(token);
        return node;
    }
    JslAstLiteralKind literal_kind;
    if (match(parser, JSL_TOKEN_INTEGER)) literal_kind = JSL_AST_LITERAL_INTEGER;
    else if (match(parser, JSL_TOKEN_FLOAT)) literal_kind = JSL_AST_LITERAL_FLOAT;
    else if (match(parser, JSL_TOKEN_STRING)) literal_kind = JSL_AST_LITERAL_STRING;
    else if (match(parser, JSL_TOKEN_TRUE)) literal_kind = JSL_AST_LITERAL_TRUE;
    else if (match(parser, JSL_TOKEN_FALSE)) literal_kind = JSL_AST_LITERAL_FALSE;
    else if (match(parser, JSL_TOKEN_NULL)) literal_kind = JSL_AST_LITERAL_NULL;
    else if (match(parser, JSL_TOKEN_LEFT_PAREN)) {
        JslAstNode *expression = parse_expression(parser);
        if (expression == NULL || !consume(parser, JSL_TOKEN_RIGHT_PAREN, "expected ')' after expression")) {
            jsl_ast_node_free(expression);
            return NULL;
        }
        JslAstNode *node = new_node(parser, JSL_AST_GROUPING_EXPRESSION, token.position);
        if (node == NULL) { jsl_ast_node_free(expression); return NULL; }
        node->as.grouping_expression.expression = expression;
        return node;
    } else {
        set_error(parser, token.position, "expected expression");
        return NULL;
    }
    JslAstNode *node = new_node(parser, JSL_AST_LITERAL_EXPRESSION, token.position);
    if (node != NULL) {
        node->as.literal_expression.kind = literal_kind;
        node->as.literal_expression.value = token_text(token);
    }
    return node;
}

static JslAstNode *parse_postfix(JslParser *parser) {
    JslAstNode *expression = parse_primary(parser);
    while (expression != NULL) {
        if (match(parser, JSL_TOKEN_DOT)) {
            JslPosition position = parser->previous.position;
            if (!consume(parser, JSL_TOKEN_IDENTIFIER, "expected property name after '.'")) { jsl_ast_node_free(expression); return NULL; }
            JslAstNode *member = new_node(parser, JSL_AST_MEMBER_EXPRESSION, position);
            if (member == NULL) { jsl_ast_node_free(expression); return NULL; }
            member->as.member_expression.object = expression;
            member->as.member_expression.property = token_text(parser->previous);
            expression = member;
        } else if (match(parser, JSL_TOKEN_LEFT_PAREN)) {
            JslAstNode *call = new_node(parser, JSL_AST_CALL_EXPRESSION, parser->previous.position);
            if (call == NULL) { jsl_ast_node_free(expression); return NULL; }
            call->as.call_expression.callee = expression;
            if (!check(parser, JSL_TOKEN_RIGHT_PAREN)) {
                do {
                    JslAstNode *argument = parse_expression(parser);
                    if (argument == NULL || !jsl_ast_node_list_append(&call->as.call_expression.arguments, argument)) {
                        if (argument != NULL) jsl_ast_node_free(argument);
                        set_error(parser, parser->current.position, "out of memory");
                        jsl_ast_node_free(call);
                        return NULL;
                    }
                } while (match(parser, JSL_TOKEN_COMMA));
            }
            if (!consume(parser, JSL_TOKEN_RIGHT_PAREN, "expected ')' after arguments")) { jsl_ast_node_free(call); return NULL; }
            expression = call;
        } else {
            break;
        }
    }
    return expression;
}

static JslAstNode *parse_unary(JslParser *parser) {
    if (check(parser, JSL_TOKEN_BANG) || check(parser, JSL_TOKEN_MINUS) || check(parser, JSL_TOKEN_PLUS)) {
        JslToken operator_token = parser->current;
        advance(parser);
        JslAstNode *operand = parse_unary(parser);
        if (operand == NULL) return NULL;
        JslAstNode *node = new_node(parser, JSL_AST_UNARY_EXPRESSION, operator_token.position);
        if (node == NULL) { jsl_ast_node_free(operand); return NULL; }
        node->as.unary_expression.operator_text = token_text(operator_token);
        node->as.unary_expression.operand = operand;
        return node;
    }
    return parse_postfix(parser);
}

static int precedence(JslTokenType type) {
    switch (type) {
        case JSL_TOKEN_OR: return 1;
        case JSL_TOKEN_AND: return 2;
        case JSL_TOKEN_EQUAL: case JSL_TOKEN_NOT_EQUAL: return 3;
        case JSL_TOKEN_LESS: case JSL_TOKEN_LESS_EQUAL: case JSL_TOKEN_GREATER: case JSL_TOKEN_GREATER_EQUAL: return 4;
        case JSL_TOKEN_PLUS: case JSL_TOKEN_MINUS: return 5;
        case JSL_TOKEN_ASTERISK: case JSL_TOKEN_SLASH: case JSL_TOKEN_PERCENT: return 6;
        default: return 0;
    }
}

static JslAstNode *parse_binary(JslParser *parser, int minimum_precedence) {
    JslAstNode *left = parse_unary(parser);
    while (left != NULL && precedence(parser->current.type) >= minimum_precedence) {
        JslToken operator_token = parser->current;
        int operator_precedence = precedence(operator_token.type);
        advance(parser);
        JslAstNode *right = parse_binary(parser, operator_precedence + 1);
        if (right == NULL) { jsl_ast_node_free(left); return NULL; }
        JslAstNode *node = new_node(parser, JSL_AST_BINARY_EXPRESSION, operator_token.position);
        if (node == NULL) { jsl_ast_node_free(left); jsl_ast_node_free(right); return NULL; }
        node->as.binary_expression.left = left;
        node->as.binary_expression.operator_text = token_text(operator_token);
        node->as.binary_expression.right = right;
        left = node;
    }
    return left;
}

static JslAstNode *parse_expression(JslParser *parser) {
    JslAstNode *condition = parse_binary(parser, 1);
    if (condition == NULL || !match(parser, JSL_TOKEN_QUESTION)) return condition;
    JslPosition position = parser->previous.position;
    JslAstNode *then_expression = parse_expression(parser);
    if (then_expression == NULL || !consume(parser, JSL_TOKEN_COLON, "expected ':' in conditional expression")) {
        jsl_ast_node_free(condition);
        jsl_ast_node_free(then_expression);
        return NULL;
    }
    JslAstNode *else_expression = parse_expression(parser);
    if (else_expression == NULL) {
        jsl_ast_node_free(condition);
        jsl_ast_node_free(then_expression);
        return NULL;
    }
    JslAstNode *node = new_node(parser, JSL_AST_CONDITIONAL_EXPRESSION, position);
    if (node == NULL) {
        jsl_ast_node_free(condition);
        jsl_ast_node_free(then_expression);
        jsl_ast_node_free(else_expression);
        return NULL;
    }
    node->as.conditional_expression.condition = condition;
    node->as.conditional_expression.then_expression = then_expression;
    node->as.conditional_expression.else_expression = else_expression;
    return node;
}

static JslAstNode *parse_block(JslParser *parser, JslPosition position) {
    JslAstNode *block = new_node(parser, JSL_AST_BLOCK_STATEMENT, position);
    if (block == NULL) return NULL;
    while (!check(parser, JSL_TOKEN_RIGHT_BRACE) && !check(parser, JSL_TOKEN_EOF) && !parser->had_error) {
        JslAstNode *statement = parse_statement(parser);
        if (statement == NULL || !jsl_ast_node_list_append(&block->as.block_statement.statements, statement)) {
            if (statement != NULL) jsl_ast_node_free(statement);
            set_error(parser, parser->current.position, "out of memory");
            jsl_ast_node_free(block);
            return NULL;
        }
    }
    if (!consume(parser, JSL_TOKEN_RIGHT_BRACE, "expected '}' after block")) { jsl_ast_node_free(block); return NULL; }
    return block;
}

static JslAstNode *parse_variable_statement(JslParser *parser, int is_mutable, JslPosition position) {
    if (!consume(parser, JSL_TOKEN_IDENTIFIER, "expected variable name")) return NULL;
    JslToken name = parser->previous;
    JslAstText type_name = {0};
    if (match(parser, JSL_TOKEN_COLON)) {
        if (!consume(parser, JSL_TOKEN_IDENTIFIER, "expected type name after ':'")) return NULL;
        type_name = token_text(parser->previous);
    }
    if (!consume(parser, JSL_TOKEN_ASSIGN, "expected '=' after variable declaration")) return NULL;
    JslAstNode *initializer = parse_expression(parser);
    if (initializer == NULL || !consume(parser, JSL_TOKEN_SEMICOLON, "expected ';' after variable declaration")) {
        jsl_ast_node_free(initializer);
        return NULL;
    }
    JslAstNode *node = new_node(parser, JSL_AST_VARIABLE_STATEMENT, position);
    if (node == NULL) { jsl_ast_node_free(initializer); return NULL; }
    node->as.variable_statement.is_mutable = is_mutable;
    node->as.variable_statement.name = token_text(name);
    node->as.variable_statement.type_name = type_name;
    node->as.variable_statement.initializer = initializer;
    return node;
}

static JslAstNode *parse_return_statement(JslParser *parser, JslPosition position) {
    JslAstNode *value = NULL;
    if (!check(parser, JSL_TOKEN_SEMICOLON)) value = parse_expression(parser);
    if ((value == NULL && !check(parser, JSL_TOKEN_SEMICOLON)) || !consume(parser, JSL_TOKEN_SEMICOLON, "expected ';' after return value")) {
        jsl_ast_node_free(value);
        return NULL;
    }
    JslAstNode *node = new_node(parser, JSL_AST_RETURN_STATEMENT, position);
    if (node == NULL) { jsl_ast_node_free(value); return NULL; }
    node->as.return_statement.value = value;
    return node;
}

static JslAstNode *parse_if_statement(JslParser *parser, JslPosition position) {
    if (!consume(parser, JSL_TOKEN_LEFT_PAREN, "expected '(' after 'if'")) return NULL;
    JslAstNode *condition = parse_expression(parser);
    if (condition == NULL || !consume(parser, JSL_TOKEN_RIGHT_PAREN, "expected ')' after if condition") || !consume(parser, JSL_TOKEN_LEFT_BRACE, "expected '{' after if condition")) {
        jsl_ast_node_free(condition);
        return NULL;
    }
    JslAstNode *then_branch = parse_block(parser, parser->previous.position);
    JslAstNode *else_branch = NULL;
    if (then_branch != NULL && match(parser, JSL_TOKEN_ELSE)) {
        if (!consume(parser, JSL_TOKEN_LEFT_BRACE, "expected '{' after 'else'")) { jsl_ast_node_free(condition); jsl_ast_node_free(then_branch); return NULL; }
        else_branch = parse_block(parser, parser->previous.position);
    }
    if (then_branch == NULL || (parser->had_error && else_branch == NULL)) {
        jsl_ast_node_free(condition); jsl_ast_node_free(then_branch); jsl_ast_node_free(else_branch);
        return NULL;
    }
    JslAstNode *node = new_node(parser, JSL_AST_IF_STATEMENT, position);
    if (node == NULL) { jsl_ast_node_free(condition); jsl_ast_node_free(then_branch); jsl_ast_node_free(else_branch); return NULL; }
    node->as.if_statement.condition = condition;
    node->as.if_statement.then_branch = then_branch;
    node->as.if_statement.else_branch = else_branch;
    return node;
}

static JslAstNode *parse_statement(JslParser *parser) {
    JslToken token = parser->current;
    if (match(parser, JSL_TOKEN_CONST)) return parse_variable_statement(parser, 0, token.position);
    if (match(parser, JSL_TOKEN_LET)) return parse_variable_statement(parser, 1, token.position);
    if (match(parser, JSL_TOKEN_RETURN)) return parse_return_statement(parser, token.position);
    if (match(parser, JSL_TOKEN_IF)) return parse_if_statement(parser, token.position);
    JslAstNode *expression = parse_expression(parser);
    if (expression == NULL || !consume(parser, JSL_TOKEN_SEMICOLON, "expected ';' after expression")) { jsl_ast_node_free(expression); return NULL; }
    JslAstNode *node = new_node(parser, JSL_AST_EXPRESSION_STATEMENT, token.position);
    if (node == NULL) { jsl_ast_node_free(expression); return NULL; }
    node->as.expression_statement.expression = expression;
    return node;
}

static JslAstNode *parse_function(JslParser *parser, JslPosition position, int is_exported) {
    if (!consume(parser, JSL_TOKEN_IDENTIFIER, "expected function name")) return NULL;
    JslToken name = parser->previous;
    if (!consume(parser, JSL_TOKEN_LEFT_PAREN, "expected '(' after function name")) return NULL;
    JslAstParameter *parameters = NULL;
    size_t parameter_count = 0;
    if (!check(parser, JSL_TOKEN_RIGHT_PAREN)) {
        do {
            if (!consume(parser, JSL_TOKEN_IDENTIFIER, "expected parameter name")) { free(parameters); return NULL; }
            JslToken parameter_name = parser->previous;
            if (!consume(parser, JSL_TOKEN_COLON, "expected ':' after parameter name") || !consume(parser, JSL_TOKEN_IDENTIFIER, "expected parameter type")) { free(parameters); return NULL; }
            JslAstParameter *grown = realloc(parameters, (parameter_count + 1) * sizeof(*parameters));
            if (grown == NULL) { free(parameters); set_error(parser, parameter_name.position, "out of memory"); return NULL; }
            parameters = grown;
            parameters[parameter_count++] = (JslAstParameter){parameter_name.position, token_text(parameter_name), token_text(parser->previous)};
        } while (match(parser, JSL_TOKEN_COMMA));
    }
    if (!consume(parser, JSL_TOKEN_RIGHT_PAREN, "expected ')' after parameters") || !consume(parser, JSL_TOKEN_COLON, "expected ':' before return type") || !consume(parser, JSL_TOKEN_IDENTIFIER, "expected return type")) { free(parameters); return NULL; }
    JslAstText return_type = token_text(parser->previous);
    if (!consume(parser, JSL_TOKEN_LEFT_BRACE, "expected '{' before function body")) { free(parameters); return NULL; }
    JslAstNode *body = parse_block(parser, parser->previous.position);
    if (body == NULL) { free(parameters); return NULL; }
    JslAstNode *node = new_node(parser, JSL_AST_FUNCTION_DECLARATION, position);
    if (node == NULL) { free(parameters); jsl_ast_node_free(body); return NULL; }
    node->as.function_declaration.is_exported = is_exported;
    node->as.function_declaration.name = token_text(name);
    node->as.function_declaration.parameters = parameters;
    node->as.function_declaration.parameter_count = parameter_count;
    node->as.function_declaration.return_type = return_type;
    node->as.function_declaration.body = body;
    return node;
}

static JslAstNode *parse_import(JslParser *parser, JslPosition position) {
    if (!consume(parser, JSL_TOKEN_LEFT_BRACE, "expected '{' after 'import'")) return NULL;
    JslAstText *names = NULL;
    size_t name_count = 0;
    if (!check(parser, JSL_TOKEN_RIGHT_BRACE)) {
        do {
            if (!consume(parser, JSL_TOKEN_IDENTIFIER, "expected imported name")) { free(names); return NULL; }
            JslAstText *grown = realloc(names, (name_count + 1) * sizeof(*names));
            if (grown == NULL) { free(names); set_error(parser, parser->previous.position, "out of memory"); return NULL; }
            names = grown;
            names[name_count++] = token_text(parser->previous);
        } while (match(parser, JSL_TOKEN_COMMA));
    }
    if (!consume(parser, JSL_TOKEN_RIGHT_BRACE, "expected '}' after imported names") || !consume(parser, JSL_TOKEN_FROM, "expected 'from' after imported names") || !consume(parser, JSL_TOKEN_STRING, "expected module path string after 'from'")) { free(names); return NULL; }
    JslAstText module_path = token_text(parser->previous);
    if (!consume(parser, JSL_TOKEN_SEMICOLON, "expected ';' after import declaration")) { free(names); return NULL; }
    JslAstNode *node = new_node(parser, JSL_AST_IMPORT_DECLARATION, position);
    if (node == NULL) { free(names); return NULL; }
    node->as.import_declaration.names = names;
    node->as.import_declaration.name_count = name_count;
    node->as.import_declaration.module_path = module_path;
    return node;
}

void jsl_parser_init(JslParser *parser, const char *filename, const char *source) {
    jsl_lexer_init(&parser->lexer, filename, source);
    parser->current = (JslToken){0};
    parser->previous = (JslToken){0};
    parser->error_message = NULL;
    parser->error_position = (JslPosition){filename, 1, 1};
    parser->had_error = 0;
    advance(parser);
}

int jsl_parser_parse_program(JslParser *parser, JslAstProgram *program) {
    *program = (JslAstProgram){0};
    while (!check(parser, JSL_TOKEN_EOF) && !parser->had_error) {
        JslToken token = parser->current;
        JslAstNode *declaration;
        if (match(parser, JSL_TOKEN_IMPORT)) declaration = parse_import(parser, token.position);
        else {
            int is_exported = match(parser, JSL_TOKEN_EXPORT);
            if (!match(parser, JSL_TOKEN_FUNCTION)) { set_error(parser, token.position, "expected import or function declaration"); break; }
            declaration = parse_function(parser, token.position, is_exported);
        }
        if (declaration == NULL || !jsl_ast_node_list_append(&program->declarations, declaration)) {
            if (declaration != NULL) jsl_ast_node_free(declaration);
            set_error(parser, parser->current.position, "out of memory");
            break;
        }
    }
    if (parser->had_error) { jsl_ast_program_free(program); return 0; }
    return 1;
}

const char *jsl_parser_error(const JslParser *parser, JslPosition *position) {
    if (position != NULL) *position = parser->error_position;
    return parser->error_message;
}

static void print_text(JslAstText text) { printf("%.*s", (int)text.length, text.start); }
static void print_node(const JslAstNode *node);
static void print_list(const JslAstNodeList *list) {
    for (size_t i = 0; i < list->count; i++) { putchar(' '); print_node(list->items[i]); }
}
static void print_node(const JslAstNode *node) {
    switch (node->kind) {
        case JSL_AST_IMPORT_DECLARATION:
            printf("(import (");
            for (size_t i = 0; i < node->as.import_declaration.name_count; i++) {
                if (i != 0) putchar(' ');
                print_text(node->as.import_declaration.names[i]);
            }
            printf(") "); print_text(node->as.import_declaration.module_path); putchar(')'); break;
        case JSL_AST_FUNCTION_DECLARATION:
            printf("(%sfunction ", node->as.function_declaration.is_exported ? "export " : ""); print_text(node->as.function_declaration.name); printf(" (");
            for (size_t i = 0; i < node->as.function_declaration.parameter_count; i++) {
                if (i != 0) putchar(' ');
                print_text(node->as.function_declaration.parameters[i].name); putchar(':'); print_text(node->as.function_declaration.parameters[i].type_name);
            }
            printf(") :"); print_text(node->as.function_declaration.return_type); putchar(' '); print_node(node->as.function_declaration.body); putchar(')'); break;
        case JSL_AST_BLOCK_STATEMENT: printf("(block"); print_list(&node->as.block_statement.statements); putchar(')'); break;
        case JSL_AST_VARIABLE_STATEMENT:
            printf("(%s ", node->as.variable_statement.is_mutable ? "let" : "const"); print_text(node->as.variable_statement.name);
            if (node->as.variable_statement.type_name.start != NULL) { putchar(':'); print_text(node->as.variable_statement.type_name); }
            putchar(' '); print_node(node->as.variable_statement.initializer); putchar(')'); break;
        case JSL_AST_RETURN_STATEMENT: printf("(return"); if (node->as.return_statement.value != NULL) { putchar(' '); print_node(node->as.return_statement.value); } putchar(')'); break;
        case JSL_AST_EXPRESSION_STATEMENT: printf("(expression "); print_node(node->as.expression_statement.expression); putchar(')'); break;
        case JSL_AST_IF_STATEMENT:
            printf("(if "); print_node(node->as.if_statement.condition); putchar(' '); print_node(node->as.if_statement.then_branch);
            if (node->as.if_statement.else_branch != NULL) { putchar(' '); print_node(node->as.if_statement.else_branch); }
            putchar(')'); break;
        case JSL_AST_IDENTIFIER_EXPRESSION: print_text(node->as.identifier_expression.name); break;
        case JSL_AST_LITERAL_EXPRESSION: print_text(node->as.literal_expression.value); break;
        case JSL_AST_UNARY_EXPRESSION: putchar('('); print_text(node->as.unary_expression.operator_text); putchar(' '); print_node(node->as.unary_expression.operand); putchar(')'); break;
        case JSL_AST_BINARY_EXPRESSION: putchar('('); print_text(node->as.binary_expression.operator_text); putchar(' '); print_node(node->as.binary_expression.left); putchar(' '); print_node(node->as.binary_expression.right); putchar(')'); break;
        case JSL_AST_CONDITIONAL_EXPRESSION: printf("(?: "); print_node(node->as.conditional_expression.condition); putchar(' '); print_node(node->as.conditional_expression.then_expression); putchar(' '); print_node(node->as.conditional_expression.else_expression); putchar(')'); break;
        case JSL_AST_MEMBER_EXPRESSION: printf("(member "); print_node(node->as.member_expression.object); putchar(' '); print_text(node->as.member_expression.property); putchar(')'); break;
        case JSL_AST_CALL_EXPRESSION: printf("(call "); print_node(node->as.call_expression.callee); print_list(&node->as.call_expression.arguments); putchar(')'); break;
        case JSL_AST_GROUPING_EXPRESSION: printf("(group "); print_node(node->as.grouping_expression.expression); putchar(')'); break;
    }
}

void jsl_ast_print_program(const JslAstProgram *program) {
    for (size_t i = 0; i < program->declarations.count; i++) { print_node(program->declarations.items[i]); putchar('\n'); }
}
