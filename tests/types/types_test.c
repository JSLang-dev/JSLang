#include "compiler/checker/checker.h"
#include "compiler/parser/parser.h"
#include "compiler/types/types.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static int check_source(const char *source, JslChecker *checker) {
    JslParser parser;
    JslAstProgram program;
    jsl_parser_init(&parser, "types.jsl", source);
    assert(jsl_parser_parse_program(&parser, &program));
    jsl_checker_init(checker);
    int result = jsl_checker_check_program(checker, &program);
    jsl_ast_program_free(&program);
    return result;
}

static void test_primitive_types(void) {
    assert(jsl_type_from_name((JslAstText){"i32", 3}) == JSL_TYPE_I32);
    assert(jsl_type_from_name((JslAstText){"string", 6}) == JSL_TYPE_STRING);
    assert(jsl_type_from_name((JslAstText){"missing", 7}) == JSL_TYPE_UNKNOWN);
    assert(jsl_type_is_numeric(JSL_TYPE_F64));
    assert(!jsl_type_is_numeric(JSL_TYPE_BOOL));
}

static void test_valid_types(void) {
    JslChecker checker;
    const char *source = "function add(left: i32, right: i32): i32 { return left + right; }\n"
                         "function main(): i32 { const result: i32 = add(20, 22); if (result > 0) { console.log(result); } return result; }";
    assert(check_source(source, &checker));
}

static void test_invalid_types(void) {
    JslChecker checker;
    assert(!check_source("function main(): i32 { const value: i32 = \"bad\"; return value; }", &checker));
    assert(strcmp(jsl_checker_error(&checker, NULL), "variable initializer type does not match declaration type") == 0);
    assert(!check_source("function main(): i32 { return true; }", &checker));
    assert(strcmp(jsl_checker_error(&checker, NULL), "return type does not match function return type") == 0);
    assert(!check_source("function add(value: i32): i32 { return value; } function main(): i32 { return add(true); }", &checker));
    assert(strcmp(jsl_checker_error(&checker, NULL), "function argument type does not match parameter type") == 0);
    assert(!check_source("function main(): i32 { if (1) { return 1; } return 0; }", &checker));
    assert(strcmp(jsl_checker_error(&checker, NULL), "if condition must be bool") == 0);
}

int main(void) {
    test_primitive_types();
    test_valid_types();
    test_invalid_types();
    puts("type tests passed");
    return 0;
}
