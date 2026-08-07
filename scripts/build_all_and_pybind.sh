#!/bin/bash

# This script builds all Tree-sitter grammars and the ScopeMux Python bindings.
# Run from the project root.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${PROJECT_ROOT_DIR}"

echo "[build_all_and_pybind.sh] Cleaning build directory to avoid stale CMake cache..."
rm -rf "${PROJECT_ROOT_DIR}/build"

echo "[build_all_and_pybind.sh] Installing scopemux_core in editable mode..."
python3 -m pip install -e ./core

echo "[build_all_and_pybind.sh] Build and Python binding complete."
