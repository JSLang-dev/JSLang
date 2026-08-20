#define _POSIX_C_SOURCE 200809L

#include "compiler/backend/c_backend.h"
#include "compiler/checker/checker.h"
#include "compiler/lexer/lexer.h"
#include "compiler/parser/parser.h"
#include "compiler/source/source.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef JSLANG_VERSION
#error "JSLANG_VERSION must be supplied by the build system"
#endif

static char *read_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) return NULL;
    if (fseek(file, 0, SEEK_END) != 0) { fclose(file); return NULL; }
    long length = ftell(file);
    if (length < 0) { fclose(file); return NULL; }
    rewind(file);
    char *source = malloc((size_t)length + 1);
    if (source == NULL || fread(source, 1, (size_t)length, file) != (size_t)length) { free(source); fclose(file); return NULL; }
    source[length] = '\0';
    fclose(file);
    return source;
}

static void print_usage(void) { fprintf(stderr, "usage: jsl <version|lex|parse|check|build> [arguments]\n"); }

static int lex_file(const char *path) {
    char *source = read_file(path);
    if (source == NULL) { fprintf(stderr, "error: unable to read '%s'\n", path); return 1; }
    JslLexer lexer;
    jsl_lexer_init(&lexer, path, source);
    for (;;) {
        JslToken token = jsl_lexer_next(&lexer);
        if (token.type == JSL_TOKEN_ILLEGAL) {
            JslPosition error_position;
            const char *message = jsl_lexer_error(&lexer, &error_position);
            if (jsl_lexer_error_character(&lexer) != '\0') fprintf(stderr, "error: %s '%c'\n", message, jsl_lexer_error_character(&lexer));
            else fprintf(stderr, "error: %s\n", message);
            fprintf(stderr, "  %s:%zu:%zu\n", error_position.filename, error_position.line, error_position.column);
            free(source);
            return 1;
        }
        printf("%s:%zu:%zu %-12s ", token.position.filename, token.position.line, token.position.column, jsl_token_type_name(token.type));
        if (token.type != JSL_TOKEN_EOF) printf("%.*s", (int)token.length, token.start);
        putchar('\n');
        if (token.type == JSL_TOKEN_EOF) break;
    }
    free(source);
    return 0;
}

static int parse_file(const char *path) {
    char *source = read_file(path);
    if (source == NULL) { fprintf(stderr, "error: unable to read '%s'\n", path); return 1; }
    JslParser parser;
    JslAstProgram program;
    jsl_parser_init(&parser, path, source);
    if (!jsl_parser_parse_program(&parser, &program)) {
        JslPosition position;
        fprintf(stderr, "error: %s\n", jsl_parser_error(&parser, &position));
        fprintf(stderr, "  %s:%zu:%zu\n", position.filename, position.line, position.column);
        free(source);
        return 1;
    }
    jsl_ast_print_program(&program);
    jsl_ast_program_free(&program);
    free(source);
    return 0;
}

static int check_file(const char *path) {
    char *source = read_file(path);
    if (source == NULL) { fprintf(stderr, "error: unable to read '%s'\n", path); return 1; }
    JslParser parser;
    JslAstProgram program;
    jsl_parser_init(&parser, path, source);
    if (!jsl_parser_parse_program(&parser, &program)) {
        JslPosition position;
        fprintf(stderr, "error: %s\n", jsl_parser_error(&parser, &position));
        fprintf(stderr, "  %s:%zu:%zu\n", position.filename, position.line, position.column);
        free(source);
        return 1;
    }
    JslChecker checker;
    jsl_checker_init(&checker);
    if (!jsl_checker_check_program(&checker, &program)) {
        JslPosition position;
        fprintf(stderr, "error: %s\n", jsl_checker_error(&checker, &position));
        fprintf(stderr, "  %s:%zu:%zu\n", position.filename, position.line, position.column);
        jsl_ast_program_free(&program);
        free(source);
        return 1;
    }
    jsl_ast_program_free(&program);
    free(source);
    return 0;
}

