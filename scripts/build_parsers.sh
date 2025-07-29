#!/bin/bash
# Script to build parse_cst and parse_ast executables for ScopeMux C parser
# This script assumes gcc and all dependencies are installed and run from project root or scripts dir

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.."; pwd)"
SRC_DIR="$PROJECT_ROOT/core/src"
INCLUDE_DIR="$PROJECT_ROOT/core/include"
SCRIPTS_DIR="$PROJECT_ROOT/scripts"

# Common flags - include all necessary paths and libraries
CFLAGS="-I$INCLUDE_DIR -I$SRC_DIR -I$SRC_DIR/parser -I$SRC_DIR/common -g -O2 -Wall -Wextra -std=c11"
# Use local tree-sitter libraries from build-c
TREE_SITTER_LIBS="$PROJECT_ROOT/build-c/tree-sitter-libs"
LDFLAGS="-L$TREE_SITTER_LIBS -ltree-sitter -ltree-sitter-c -lm"

# Common source files needed by both executables
COMMON_SOURCES="
$SRC_DIR/common/memory_management.c
$SRC_DIR/common/logging.c
$SRC_DIR/common/language.c
$SRC_DIR/parser/parser_context.c
$SRC_DIR/parser/query_manager.c
$SRC_DIR/parser/ts_init.c
$SRC_DIR/parser/parser.c
$SRC_DIR/parser/ast_node.c
"

# Build parse_cst
if [ -f "$SRC_DIR/parser/parse_cst.c" ]; then
  echo "Building parse_cst..."
  gcc $CFLAGS "$SRC_DIR/parser/parse_cst.c" \
    "$SRC_DIR/parser/cst_node.c" \
    "$SRC_DIR/parser/ts_cst_builder.c" \
    $COMMON_SOURCES \
    -o "$SCRIPTS_DIR/parse_cst" $LDFLAGS
  echo "Built $SCRIPTS_DIR/parse_cst"
else
  echo "parse_cst.c not found in $SRC_DIR/parser. Skipping parse_cst build."
fi

# Build parse_ast
if [ -f "$SRC_DIR/parser/parse_ast.c" ]; then
  echo "Building parse_ast..."
  gcc $CFLAGS "$SRC_DIR/parser/parse_ast.c" \
    $COMMON_SOURCES \
    -o "$SCRIPTS_DIR/parse_ast" $LDFLAGS
  echo "Built $SCRIPTS_DIR/parse_ast"
else
  echo "parse_ast.c not found in $SRC_DIR/parser. Skipping parse_ast build."
fi

echo "Executables built in $SCRIPTS_DIR:"
ls -lh "$SCRIPTS_DIR/parse_"* 2>/dev/null || echo "No parse_* executables found"
