#!/bin/sh
set -eu

jsl="$1"
output="$(mktemp /tmp/jslang-build-test-XXXXXX)"
rm -f "$output"
trap 'rm -f "$output"' EXIT

"$jsl" build examples/executable/main.jsl -o "$output"
set +e
"$output"
first_exit_code=$?
set -e

test "$first_exit_code" -eq 42

"$jsl" build examples/functions/main.jsl -o "$output"
set +e
"$output"
second_exit_code=$?
set -e

test "$second_exit_code" -eq 42

"$jsl" build examples/structs/main.jsl -o "$output"
set +e
"$output"
third_exit_code=$?
set -e

test "$third_exit_code" -eq 30

"$jsl" build examples/conditionals/main.jsl -o "$output"
set +e
"$output" 2>/dev/null
fourth_exit_code=$?
set -e

test "$fourth_exit_code" -eq 42

console_error="$(mktemp /tmp/jslang-console-error-XXXXXX)"
trap 'rm -f "$output" "$console_error"' EXIT
console_output="$("$jsl" build examples/console/main.jsl -o "$output" && "$output" 2>"$console_error")"
expected_console_output="$(printf 'Hello, JSLang\n42')"
expected_console_error="$(printf 'warning\nerror')"
test "$console_output" = "$expected_console_output"
test "$(cat "$console_error")" = "$expected_console_error"
