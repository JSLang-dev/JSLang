CC ?= cc
CFLAGS := -std=c17 -Wall -Wextra -Wpedantic -I.
BUILD_DIR := build
COMMON := compiler/source/source.c compiler/token/token.c compiler/lexer/lexer.c compiler/ast/ast.c compiler/parser/parser.c
VERSION := $(shell tr -d '\r\n' < VERSION)

.PHONY: all test clean

all: $(BUILD_DIR)/jsl

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/jsl: $(COMMON) cmd/jsl/main.c VERSION | $(BUILD_DIR)
	$(CC) $(CFLAGS) -DJSLANG_VERSION=\"$(VERSION)\" $(COMMON) cmd/jsl/main.c -o $@

$(BUILD_DIR)/lexer_tests: $(COMMON) tests/lexer/lexer_test.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(COMMON) tests/lexer/lexer_test.c -o $@

$(BUILD_DIR)/parser_tests: $(COMMON) tests/parser/parser_test.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(COMMON) tests/parser/parser_test.c -o $@

test: $(BUILD_DIR)/lexer_tests $(BUILD_DIR)/parser_tests $(BUILD_DIR)/jsl
	$(BUILD_DIR)/lexer_tests
	$(BUILD_DIR)/parser_tests
	test "$$($(BUILD_DIR)/jsl version)" = "jsl $(VERSION)"

clean:
	rm -rf $(BUILD_DIR)
