#!/bin/bash
# compare_ast_to_expected.sh: Generate actual AST JSON for a C file, compare with .expected.json, and clean up.
# Usage: ./compare_ast_to_expected.sh <source.c> <source.c.expected.json>

set -e

if [ "$#" -ne 2 ]; then
  echo "Usage: $0 <source.c> <source.c.expected.json>" >&2
  exit 1
fi

SRC="$1"
EXPECTED="$2"

if [ ! -f "$SRC" ]; then
  echo "Error: Source file '$SRC' does not exist." >&2
  exit 2
fi
if [ ! -f "$EXPECTED" ]; then
  echo "Error: Expected JSON file '$EXPECTED' does not exist." >&2
  exit 2
fi

ACTUAL=$(mktemp --suffix=.actual.json)

# Generate actual AST JSON
./generate_ast_json.sh "$SRC" "$ACTUAL"

# Run diff
./diff_json_ast.sh "$ACTUAL" "$EXPECTED"

# Clean up
rm -f "$ACTUAL"
