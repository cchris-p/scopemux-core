#!/bin/bash

# ScopeMux Python Tests Runner Script
# Uses the shared test runner library for standardized test execution

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
CMAKE_PROJECT_BUILD_DIR="${PROJECT_ROOT_DIR}/build"

# Set parallel jobs for test execution
PARALLEL_JOBS=4

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

# Define the Python test categories array for example tests
PYTHON_TEST_CATEGORIES=()
if [ "${RUN_PYTHON_BASIC_SYNTAX_TESTS}" = true ]; then
    PYTHON_TEST_CATEGORIES+=("basic_syntax")
fi
if [ "${RUN_PYTHON_COMPLEX_STRUCTURES_TESTS}" = true ]; then
    PYTHON_TEST_CATEGORIES+=("complex_structures")
fi
if [ "${RUN_PYTHON_CLASS_TESTS}" = true ]; then
    PYTHON_TEST_CATEGORIES+=("classes")
fi
if [ "${RUN_PYTHON_FUNCTION_TESTS}" = true ]; then
    PYTHON_TEST_CATEGORIES+=("functions")
fi

# Process all Python example tests using the shared library
echo "[run_python_tests.sh] Processing Python example test directories (with recursive scanning)"
if [ ${#PYTHON_TEST_CATEGORIES[@]} -gt 0 ]; then
    # Build the example test executable if we have any categories to run
    build_test_target "python_example_ast_tests" "Python Example AST Tests"
    
    # Run all the Python example tests from the categories
    process_language_tests \
        "python" \
        PYTHON_TEST_CATEGORIES \
        "${CMAKE_PROJECT_BUILD_DIR}/core/tests/python_example_ast_tests" \
        "${PARALLEL_JOBS}" \
        ".py"
    
    # Track failures from the language test processor
    if [ $? -gt 0 ]; then 
        TEST_FAILURES=$((TEST_FAILURES + 1))
    fi
fi

# Run CST tests if enabled
if [ "${RUN_PYTHON_CST_TESTS}" = true ]; then
    build_test_target "python_cst_tests" "Python CST Tests"
    run_test_suite "Python CST Tests" "${CMAKE_PROJECT_BUILD_DIR}/${PYTHON_CST_EXECUTABLE_RELPATH}"
    if [ $? -ne 0 ]; then TEST_FAILURES=$((TEST_FAILURES + 1)); fi
fi

# Let the shared library handle the final test summary and exit code
print_test_summary
