#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

# ScopeMux Rust Tests Runner Script
# Uses the shared test runner library for standardized test execution

# Source the shared test runner library
source "${SCRIPT_DIR}/test_runner_lib.sh"

setup_runner_logging "$0" "$PROJECT_ROOT_DIR"
initialize_runner_build_dir "$PROJECT_ROOT_DIR" "build-rust"

# Initialize global counters
TEST_FAILURES=0

# Rust Language Test Toggles
RUN_RUST_BASIC_AST_TESTS=true

# Set parallel jobs for test execution
PARALLEL_JOBS=1

# Rust language test executables
RUST_BASIC_AST_EXECUTABLE_RELPATH="core/tests/rust_basic_ast_tests"

# Command-line flag parsing for advanced options
CLEAN_BUILD=true

for arg in "$@"; do
    case $arg in
    --no-clean)
        CLEAN_BUILD=false
        echo "[run_rust_tests.sh] Skipping clean build"
        ;;
    --help)
        echo "Usage: ./run_rust_tests.sh [options]"
        echo "Options:"
        echo "  --no-clean      : Skip cleaning build directory"
        echo "  --help          : Show this help message"
        exit 0
        ;;
    esac
done

# Prepare build directory (clean or not, depending on flag)
prepare_and_configure_build "$PROJECT_ROOT_DIR" "$CMAKE_BUILD_DIR" "$CLEAN_BUILD"

# Run standard Rust language tests
echo "[run_rust_tests.sh] Running Rust language test suite"

if [ "${RUN_RUST_BASIC_AST_TESTS}" = true ]; then
    build_and_run_test_target "run_rust_tests.sh" "$CMAKE_BUILD_DIR" "rust_basic_ast_tests" "Rust Basic AST Tests" "$RUST_BASIC_AST_EXECUTABLE_RELPATH"
fi

# Let the shared library handle the final test summary and exit code
print_test_summary
