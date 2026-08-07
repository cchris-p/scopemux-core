#!/bin/bash

# Create vendor directory if it doesn't exist
mkdir -p vendor

# Clone tree-sitter core and language grammars
cd vendor

# tree-sitter core
git clone https://github.com/tree-sitter/tree-sitter.git

# language grammars
git clone https://github.com/tree-sitter/tree-sitter-c.git
git clone https://github.com/tree-sitter/tree-sitter-cpp.git
git clone https://github.com/tree-sitter/tree-sitter-python.git
git clone https://github.com/tree-sitter/tree-sitter-javascript.git

# TypeScript requires special handling since we need the typescript subdirectory
git clone https://github.com/tree-sitter/tree-sitter-typescript.git

# Clone pybind11
git clone https://github.com/pybind/pybind11.git

# Build all tree-sitter libraries with -fPIC
echo "Building tree-sitter core..."
cd tree-sitter && make CFLAGS="-fPIC" && cd ..

echo "Building tree-sitter-c..."
cd tree-sitter-c && make CFLAGS="-fPIC" && cd ..

echo "Building tree-sitter-cpp..."
cd tree-sitter-cpp && make CFLAGS="-fPIC" && cd ..

echo "Building tree-sitter-python..."
cd tree-sitter-python && make CFLAGS="-fPIC" && cd ..

echo "Building tree-sitter-javascript..."
cd tree-sitter-javascript && make CFLAGS="-fPIC" && cd ..

echo "Building tree-sitter-typescript..."
cd tree-sitter-typescript/typescript && make CFLAGS="-fPIC" && cd ../..

# Create build directory for libraries
cd ..
mkdir -p build/tree-sitter-libs

# Copy static libraries to build directory
cp vendor/tree-sitter/libtree-sitter.a build/tree-sitter-libs/
cp vendor/tree-sitter-c/libtree-sitter-c.a build/tree-sitter-libs/
cp vendor/tree-sitter-cpp/libtree-sitter-cpp.a build/tree-sitter-libs/
cp vendor/tree-sitter-python/libtree-sitter-python.a build/tree-sitter-libs/
cp vendor/tree-sitter-javascript/libtree-sitter-javascript.a build/tree-sitter-libs/
cp vendor/tree-sitter-typescript/typescript/libtree-sitter-typescript.a build/tree-sitter-libs/
