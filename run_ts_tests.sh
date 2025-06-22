#!/bin/bash

# ScopeMux TypeScript Tests Runner Script
# Uses the shared test runner library for standardized test execution

# Source the shared test runner library
source scripts/test_runner_lib.sh

# Exit on any error (disabled during test loop to allow all tests to run)
# set -e

# Initialize global counters
TEST_FAILURES=0

# TypeScript Language Test Toggles
RUN_TS_BASIC_AST_TESTS=true
RUN_TS_EXAMPLE_AST_TESTS=true
RUN_TS_CST_TESTS=false  # Disabled - source files don't exist yet

# TypeScript example test directory toggles
RUN_TS_BASIC_SYNTAX_TESTS=true
RUN_TS_INTERFACES_TESTS=false
RUN_TS_GENERICS_TESTS=false
RUN_TS_CLASSES_TESTS=false

# Project root directory (assuming this script is in the root)
PROJECT_ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
CMAKE_PROJECT_BUILD_DIR="${PROJECT_ROOT_DIR}/build"

# Set parallel jobs for test execution
PARALLEL_JOBS=4

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
prepare_clean_build_dir "$CMAKE_PROJECT_BUILD_DIR" "$CLEAN_BUILD"

# Setup CMake configuration using the shared library
setup_cmake_config "$PROJECT_ROOT_DIR"

# Run standard TypeScript language tests
echo "[run_ts_tests.sh] Running TypeScript language test suite"

# Run basic TypeScript tests if enabled
if [ "${RUN_TS_BASIC_AST_TESTS}" = true ]; then
    build_test_target "ts_basic_ast_tests" "TypeScript Basic AST Tests"
    build_result=$?
    
    if [ $build_result -ne 0 ]; then
        echo "[run_ts_tests.sh] ERROR: Failed to build ts_basic_ast_tests"
        ((TEST_FAILURES++))
    else
        # Change to build directory and get absolute path to executable
        cd "$CMAKE_PROJECT_BUILD_DIR"
        make "ts_basic_ast_tests"
        
        # Verify that the executable was built
        if [ ! -f "core/tests/ts_basic_ast_tests" ]; then
            echo "[run_ts_tests.sh] ERROR: Executable not found at core/tests/ts_basic_ast_tests"
            ((TEST_FAILURES++))
        else
            # Get the absolute path to the executable
            executable_path="$(pwd)/core/tests/ts_basic_ast_tests"
            
            # Run the test
            run_test_suite "TypeScript Basic AST Tests" "$executable_path"
            if [ $? -ne 0 ]; then TEST_FAILURES=$((TEST_FAILURES + 1)); fi
        fi
    fi
fi

# Define the TypeScript test categories array for example tests
TS_TEST_CATEGORIES=()
if [ "${RUN_TS_BASIC_SYNTAX_TESTS}" = true ]; then
    TS_TEST_CATEGORIES+=("basic_syntax")
fi
if [ "${RUN_TS_INTERFACES_TESTS}" = true ]; then
    TS_TEST_CATEGORIES+=("interfaces")
fi
if [ "${RUN_TS_GENERICS_TESTS}" = true ]; then
    TS_TEST_CATEGORIES+=("generics")
fi
if [ "${RUN_TS_CLASSES_TESTS}" = true ]; then
    TS_TEST_CATEGORIES+=("classes")
fi

# Process all TypeScript example tests using the shared library
echo "[run_ts_tests.sh] Processing TypeScript example test directories (with recursive scanning)"
if [ ${#TS_TEST_CATEGORIES[@]} -gt 0 ]; then
    # Build the example test executable if we have any categories to run
    build_test_target "ts_example_ast_tests" "TypeScript Example AST Tests"
    
    # Run all the TypeScript example tests from the categories
    process_language_tests \
        "ts" \
        TS_TEST_CATEGORIES \
        "${CMAKE_PROJECT_BUILD_DIR}/core/tests/ts_example_ast_tests" \
        "${PARALLEL_JOBS}" \
        ".ts"
    
    # Track failures from the language test processor
    if [ $? -gt 0 ]; then 
        TEST_FAILURES=$((TEST_FAILURES + 1))
    fi
fi

# Run CST tests if enabled
if [ "${RUN_TS_CST_TESTS}" = true ]; then
    build_test_target "ts_cst_tests" "TypeScript CST Tests"
    build_result=$?
    
    if [ $build_result -ne 0 ]; then
        echo "[run_ts_tests.sh] ERROR: Failed to build ts_cst_tests"
        ((TEST_FAILURES++))
    else
        # Change to build directory and get absolute path to executable
        cd "$CMAKE_PROJECT_BUILD_DIR"
        make "ts_cst_tests"
        
        # Verify that the executable was built
        if [ ! -f "core/tests/ts_cst_tests" ]; then
            echo "[run_ts_tests.sh] ERROR: Executable not found at core/tests/ts_cst_tests"
            ((TEST_FAILURES++))
        else
            # Get the absolute path to the executable
            executable_path="$(pwd)/core/tests/ts_cst_tests"
            
            # Run the test
            run_test_suite "TypeScript CST Tests" "$executable_path"
            if [ $? -ne 0 ]; then TEST_FAILURES=$((TEST_FAILURES + 1)); fi
        fi
    fi
fi

# Let the shared library handle the final test summary and exit code
print_test_summary
