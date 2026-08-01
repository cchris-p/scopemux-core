#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

# ScopeMux TypeScript Tests Runner Script
# Uses the shared test runner library for standardized test execution
#
# NOTE: This script uses a unique build directory (build-ts) to allow parallel test execution across languages.
# This prevents race conditions and build directory conflicts with other test runners.

# Source the shared test runner library
source "${SCRIPT_DIR}/test_runner_lib.sh"

setup_runner_logging "$0" "$PROJECT_ROOT_DIR"
initialize_runner_build_dir "$PROJECT_ROOT_DIR" "build-ts"

# Exit on any error (disabled during test loop to allow all tests to run)
# set -e

# Initialize global counters
TEST_FAILURES=0

# TypeScript Language Test Toggles
RUN_TS_BASIC_AST_TESTS=true
RUN_TS_EXAMPLE_AST_TESTS=true
RUN_TS_CST_TESTS=false # Disabled - source files don't exist yet

# TypeScript example test directory toggles
RUN_TS_BASIC_SYNTAX_TESTS=true
RUN_TS_INTERFACES_TESTS=false
RUN_TS_GENERICS_TESTS=false
RUN_TS_CLASSES_TESTS=false

# Set parallel jobs for test execution
PARALLEL_JOBS=1

# TypeScript language test executables
TS_BASIC_AST_EXECUTABLE_RELPATH="core/tests/ts_basic_ast_tests"
TS_EXAMPLE_AST_EXECUTABLE_RELPATH="core/tests/ts_example_ast_tests"
TS_CST_EXECUTABLE_RELPATH="core/tests/ts_cst_tests"

# Command-line flag parsing for advanced options
CLEAN_BUILD=true

# Process command line arguments
for arg in "$@"; do
    case $arg in
    --no-clean)
        CLEAN_BUILD=false
        echo "[run_ts_tests.sh] Skipping clean build"
        ;;
    --help)
        echo "Usage: ./run_ts_tests.sh [options]"
        echo "Options:"
        echo "  --no-clean      : Skip cleaning build directory"
        echo "  --help          : Show this help message"
        exit 0
        ;;
    esac
done

# Prepare build directory (clean or not, depending on flag)
prepare_and_configure_build "$PROJECT_ROOT_DIR" "$CMAKE_BUILD_DIR" "$CLEAN_BUILD"

# Run standard TypeScript language tests
echo "[run_ts_tests.sh] Running TypeScript language test suite"

# Run basic TypeScript tests if enabled
if [ "${RUN_TS_BASIC_AST_TESTS}" = true ]; then
    build_and_run_test_target "run_ts_tests.sh" "$CMAKE_BUILD_DIR" "ts_basic_ast_tests" "TypeScript Basic AST Tests" "$TS_BASIC_AST_EXECUTABLE_RELPATH"
fi

# Gather enabled TypeScript example test categories
TS_TEST_CATEGORIES=()
if [ "$RUN_TS_BASIC_SYNTAX_TESTS" = true ]; then
    TS_TEST_CATEGORIES+=("basic_syntax")
fi
if [ "$RUN_TS_INTERFACES_TESTS" = true ]; then
    TS_TEST_CATEGORIES+=("interfaces")
fi
if [ "$RUN_TS_GENERICS_TESTS" = true ]; then
    TS_TEST_CATEGORIES+=("generics")
fi
if [ "$RUN_TS_CLASSES_TESTS" = true ]; then
    TS_TEST_CATEGORIES+=("classes")
fi

# Run per-directory TypeScript example tests if any are enabled
if [ "${#TS_TEST_CATEGORIES[@]}" -gt 0 ]; then
    process_language_tests ts TS_TEST_CATEGORIES "$CMAKE_BUILD_DIR/core/tests/ts_example_ast_tests" "$PARALLEL_JOBS" ".ts"
fi

# Run CST tests if enabled
if [ "${RUN_TS_CST_TESTS}" = true ]; then
    build_and_run_test_target "run_ts_tests.sh" "$CMAKE_BUILD_DIR" "ts_cst_tests" "TypeScript CST Tests" "$TS_CST_EXECUTABLE_RELPATH"
fi

# Let the shared library handle the final test summary and exit code
print_test_summary
