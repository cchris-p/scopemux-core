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
# Clean any previous build artifacts
rm -rf build
# Run setup.py to build directly to ../build/core/
python3 setup.py build_ext
cd ..

echo "[build_all_and_pybind.sh] Build and Python binding complete."

# Diagnostic: Check that the Python extension exports the expected symbol
SO_FILE=$(find build/core -maxdepth 1 -name "scopemux_core*.so" | head -n 1)
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

# Remove old installed versions from site-packages
SITE_PACKAGES=$(python3 -c "import site; print(site.getsitepackages()[0])")
echo "[build_all_and_pybind.sh] Removing old installed scopemux_core packages from site-packages..."
rm -rf $SITE_PACKAGES/scopemux_core*
rm -rf $SITE_PACKAGES/scopemux_core-*.egg

# (Optional) Uninstall via pip as well, if installed as a package
echo "[build_all_and_pybind.sh] Attempting to uninstall scopemux_core via pip (if it was ever installed as a package). If you see a 'Skipping scopemux_core as it is not installed.' warning, this is normal and just means there was nothing to remove."
pip uninstall -y scopemux_core || true

# Copy the .so file to scopemux_core.so for import compatibility
# Make sure build/core directory exists
mkdir -p build/core
# Copy the .so file from core/ to build/core/ if needed
if [ -f core/scopemux_core.cpython-*.so ] && [ ! -f build/core/scopemux_core.so ]; then
  cp core/scopemux_core.cpython-*.so build/core/scopemux_core.so
fi

# Show where all .so files are located for debugging
echo "[build_all_and_pybind.sh] Checking all scopemux_core*.so files in the project:"
find . -name "scopemux_core*.so" -exec ls -la {} \;

# Ensure build directory is first in PYTHONPATH for local testing
export PYTHONPATH="$(pwd)/build/core:$PYTHONPATH"
echo "[build_all_and_pybind.sh] PYTHONPATH set to: $PYTHONPATH"

# Print the path of the loaded module for verification
python3 -c "import sys; sys.path.insert(0, '$(pwd)/build/core'); import scopemux_core; print('Loaded scopemux_core from:', scopemux_core.__file__)"
