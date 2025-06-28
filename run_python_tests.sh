#!/bin/bash

# All output from this script (stdout and stderr) is written to a file named after the script and the current datetime.
# Filename format: run_python_tests-YYYYMMDD-HHMMSS.txt (UTC)
# This is useful for preserving logs for each test run.
SCRIPT_BASENAME="$(basename "$0" .sh)"
RUN_DATETIME="$(date -u +"%Y%m%d-%H%M%S")"
OUTPUT_FILE="${SCRIPT_BASENAME}-${RUN_DATETIME}.txt"
# Redirect all output to the log file (both stdout and stderr)
exec > >(tee "$OUTPUT_FILE") 2>&1

# ScopeMux Python Tests Runner Script
# Uses the shared test runner library for standardized test execution
#
# NOTE: This script uses a unique build directory (build-python) to allow parallel test execution across languages.
# This prevents race conditions and build directory conflicts with other test runners.

# Source the shared test runner library
source scripts/test_runner_lib.sh

# Exit on any error (disabled during test loop to allow all tests to run)
# set -e

# Initialize global counters
TEST_FAILURES=0

# Python Language Test Toggles
RUN_PYTHON_BASIC_AST_TESTS=true
RUN_PYTHON_EXAMPLE_AST_TESTS=true
RUN_PYTHON_CST_TESTS=true

# Python example test directory toggles
RUN_PYTHON_BASIC_SYNTAX_TESTS=true
RUN_PYTHON_COMPLEX_STRUCTURES_TESTS=true
RUN_PYTHON_CLASS_TESTS=true
RUN_PYTHON_FUNCTION_TESTS=true

# Project root directory (assuming this script is in the root)
PROJECT_ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
CMAKE_PROJECT_BUILD_DIR="${PROJECT_ROOT_DIR}/build-python"

# Set parallel jobs for test execution
PARALLEL_JOBS=1

# Python language test executables
PYTHON_BASIC_AST_EXECUTABLE_RELPATH="core/tests/python_basic_ast_tests"
PYTHON_EXAMPLE_AST_EXECUTABLE_RELPATH="core/tests/python_example_ast_tests"
PYTHON_CST_EXECUTABLE_RELPATH="core/tests/python_cst_tests"

# Command-line flag parsing for advanced options
CLEAN_BUILD=true

# Process command line arguments
for arg in "$@"; do
    case $arg in
    --no-clean)
        CLEAN_BUILD=false
        echo "[run_python_tests.sh] Skipping clean build"
        ;;
    --help)
        echo "Usage: ./run_python_tests.sh [options]"
        echo "Options:"
        echo "  --no-clean      : Skip cleaning build directory"
        echo "  --help          : Show this help message"
        exit 0
        ;;
    esac
done

# Prepare build directory (clean or not, depending on flag)
prepare_clean_build_dir "$CMAKE_PROJECT_BUILD_DIR" "$CLEAN_BUILD"

# Setup CMake configuration using the shared library
setup_cmake_config "$PROJECT_ROOT_DIR"

# Run standard Python language tests
echo "[run_python_tests.sh] Running Python language test suite"

# Run basic Python tests if enabled
if [ "${RUN_PYTHON_BASIC_AST_TESTS}" = true ]; then
    build_test_target "python_basic_ast_tests" "Python Basic AST Tests"
    run_test_suite "Python Basic AST Tests" "${CMAKE_PROJECT_BUILD_DIR}/${PYTHON_BASIC_AST_EXECUTABLE_RELPATH}"
    if [ $? -ne 0 ]; then TEST_FAILURES=$((TEST_FAILURES + 1)); fi
fi

# Gather enabled Python example test categories
PYTHON_TEST_CATEGORIES=()
if [ "$RUN_PYTHON_BASIC_SYNTAX_TESTS" = true ]; then
    PYTHON_TEST_CATEGORIES+=("basic_syntax")
fi
if [ "$RUN_PYTHON_COMPLEX_STRUCTURES_TESTS" = true ]; then
    PYTHON_TEST_CATEGORIES+=("complex_structures")
fi
if [ "$RUN_PYTHON_CLASS_TESTS" = true ]; then
    PYTHON_TEST_CATEGORIES+=("classes")
fi
if [ "$RUN_PYTHON_FUNCTION_TESTS" = true ]; then
    PYTHON_TEST_CATEGORIES+=("functions")
fi

# Run per-directory Python example tests if any are enabled
if [ "${#PYTHON_TEST_CATEGORIES[@]}" -gt 0 ]; then
    process_language_tests python PYTHON_TEST_CATEGORIES "$CMAKE_PROJECT_BUILD_DIR/core/tests/python_example_ast_tests" "$PARALLEL_JOBS" ".py"
fi

# Run CST tests if enabled
if [ "${RUN_PYTHON_CST_TESTS}" = true ]; then
    build_test_target "python_cst_tests" "Python CST Tests"
    run_test_suite "Python CST Tests" "${CMAKE_PROJECT_BUILD_DIR}/${PYTHON_CST_EXECUTABLE_RELPATH}"
    if [ $? -ne 0 ]; then TEST_FAILURES=$((TEST_FAILURES + 1)); fi
fi

# Let the shared library handle the final test summary and exit code
print_test_summary
