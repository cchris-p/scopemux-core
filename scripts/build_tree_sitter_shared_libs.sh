#!/bin/bash

# This script builds shared libraries for Tree-sitter grammars
# Run from the project root.

set -e

echo "[build_tree_sitter_shared_libs.sh] Building shared libraries for Tree-sitter grammars..."

# Create the tree-sitter-libs directory if it doesn't exist
mkdir -p build/tree-sitter-libs

# Build shared library for tree-sitter core
echo "Building libtree-sitter.so..."
gcc -shared -o build/tree-sitter-libs/libtree-sitter.so -Wl,--whole-archive build/tree-sitter-libs/libtree-sitter.a -Wl,--no-whole-archive

# Build shared libraries for each grammar
echo "Building libtree-sitter-c.so..."
gcc -shared -o build/tree-sitter-libs/libtree-sitter-c.so -Wl,--whole-archive build/tree-sitter-libs/libtree-sitter-c.a -Wl,--no-whole-archive -L build/tree-sitter-libs -ltree-sitter

echo "Building libtree-sitter-cpp.so..."
gcc -shared -o build/tree-sitter-libs/libtree-sitter-cpp.so -Wl,--whole-archive build/tree-sitter-libs/libtree-sitter-cpp.a -Wl,--no-whole-archive -L build/tree-sitter-libs -ltree-sitter

echo "Building libtree-sitter-python.so..."
gcc -shared -o build/tree-sitter-libs/libtree-sitter-python.so -Wl,--whole-archive build/tree-sitter-libs/libtree-sitter-python.a -Wl,--no-whole-archive -L build/tree-sitter-libs -ltree-sitter

echo "Building libtree-sitter-javascript.so..."
gcc -shared -o build/tree-sitter-libs/libtree-sitter-javascript.so -Wl,--whole-archive build/tree-sitter-libs/libtree-sitter-javascript.a -Wl,--no-whole-archive -L build/tree-sitter-libs -ltree-sitter

echo "Building libtree-sitter-typescript.so..."
gcc -shared -o build/tree-sitter-libs/libtree-sitter-typescript.so -Wl,--whole-archive build/tree-sitter-libs/libtree-sitter-typescript.a -Wl,--no-whole-archive -L build/tree-sitter-libs -ltree-sitter

echo "[build_tree_sitter_shared_libs.sh] Shared libraries built successfully."
