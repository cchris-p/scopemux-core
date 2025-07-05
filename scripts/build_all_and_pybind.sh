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

# Diagnostic: Check that the Python extension exports the expected symbol
SO_FILE=$(find core -maxdepth 1 -name "scopemux_core*.so" | head -n 1)
if [ -z "$SO_FILE" ]; then
  echo "[ERROR] Could not find scopemux_core .so after build. Python bindings will not work."
  exit 1
fi

if ! nm -D "$SO_FILE" | grep -q 'register_all_language_compliance'; then
  echo "[ERROR] Symbol register_all_language_compliance is missing from $SO_FILE."
  echo "         The Python bindings will fail to import. Check CMake sources and symbol export settings."
  exit 1
else
  echo "[OK] register_all_language_compliance symbol found in $SO_FILE."
fi
