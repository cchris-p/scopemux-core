#!/bin/bash
# diff_json_ast.sh: Compare two JSON files (actual vs expected AST) and print a unified diff.
# Usage: ./diff_json_ast.sh actual.json expected.json

set -e

if [ "$#" -ne 2 ]; then
  echo "Usage: $0 <actual.json> <expected.json>"
  exit 1
fi

ACTUAL="$1"
EXPECTED="$2"

if ! [ -f "$ACTUAL" ]; then
  echo "Error: Actual output file '$ACTUAL' does not exist."
  exit 2
fi
if ! [ -f "$EXPECTED" ]; then
  echo "Error: Expected file '$EXPECTED' does not exist."
  exit 2
fi

# Use jq to pretty-print and sort keys for both files
tmp_actual=$(mktemp)
tmp_expected=$(mktemp)
jq --sort-keys . "$ACTUAL" > "$tmp_actual"
jq --sort-keys . "$EXPECTED" > "$tmp_expected"

diff -u "$tmp_expected" "$tmp_actual" || true

rm -f "$tmp_actual" "$tmp_expected"
