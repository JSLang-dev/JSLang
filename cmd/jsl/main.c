#include "compiler/lexer/lexer.h"
#include "compiler/source/source.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static void print_usage(void) { fprintf(stderr, "usage: jsl <version|lex> [arguments]\n"); }

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

int main(int argc, char **argv) {
    if (argc < 2) { print_usage(); return 2; }
    if (strcmp(argv[1], "version") == 0) { printf("jsl %s\n", JSLANG_VERSION); return 0; }
    if (strcmp(argv[1], "lex") == 0 && argc == 3) return lex_file(argv[2]);
    print_usage();
    return 2;
}
