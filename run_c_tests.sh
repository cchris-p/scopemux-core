#!/bin/bash

# ScopeMux C Tests Runner Script
# Uses the shared test runner library for standardized test execution

# Source the shared test runner library
source scripts/test_runner_lib.sh

# Exit on any error
set -e

# Initialize global counters
TEST_FAILURES=0

# C Language Test Toggles
RUN_C_BASIC_AST_TESTS=true
RUN_C_CST_TESTS=false
RUN_C_PREPROCESSOR_TESTS=false

# C example test directory toggles
RUN_C_BASIC_SYNTAX_TESTS=true
RUN_C_COMPLEX_STRUCTURES_TESTS=true
RUN_C_FILE_IO_TESTS=true
RUN_C_MEMORY_MANAGEMENT_TESTS=true
RUN_C_STRUCT_UNION_ENUM_TESTS=true

# Project root directory (assuming this script is in the root)
PROJECT_ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
CMAKE_PROJECT_BUILD_DIR="${PROJECT_ROOT_DIR}/build"

# Set parallel jobs for test execution
PARALLEL_JOBS=4

# C language test executables
C_BASIC_AST_EXECUTABLE_RELPATH="core/tests/c_basic_ast_tests"
C_EXAMPLE_AST_EXECUTABLE_RELPATH="core/tests/c_example_ast_tests"
C_CST_EXECUTABLE_RELPATH="core/tests/c_cst_tests"
C_PREPROCESSOR_EXECUTABLE_RELPATH="core/tests/c_preprocessor_tests"

# Command-line flag parsing for advanced options
CLEAN_BUILD=true

# Process command line arguments
for arg in "$@"; do
    case $arg in
    --no-clean)
        CLEAN_BUILD=false
        echo "[run_c_tests.sh] Skipping clean build"
        ;;
    --help)
        echo "Usage: ./run_c_tests.sh [options]"
        echo "Options:"
        echo "  --no-clean      : Skip cleaning build directory"
        echo "  --help          : Show this help message"
        exit 0
        ;;
    esac
done

# Handle build directory preparation
if [ "$CLEAN_BUILD" = true ] && [ -d "$CMAKE_PROJECT_BUILD_DIR" ]; then
    echo "[run_c_tests.sh] Cleaning build directory: $CMAKE_PROJECT_BUILD_DIR"
    rm -rf "$CMAKE_PROJECT_BUILD_DIR"
fi

# Create build directory if it doesn't exist
if [ ! -d "$CMAKE_PROJECT_BUILD_DIR" ]; then
    echo "[run_c_tests.sh] Creating build directory: $CMAKE_PROJECT_BUILD_DIR"
    mkdir -p "$CMAKE_PROJECT_BUILD_DIR"
fi

# Setup CMake configuration using the shared library
setup_cmake_config "$PROJECT_ROOT_DIR"

# Run standard C language tests
echo "[run_c_tests.sh] Running C language test suite"

# Run the basic AST tests if enabled
if [ "${RUN_C_BASIC_AST_TESTS}" = true ]; then
    build_test_target "c_basic_ast_tests" "C Basic AST Tests"
    run_test_suite "C Basic AST Tests" "${CMAKE_PROJECT_BUILD_DIR}/${C_BASIC_AST_EXECUTABLE_RELPATH}"
    if [ $? -ne 0 ]; then TEST_FAILURES=$((TEST_FAILURES + 1)); fi
fi

# Define the C test categories array for example tests
C_TEST_CATEGORIES=()
if [ "${RUN_C_BASIC_SYNTAX_TESTS}" = true ]; then
    C_TEST_CATEGORIES+=("basic_syntax")
fi
if [ "${RUN_C_COMPLEX_STRUCTURES_TESTS}" = true ]; then
    C_TEST_CATEGORIES+=("complex_structures")
fi
if [ "${RUN_C_FILE_IO_TESTS}" = true ]; then
    C_TEST_CATEGORIES+=("file_io")
fi
if [ "${RUN_C_MEMORY_MANAGEMENT_TESTS}" = true ]; then
    C_TEST_CATEGORIES+=("memory_management")
fi
if [ "${RUN_C_STRUCT_UNION_ENUM_TESTS}" = true ]; then
    C_TEST_CATEGORIES+=("struct_union_enum")
fi

# Process all C example tests using the shared library
echo "[run_c_tests.sh] Processing C example test directories (with recursive scanning)"
if [ ${#C_TEST_CATEGORIES[@]} -gt 0 ]; then
    # Build the example test executable if we have any categories to run
    build_test_target "c_example_ast_tests" "C Example AST Tests"
    
    # Run all the C example tests from the categories
    process_language_tests \
        "c" \
        C_TEST_CATEGORIES \
        "${CMAKE_PROJECT_BUILD_DIR}/core/tests/c_example_ast_tests" \
        "${PARALLEL_JOBS}" \
        ".c"
    
    # Track failures from the language test processor
    if [ $? -gt 0 ]; then 
        TEST_FAILURES=$((TEST_FAILURES + 1))
    fi
fi

# Run other test types if enabled
if [ "${RUN_C_CST_TESTS}" = true ]; then
    build_test_target "c_cst_tests" "C CST Tests"
    run_test_suite "C CST Tests" "${CMAKE_PROJECT_BUILD_DIR}/${C_CST_EXECUTABLE_RELPATH}"
    if [ $? -ne 0 ]; then TEST_FAILURES=$((TEST_FAILURES + 1)); fi
fi

if [ "${RUN_C_PREPROCESSOR_TESTS}" = true ]; then
    build_test_target "c_preprocessor_tests" "C Preprocessor Tests"
    run_test_suite "C Preprocessor Tests" "${CMAKE_PROJECT_BUILD_DIR}/${C_PREPROCESSOR_EXECUTABLE_RELPATH}"
    if [ $? -ne 0 ]; then TEST_FAILURES=$((TEST_FAILURES + 1)); fi
fi

# Let the shared library handle the final test summary and exit code
print_test_summary
