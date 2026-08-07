#!/bin/bash
# generate_ast_json.sh: Generate AST JSON from a C source file using ScopeMux Python bindings.
# Usage: ./generate_ast_json.sh <source.c> <output.json>

set -e

if [ "$#" -ne 2 ]; then
  echo "Usage: $0 <source.c> <output.json>"
  exit 1
fi

SRC="$1"
OUT="$2"
TMPDIR=$(mktemp -d)

python3 -m core.parser_testing.cli "$SRC" --mode ast --output-dir "$TMPDIR"

BASENAME=$(basename "$SRC")
AST_JSON="$TMPDIR/${BASENAME}.ast.json"

if [ ! -f "$AST_JSON" ]; then
  echo "ERROR: AST JSON was not generated: $AST_JSON" >&2
  exit 2
fi

mv "$AST_JSON" "$OUT"
echo "Generated AST JSON: $OUT"

rm -rf "$TMPDIR"
