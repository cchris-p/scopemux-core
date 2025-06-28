#!/bin/bash

# ScopeMux C Tests Runner Script
# Uses the shared test runner library for standardized test execution
#
# NOTE: This script uses a unique build directory (build-c) to allow parallel test execution across languages.
# This prevents race conditions and build directory conflicts with other test runners.

# Source the shared test runner library
source scripts/test_runner_lib.sh

# Exit on any error (disabled during test loop to allow all tests to run)
# set -e

# Initialize global counters
TEST_FAILURES=0

# C Language Test Toggles
RUN_C_BASIC_AST_TESTS=false
# Note: The following tests are disabled because their source files don't exist yet
RUN_C_CST_TESTS=false
RUN_C_PREPROCESSOR_TESTS=false

# C example test directory toggles
RUN_C_BASIC_SYNTAX_TESTS=true
RUN_C_COMPLEX_STRUCTURES_TESTS=false
RUN_C_FILE_IO_TESTS=false
RUN_C_MEMORY_MANAGEMENT_TESTS=false
RUN_C_STRUCT_UNION_ENUM_TESTS=false

# Project root directory (assuming this script is in the root)
PROJECT_ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
CMAKE_PROJECT_BUILD_DIR="${PROJECT_ROOT_DIR}/build-c"

# Set parallel jobs for test execution
PARALLEL_JOBS=1

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

# Prepare build directory (clean or not, depending on flag)
prepare_clean_build_dir "$CMAKE_PROJECT_BUILD_DIR" "$CLEAN_BUILD"

# Setup CMake configuration using the shared library
setup_cmake_config "$PROJECT_ROOT_DIR"

# Run all C test executables in a robust, standardized way
# Define all C test targets and their display names
C_TEST_TARGETS=(
    "c_basic_ast_tests:C Basic AST Tests"
    "c_cst_tests:C CST Tests"
    "c_preprocessor_tests:C Preprocessor Tests"
)

# Map from target to executable relpath
declare -A C_TEST_EXECUTABLES
C_TEST_EXECUTABLES["c_basic_ast_tests"]="core/tests/c_basic_ast_tests"
C_TEST_EXECUTABLES["c_example_ast_tests"]="core/tests/c_example_ast_tests"
C_TEST_EXECUTABLES["c_cst_tests"]="core/tests/c_cst_tests"
C_TEST_EXECUTABLES["c_preprocessor_tests"]="core/tests/c_preprocessor_tests"

# Loop over all C test targets that have their own executables
for target in "${C_TEST_TARGETS[@]}"; do
    # Split the target:description string
    IFS=':' read -r test_name test_description <<<"$target"

    # Determine if the test should run based on its toggle
    should_run=false
    case "$test_name" in
    "c_basic_ast_tests")
        if [ "$RUN_C_BASIC_AST_TESTS" = true ]; then should_run=true; fi
        ;;
    "c_cst_tests")
        if [ "$RUN_C_CST_TESTS" = true ]; then
            echo "[run_c_tests.sh] WARNING: $test_name is enabled but its source files do not exist yet. Skipping."
        fi
        # This test is not ready, so we always skip it.
        continue
        ;;
    "c_preprocessor_tests")
        if [ "$RUN_C_PREPROCESSOR_TESTS" = true ]; then
            echo "[run_c_tests.sh] WARNING: $test_name is enabled but its source files do not exist yet. Skipping."
        fi
        # This test is not ready, so we always skip it.
        continue
        ;;
    esac

    # Skip if the toggle is false
    if [ "$should_run" = false ]; then
        continue
    fi

    # Build the test target
    echo "[run_c_tests.sh] Building $test_description ($test_name)..."
    build_test_target "$test_name" "$CMAKE_PROJECT_BUILD_DIR"
    build_result=$?

    if [ $build_result -ne 0 ]; then
        echo "[run_c_tests.sh] ERROR: Failed to build $test_name"
        ((TEST_FAILURES++))
        continue
    fi

    # Get the absolute path to the executable
    executable_path="${CMAKE_PROJECT_BUILD_DIR}/${C_TEST_EXECUTABLES[$test_name]}"

    # Verify that the executable was built
    if [ ! -f "$executable_path" ]; then
        echo "[run_c_tests.sh] ERROR: Executable not found at $executable_path"
        ((TEST_FAILURES++))
        continue
    fi

    # Run the test
    echo "[run_c_tests.sh] Running $test_description..."
    run_test_suite "$test_description" "$executable_path"
    test_result=$?

    if [ $test_result -ne 0 ]; then
        echo "[run_c_tests.sh] ERROR: Test $test_name failed with exit code $test_result"
        ((TEST_FAILURES++))
    else
        echo "[run_c_tests.sh] Test $test_name passed"
    fi
done
# Gather enabled C example test categories
C_CATEGORIES=()
if [ "$RUN_C_BASIC_SYNTAX_TESTS" = true ]; then
    C_CATEGORIES+=("basic_syntax")
fi
if [ "$RUN_C_COMPLEX_STRUCTURES_TESTS" = true ]; then
    C_CATEGORIES+=("complex_structures")
fi
if [ "$RUN_C_FILE_IO_TESTS" = true ]; then
    C_CATEGORIES+=("file_io")
fi
if [ "$RUN_C_MEMORY_MANAGEMENT_TESTS" = true ]; then
    C_CATEGORIES+=("memory_management")
fi
if [ "$RUN_C_STRUCT_UNION_ENUM_TESTS" = true ]; then
    C_CATEGORIES+=("struct_union_enum")
fi

# Run per-directory C example tests if any are enabled
if [ "${#C_CATEGORIES[@]}" -gt 0 ]; then
        echo "[run_c_tests.sh] Building C example AST tests executable..."
    build_test_target "c_example_ast_tests" "$CMAKE_PROJECT_BUILD_DIR"
    build_result=$?
    if [ $build_result -ne 0 ]; then
        echo "[run_c_tests.sh] ERROR: Failed to build c_example_ast_tests, skipping example tests."
        ((TEST_FAILURES++))
    else
        process_language_tests c C_CATEGORIES "$C_EXAMPLE_AST_EXECUTABLE_RELPATH"
    fi
fi

print_test_summary