static char *default_output_path(const char *path) {
    const char *name = strrchr(path, '/');
    name = name == NULL ? path : name + 1;
    size_t length = strlen(name);
    if (length > 4 && strcmp(name + length - 4, ".jsl") == 0) length -= 4;
    char *output = malloc(length + 1);
    if (output == NULL) return NULL;
    memcpy(output, name, length);
    output[length] = '\0';
    return output;
}

static int build_file(const char *path, const char *output_path) {
    char *source = read_file(path);
    if (source == NULL) { fprintf(stderr, "error: unable to read '%s'\n", path); return 1; }
    JslParser parser;
    JslAstProgram program;
    jsl_parser_init(&parser, path, source);
    if (!jsl_parser_parse_program(&parser, &program)) {
        JslPosition position;
        fprintf(stderr, "error: %s\n", jsl_parser_error(&parser, &position));
        fprintf(stderr, "  %s:%zu:%zu\n", position.filename, position.line, position.column);
        free(source);
        return 1;
    }
    JslChecker checker;
    jsl_checker_init(&checker);
    if (!jsl_checker_check_program(&checker, &program)) {
        JslPosition position;
        fprintf(stderr, "error: %s\n", jsl_checker_error(&checker, &position));
        fprintf(stderr, "  %s:%zu:%zu\n", position.filename, position.line, position.column);
        jsl_ast_program_free(&program);
        free(source);
        return 1;
    }
    char temporary_path[] = "/tmp/jsl-c-backend-XXXXXX";
    int descriptor = mkstemp(temporary_path);
    if (descriptor < 0) {
        fprintf(stderr, "error: unable to create temporary C source\n");
        jsl_ast_program_free(&program);
        free(source);
        return 1;
    }
    FILE *generated = fdopen(descriptor, "w");
    JslCBackend backend;
    jsl_c_backend_init(&backend);
    int emitted = 0;
    if (generated != NULL) {
        emitted = jsl_c_backend_emit(&program, generated, &backend);
        if (fclose(generated) != 0) emitted = 0;
    } else {
        close(descriptor);
    }
    if (!emitted) {
        if (backend.error_message == NULL) {
            backend.error_message = "unable to write generated C source";
            backend.error_position = (JslPosition){path, 1, 1};
        }
        fprintf(stderr, "error: %s\n", backend.error_message == NULL ? "unable to write generated C source" : backend.error_message);
        fprintf(stderr, "  %s:%zu:%zu\n", backend.error_position.filename, backend.error_position.line, backend.error_position.column);
        unlink(temporary_path);
        jsl_ast_program_free(&program);
        free(source);
        return 1;
    }
    pid_t child = fork();
    if (child == 0) {
        execlp("cc", "cc", "-std=c17", "-x", "c", temporary_path, "-o", output_path, (char *)NULL);
        _exit(127);
    }
    int status = 0;
    int compiled = child > 0 && waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0;
    unlink(temporary_path);
    jsl_ast_program_free(&program);
    free(source);
    if (!compiled) { fprintf(stderr, "error: native C compilation failed\n"); return 1; }
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) { print_usage(); return 2; }
    if (strcmp(argv[1], "version") == 0) { printf("jsl %s\n", JSLANG_VERSION); return 0; }
    if (strcmp(argv[1], "lex") == 0 && argc == 3) return lex_file(argv[2]);
    if (strcmp(argv[1], "parse") == 0 && argc == 3) return parse_file(argv[2]);
    if (strcmp(argv[1], "check") == 0 && argc == 3) return check_file(argv[2]);
    if (strcmp(argv[1], "build") == 0 && argc == 3) {
        char *output_path = default_output_path(argv[2]);
        if (output_path == NULL) { fprintf(stderr, "error: out of memory\n"); return 1; }
        int result = build_file(argv[2], output_path);
        free(output_path);
        return result;
    }
    if (strcmp(argv[1], "build") == 0 && argc == 5 && strcmp(argv[3], "-o") == 0) return build_file(argv[2], argv[4]);
    print_usage();
    return 2;
}
