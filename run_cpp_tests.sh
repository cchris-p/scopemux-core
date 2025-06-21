#!/bin/bash

# ScopeMux C++ Test Runner (standardized)
# Uses shared test runner library for consistent logic across languages

# Source the shared test runner library
source scripts/test_runner_lib.sh

# Exit on any error
set -e

# Initialize global counters
TEST_FAILURES=0

# C++ Language Test Toggles
RUN_CPP_BASIC_AST_TESTS=true
RUN_CPP_CST_TESTS=true

# C++ example test directory toggles
RUN_CPP_BASIC_SYNTAX_TESTS=true
RUN_CPP_COMPLEX_STRUCTURES_TESTS=true
RUN_CPP_MODERN_CPP_TESTS=true
RUN_CPP_TEMPLATES_TESTS=true

# Project root directory (assuming this script is in the root)
PROJECT_ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
CMAKE_PROJECT_BUILD_DIR="${PROJECT_ROOT_DIR}/build"

# Set parallel jobs for test execution
PARALLEL_JOBS=4

# C++ language test executables
CPP_BASIC_AST_EXECUTABLE_RELPATH="core/tests/cpp_basic_ast_tests"
CPP_EXAMPLE_AST_EXECUTABLE_RELPATH="core/tests/cpp_example_ast_tests"
CPP_CST_EXECUTABLE_RELPATH="core/tests/cpp_cst_tests"

# Command-line flag parsing for advanced options
CLEAN_BUILD=true

# Process command line arguments
for arg in "$@"; do
    case $arg in
    --no-clean)
        CLEAN_BUILD=false
        echo "[run_cpp_tests.sh] Skipping clean build"
        ;;
    --help)
        echo "Usage: ./run_cpp_tests.sh [options]"
        echo "Options:"
        echo "  --no-clean      : Skip cleaning build directory"
        echo "  --help          : Show this help message"
        exit 0
        ;;
    esac
done

# Handle build directory preparation
if [ "$CLEAN_BUILD" = true ] && [ -d "$CMAKE_PROJECT_BUILD_DIR" ]; then
    echo "[run_cpp_tests.sh] Cleaning build directory: $CMAKE_PROJECT_BUILD_DIR"
    rm -rf "$CMAKE_PROJECT_BUILD_DIR"
fi

# Create build directory if it doesn't exist
if [ ! -d "$CMAKE_PROJECT_BUILD_DIR" ]; then
    echo "[run_cpp_tests.sh] Creating build directory: $CMAKE_PROJECT_BUILD_DIR"
    mkdir -p "$CMAKE_PROJECT_BUILD_DIR"
fi

# Setup CMake configuration using the shared library
setup_cmake_config "$PROJECT_ROOT_DIR"

# Run standard C++ language tests
echo "[run_cpp_tests.sh] Running C++ language test suite"

# Run basic C++ tests if enabled
if [ "${RUN_CPP_BASIC_AST_TESTS}" = true ]; then
    build_test_target "cpp_basic_ast_tests" "C++ Basic AST Tests"
    run_test_suite "C++ Basic AST Tests" "${CMAKE_PROJECT_BUILD_DIR}/${CPP_BASIC_AST_EXECUTABLE_RELPATH}"
    if [ $? -ne 0 ]; then TEST_FAILURES=$((TEST_FAILURES + 1)); fi
fi

# Define the C++ test categories array for example tests
CPP_TEST_CATEGORIES=()
if [ "${RUN_CPP_BASIC_SYNTAX_TESTS}" = true ]; then
    CPP_TEST_CATEGORIES+=("basic_syntax")
fi
if [ "${RUN_CPP_COMPLEX_STRUCTURES_TESTS}" = true ]; then
    CPP_TEST_CATEGORIES+=("complex_structures")
fi
if [ "${RUN_CPP_MODERN_CPP_TESTS}" = true ]; then
    CPP_TEST_CATEGORIES+=("modern_cpp")
fi
if [ "${RUN_CPP_TEMPLATES_TESTS}" = true ]; then
    CPP_TEST_CATEGORIES+=("templates")
fi

# Process all C++ example tests using the shared library
echo "[run_cpp_tests.sh] Processing C++ example test directories (with recursive scanning)"
if [ ${#CPP_TEST_CATEGORIES[@]} -gt 0 ]; then
    # Build the example test executable if we have any categories to run
    build_test_target "cpp_example_ast_tests" "C++ Example AST Tests"
    
    # Run all the C++ example tests from the categories
    process_language_tests \
        "cpp" \
        CPP_TEST_CATEGORIES \
        "${CMAKE_PROJECT_BUILD_DIR}/core/tests/cpp_example_ast_tests" \
        "${PARALLEL_JOBS}" \
        ".cpp"
    
    # Track failures from the language test processor
    if [ $? -gt 0 ]; then 
        TEST_FAILURES=$((TEST_FAILURES + 1))
    fi
fi

# Run CST tests if enabled
if [ "${RUN_CPP_CST_TESTS}" = true ]; then
    build_test_target "cpp_cst_tests" "C++ CST Tests"
    run_test_suite "C++ CST Tests" "${CMAKE_PROJECT_BUILD_DIR}/${CPP_CST_EXECUTABLE_RELPATH}"
    if [ $? -ne 0 ]; then TEST_FAILURES=$((TEST_FAILURES + 1)); fi
fi

# Let the shared library handle the final test summary and exit code
finish_test_run
