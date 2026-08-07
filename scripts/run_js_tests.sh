#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

# ScopeMux JS Tests Runner Script
# Uses the shared test runner library for standardized test execution
#
# NOTE: This script uses a unique build directory (build-js) to allow parallel test execution across languages.
# This prevents race conditions and build directory conflicts with other test runners.

# Source the shared test runner library
source "${SCRIPT_DIR}/test_runner_lib.sh"

setup_runner_logging "$0" "$PROJECT_ROOT_DIR"
initialize_runner_build_dir "$PROJECT_ROOT_DIR" "build-js"

# Exit on any error (disabled during test loop to allow all tests to run)
# set -e

# Initialize global counters
TEST_FAILURES=0

# JavaScript Language Test Toggles
RUN_JS_BASIC_AST_TESTS=true
RUN_JS_EXAMPLE_AST_TESTS=true
RUN_JS_CST_TESTS=false # Disabled - source files don't exist yet

# Set parallel jobs for test execution
PARALLEL_JOBS=1

# JavaScript language test executables
JS_BASIC_AST_EXECUTABLE_RELPATH="core/tests/js_basic_ast_tests"
JS_EXAMPLE_AST_EXECUTABLE_RELPATH="core/tests/js_example_ast_tests"
JS_CST_EXECUTABLE_RELPATH="core/tests/js_cst_tests"

# Command-line flag parsing for advanced options
CLEAN_BUILD=true

# Process command line arguments
for arg in "$@"; do
    case $arg in
    --no-clean)
        CLEAN_BUILD=false
        echo "[run_js_tests.sh] Skipping clean build"
        ;;
    --help)
        echo "Usage: ./run_js_tests.sh [options]"
        echo "Options:"
        echo "  --no-clean      : Skip cleaning build directory"
        echo "  --help          : Show this help message"
        exit 0
        ;;
    esac
done

# Prepare build directory (clean or not, depending on flag)
prepare_and_configure_build "$PROJECT_ROOT_DIR" "$CMAKE_BUILD_DIR" "$CLEAN_BUILD"

# Run standard JavaScript language tests
echo "[run_js_tests.sh] Running JavaScript language test suite"

# Run basic JavaScript tests if enabled
if [ "${RUN_JS_BASIC_AST_TESTS}" = true ]; then
    build_and_run_test_target "run_js_tests.sh" "$CMAKE_BUILD_DIR" "js_basic_ast_tests" "JavaScript Basic AST Tests" "$JS_BASIC_AST_EXECUTABLE_RELPATH"
fi

# Run manifest-defined JavaScript example tests if enabled
if [ "${RUN_JS_EXAMPLE_AST_TESTS}" = true ]; then
    load_example_categories js JS_TEST_CATEGORIES || exit 1
    echo "[run_js_tests.sh] Building JavaScript example AST tests executable..."
    build_test_target "js_example_ast_tests" "$CMAKE_BUILD_DIR" "JavaScript Example AST Tests"
    build_result=$?
    if [ $build_result -ne 0 ]; then
        echo "[run_js_tests.sh] ERROR: Failed to build js_example_ast_tests, skipping example tests."
        ((TEST_FAILURES++))
    else
        process_language_tests js JS_TEST_CATEGORIES "$CMAKE_BUILD_DIR/core/tests/js_example_ast_tests" "$PARALLEL_JOBS" ".js"
    fi
fi

# Run CST tests if enabled
if [ "${RUN_JS_CST_TESTS}" = true ]; then
    build_and_run_test_target "run_js_tests.sh" "$CMAKE_BUILD_DIR" "js_cst_tests" "JavaScript CST Tests" "$JS_CST_EXECUTABLE_RELPATH"
fi

# Let the shared library handle the final test summary and exit code
print_test_summary
