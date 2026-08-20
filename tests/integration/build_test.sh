#!/bin/sh
set -eu

jsl="$1"
output="$(mktemp /tmp/jslang-build-test-XXXXXX)"
rm -f "$output"
trap 'rm -f "$output"' EXIT

"$jsl" build examples/executable/main.jsl -o "$output"
set +e
"$output"
status=$?
set -e

test "$status" -eq 42
