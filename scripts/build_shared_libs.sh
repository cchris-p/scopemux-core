#!/bin/bash
# build_shared_libs.sh
#
# Purpose:
#   Converts static Tree-sitter language libraries (.a) to shared libraries (.so)
#   required for dynamic loading at runtime (via dlopen/dlsym).
#
# Usage:
#   This script is called automatically by scripts/test_runner_lib.sh whenever
#   any required Tree-sitter .so is missing in build/tree-sitter-libs.
#   Manual invocation is not required except for debugging build issues.
#
# Caveats:
#   - Assumes static libraries have already been built (by build_all_and_pybind.sh).
#   - Must be run from project root or scripts directory; relies on $PWD.
#   - Do not modify unless you are changing the build pipeline for Tree-sitter grammars.
#
# Action:
#   For each language, create a .so from the .a using gcc -shared and --whole-archive.

BUILD_DIR="$PWD/build/tree-sitter-libs"
echo "[build_shared_libs.sh] Building shared libraries in $BUILD_DIR"

cd $BUILD_DIR || {
    echo "[build_shared_libs.sh] Error: Cannot change to directory $BUILD_DIR"
    exit 1
}

# Create shared libraries from static libraries
echo "[build_shared_libs.sh] Converting static libraries to shared libraries..."
gcc -shared -o libtree-sitter.so -Wl,--whole-archive libtree-sitter.a -Wl,--no-whole-archive
gcc -shared -o libtree-sitter-c.so -Wl,--whole-archive libtree-sitter-c.a -Wl,--no-whole-archive
gcc -shared -o libtree-sitter-cpp.so -Wl,--whole-archive libtree-sitter-cpp.a -Wl,--no-whole-archive
gcc -shared -o libtree-sitter-python.so -Wl,--whole-archive libtree-sitter-python.a -Wl,--no-whole-archive
gcc -shared -o libtree-sitter-javascript.so -Wl,--whole-archive libtree-sitter-javascript.a -Wl,--no-whole-archive
gcc -shared -o libtree-sitter-typescript.so -Wl,--whole-archive libtree-sitter-typescript.a -Wl,--no-whole-archive

# Verify shared libraries were created
echo "[build_shared_libs.sh] Created shared libraries:"
ls -la *.so

echo "[build_shared_libs.sh] Done building shared libraries."
