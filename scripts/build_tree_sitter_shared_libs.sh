#!/bin/bash

# This script builds shared libraries for Tree-sitter grammars
# Run from the project root.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "[build_tree_sitter_shared_libs.sh] Building shared libraries for Tree-sitter grammars..."

"${SCRIPT_DIR}/build_shared_libs.sh"

echo "[build_tree_sitter_shared_libs.sh] Shared libraries built successfully."
