cmake_minimum_required(VERSION 3.20)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" JSLANG_VERSION)
string(STRIP "${JSLANG_VERSION}" JSLANG_VERSION)
project(jslang VERSION ${JSLANG_VERSION} LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(jslang_lexer
    compiler/source/source.c
    compiler/token/token.c
    compiler/lexer/lexer.c
)
target_include_directories(jslang_lexer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(jslang_lexer PRIVATE -Wall -Wextra -Wpedantic)

add_executable(jsl cmd/jsl/main.c)
target_link_libraries(jsl PRIVATE jslang_lexer)
target_compile_options(jsl PRIVATE -Wall -Wextra -Wpedantic)
target_compile_definitions(jsl PRIVATE JSLANG_VERSION="${JSLANG_VERSION}")

enable_testing()
add_executable(lexer_tests tests/lexer/lexer_test.c)
target_link_libraries(lexer_tests PRIVATE jslang_lexer)
target_compile_options(lexer_tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME lexer_tests COMMAND lexer_tests)
