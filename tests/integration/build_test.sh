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
