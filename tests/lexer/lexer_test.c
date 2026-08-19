#include "compiler/lexer/lexer.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct { JslTokenType type; const char *literal; size_t line; size_t column; } Expected;

static void test_program(void) {
    const char *source = "function main(): i32 {\n  const value: i32 = 42;\n  return value;\n}\n";
    const Expected expected[] = {
        {JSL_TOKEN_FUNCTION, "function", 1, 1}, {JSL_TOKEN_IDENTIFIER, "main", 1, 10}, {JSL_TOKEN_LEFT_PAREN, "(", 1, 14},
        {JSL_TOKEN_RIGHT_PAREN, ")", 1, 15}, {JSL_TOKEN_COLON, ":", 1, 16}, {JSL_TOKEN_IDENTIFIER, "i32", 1, 18}, {JSL_TOKEN_LEFT_BRACE, "{", 1, 22},
        {JSL_TOKEN_CONST, "const", 2, 3}, {JSL_TOKEN_IDENTIFIER, "value", 2, 9}, {JSL_TOKEN_COLON, ":", 2, 14}, {JSL_TOKEN_IDENTIFIER, "i32", 2, 16}, {JSL_TOKEN_ASSIGN, "=", 2, 20}, {JSL_TOKEN_INTEGER, "42", 2, 22}, {JSL_TOKEN_SEMICOLON, ";", 2, 24},
        {JSL_TOKEN_RETURN, "return", 3, 3}, {JSL_TOKEN_IDENTIFIER, "value", 3, 10}, {JSL_TOKEN_SEMICOLON, ";", 3, 15}, {JSL_TOKEN_RIGHT_BRACE, "}", 4, 1}, {JSL_TOKEN_EOF, "", 5, 1},
    };
    JslLexer lexer; jsl_lexer_init(&lexer, "main.jsl", source);
    for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++) {
        JslToken token = jsl_lexer_next(&lexer);
        assert(token.type == expected[i].type);
        assert(token.length == strlen(expected[i].literal));
        assert(strncmp(token.start, expected[i].literal, token.length) == 0);
        assert(token.position.line == expected[i].line && token.position.column == expected[i].column);
        assert(strcmp(token.position.filename, "main.jsl") == 0);
    }
}

static void test_literals_operators_and_comments(void) {
    JslLexer lexer; jsl_lexer_init(&lexer, "tokens.jsl", "// skip\nlet x = 3.14 >= 2 && false; /* skip */ \"hello\"");
    const JslTokenType types[] = {JSL_TOKEN_LET, JSL_TOKEN_IDENTIFIER, JSL_TOKEN_ASSIGN, JSL_TOKEN_FLOAT, JSL_TOKEN_GREATER_EQUAL, JSL_TOKEN_INTEGER, JSL_TOKEN_AND, JSL_TOKEN_FALSE, JSL_TOKEN_SEMICOLON, JSL_TOKEN_STRING, JSL_TOKEN_EOF};
    for (size_t i = 0; i < sizeof(types) / sizeof(types[0]); i++) assert(jsl_lexer_next(&lexer).type == types[i]);
}

static void test_invalid_character(void) {
    JslLexer lexer; jsl_lexer_init(&lexer, "bad.jsl", "const value = @;");
    while (jsl_lexer_next(&lexer).type != JSL_TOKEN_ILLEGAL) {}
    JslPosition position;
    assert(strcmp(jsl_lexer_error(&lexer, &position), "unexpected character") == 0);
    assert(jsl_lexer_error_character(&lexer) == '@');
    assert(position.line == 1 && position.column == 15);
}

int main(void) {
    test_program(); test_literals_operators_and_comments(); test_invalid_character();
    puts("lexer tests passed");
    return 0;
}
