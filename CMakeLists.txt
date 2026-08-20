cmake_minimum_required(VERSION 3.20)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" JSLANG_VERSION)
string(STRIP "${JSLANG_VERSION}" JSLANG_VERSION)
project(jslang VERSION ${JSLANG_VERSION} LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(jslang_frontend
    compiler/source/source.c
    compiler/token/token.c
    compiler/lexer/lexer.c
    compiler/ast/ast.c
    compiler/parser/parser.c
    compiler/checker/checker.c
)
target_include_directories(jslang_frontend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(jslang_frontend PRIVATE -Wall -Wextra -Wpedantic)

add_executable(jsl cmd/jsl/main.c)
target_link_libraries(jsl PRIVATE jslang_frontend)
target_compile_options(jsl PRIVATE -Wall -Wextra -Wpedantic)
target_compile_definitions(jsl PRIVATE JSLANG_VERSION="${JSLANG_VERSION}")

enable_testing()
add_executable(lexer_tests tests/lexer/lexer_test.c)
target_link_libraries(lexer_tests PRIVATE jslang_frontend)
target_compile_options(lexer_tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME lexer_tests COMMAND lexer_tests)

add_executable(parser_tests tests/parser/parser_test.c)
target_link_libraries(parser_tests PRIVATE jslang_frontend)
target_compile_options(parser_tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME parser_tests COMMAND parser_tests)

add_executable(checker_tests tests/checker/checker_test.c)
target_link_libraries(checker_tests PRIVATE jslang_frontend)
target_compile_options(checker_tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME checker_tests COMMAND checker_tests)
