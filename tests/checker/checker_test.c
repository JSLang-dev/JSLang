#include "compiler/checker/checker.h"
#include "compiler/parser/parser.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static int check_source(const char *filename, const char *source, JslChecker *checker) {
    JslParser parser;
    JslAstProgram program;
    jsl_parser_init(&parser, filename, source);
    assert(jsl_parser_parse_program(&parser, &program));
    jsl_checker_init(checker);
    int result = jsl_checker_check_program(checker, &program);
    jsl_ast_program_free(&program);
    return result;
}

static void test_valid_scopes_and_symbols(void) {
    JslChecker checker;
    const char *source = "import { add } from \"./math.jsl\";\n"
                         "function addLocal(value: i32): i32 { return value; }\n"
                         "function main(): i32 {\n"
                         "  const value: i32 = add(20, 22);\n"
                         "  if (value > 0) { const label: string = \"ok\"; console.info(label); }\n"
                         "  return addLocal(value);\n"
                         "}";
    assert(check_source("valid.jsl", source, &checker));
}

static void test_duplicate_variable(void) {
    JslChecker checker;
    assert(!check_source("duplicate.jsl", "function main(): i32 { const value: i32 = 1; let value: i32 = 2; return value; }", &checker));
    JslPosition position;
    assert(strcmp(jsl_checker_error(&checker, &position), "duplicate declaration in this scope") == 0);
    assert(strcmp(position.filename, "duplicate.jsl") == 0);
    assert(position.line == 1 && position.column == 46);
}

static void test_duplicate_function_and_parameter(void) {
    JslChecker checker;
    assert(!check_source("functions.jsl", "function same(): i32 { return 1; } function same(): i32 { return 2; }", &checker));
    assert(strcmp(jsl_checker_error(&checker, NULL), "duplicate declaration in this scope") == 0);
    assert(!check_source("parameters.jsl", "function add(value: i32, value: i32): i32 { return value; }", &checker));
    assert(strcmp(jsl_checker_error(&checker, NULL), "duplicate declaration in this scope") == 0);
}

static void test_undefined_variable(void) {
    JslChecker checker;
    assert(!check_source("undefined.jsl", "function main(): i32 { return missing; }", &checker));
    JslPosition position;
    assert(strcmp(jsl_checker_error(&checker, &position), "undefined variable") == 0);
    assert(strcmp(position.filename, "undefined.jsl") == 0);
    assert(position.line == 1 && position.column == 31);
}

static void test_assignments(void) {
    JslChecker checker;
    assert(check_source("assignment.jsl", "function main(): i32 { let value: i32 = 1; value = value + 41; return value; }", &checker));
    assert(!check_source("constant.jsl", "function main(): i32 { const value: i32 = 1; value = 2; return value; }", &checker));
    assert(strcmp(jsl_checker_error(&checker, NULL), "cannot assign to immutable binding") == 0);
    assert(!check_source("mismatch.jsl", "function main(): i32 { let value: i32 = 1; value = true; return value; }", &checker));
    assert(strcmp(jsl_checker_error(&checker, NULL), "assignment value does not match variable type") == 0);
}

int main(void) {
    test_valid_scopes_and_symbols();
    test_duplicate_variable();
    test_duplicate_function_and_parameter();
    test_undefined_variable();
    test_assignments();
    puts("checker tests passed");
    return 0;
}
