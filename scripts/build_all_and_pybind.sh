#!/bin/bash

# This script builds all Tree-sitter grammars and the ScopeMux Python bindings.
# Run from the project root.

set -e

echo "[build_all_and_pybind.sh] Cleaning build directory to avoid stale CMake cache..."
rm -rf build

echo "[build_all_and_pybind.sh] Ensuring build directory and CMake configuration..."

if [ ! -d build ]; then
  mkdir build
fi

if [ ! -f build/Makefile ]; then
  echo "[build_all_and_pybind.sh] Running CMake configuration in build/..."
  cmake -S . -B build
fi

echo "[build_all_and_pybind.sh] Building all Tree-sitter grammars and core libraries..."

cd build

cmake --build . --target tree_sitter_core
cmake --build . --target tree_sitter_c
cmake --build . --target tree_sitter_cpp
cmake --build . --target tree_sitter_python
cmake --build . --target tree_sitter_javascript
cmake --build . --target tree_sitter_typescript
cmake --build . --target scopemux_core

cd ..

echo "[build_all_and_pybind.sh] Running pybind in core/ directory..."
cd core
rm -rf ./build && python3 setup.py build_ext --inplace
cd ..

echo "[build_all_and_pybind.sh] Build and Python binding complete."
