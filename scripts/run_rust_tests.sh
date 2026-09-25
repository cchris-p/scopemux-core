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
RUN_RUST_EXAMPLE_AST_TESTS=true

# Set parallel jobs for test execution
PARALLEL_JOBS=1

# Rust language test executables
RUST_BASIC_AST_EXECUTABLE_RELPATH="core/tests/rust_basic_ast_tests"
RUST_EXAMPLE_AST_EXECUTABLE_RELPATH="core/tests/rust_example_ast_tests"

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

# Run manifest-defined Rust example tests if enabled
if [ "${RUN_RUST_EXAMPLE_AST_TESTS}" = true ]; then
    load_example_categories rust RUST_TEST_CATEGORIES || exit 1
    echo "[run_rust_tests.sh] Building Rust example AST tests executable..."
    build_test_target "rust_example_ast_tests" "$CMAKE_BUILD_DIR" "Rust Example AST Tests"
    build_result=$?
    if [ $build_result -ne 0 ]; then
        echo "[run_rust_tests.sh] ERROR: Failed to build rust_example_ast_tests, skipping example tests."
        ((TEST_FAILURES++))
    else
        process_language_tests rust RUST_TEST_CATEGORIES "$CMAKE_BUILD_DIR/core/tests/rust_example_ast_tests" "$PARALLEL_JOBS" ".rs"
    fi
fi

# Let the shared library handle the final test summary and exit code
print_test_summary
