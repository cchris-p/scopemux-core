#!/bin/bash
# build_shared_libs.sh
# Build shared libraries from static libraries for Tree-sitter languages

BUILD_DIR="$PWD/build/tree-sitter-libs"
echo "Building shared libraries in $BUILD_DIR"

cd $BUILD_DIR || {
    echo "Error: Cannot change to directory $BUILD_DIR"
    exit 1
}

# Create shared libraries from static libraries
echo "Converting static libraries to shared libraries..."
gcc -shared -o libtree-sitter.so -Wl,--whole-archive libtree-sitter.a -Wl,--no-whole-archive
gcc -shared -o libtree-sitter-c.so -Wl,--whole-archive libtree-sitter-c.a -Wl,--no-whole-archive
gcc -shared -o libtree-sitter-cpp.so -Wl,--whole-archive libtree-sitter-cpp.a -Wl,--no-whole-archive
gcc -shared -o libtree-sitter-python.so -Wl,--whole-archive libtree-sitter-python.a -Wl,--no-whole-archive
gcc -shared -o libtree-sitter-javascript.so -Wl,--whole-archive libtree-sitter-javascript.a -Wl,--no-whole-archive
gcc -shared -o libtree-sitter-typescript.so -Wl,--whole-archive libtree-sitter-typescript.a -Wl,--no-whole-archive

# Verify shared libraries were created
echo "Created shared libraries:"
ls -la *.so

echo "Done building shared libraries."
