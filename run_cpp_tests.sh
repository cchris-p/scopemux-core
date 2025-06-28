#!/bin/bash

# All output from this script (stdout and stderr) is written to a file named after the script.
# Filename format: run_cpp_tests.txt
# This is useful for preserving logs for each test run.
SCRIPT_BASENAME="$(basename "$0" .sh)"
OUTPUT_FILE="run_cpp_tests.txt"
# Redirect all output to the log file (both stdout and stderr)
exec > >(tee "$OUTPUT_FILE") 2>&1

# ScopeMux C++ Tests Runner Script
# Uses the shared test runner library for standardized test execution
#
# NOTE: This script uses a unique build directory (build-cpp) to allow parallel test execution across languages.
# This prevents race conditions and build directory conflicts with other test runners.

# Source the shared test runner library
source scripts/test_runner_lib.sh

# Exit on any error (disabled during test loop to allow all tests to run)
# set -e

# Initialize global counters
TEST_FAILURES=0

# C++ Language Test Toggles
RUN_CPP_BASIC_AST_TESTS=true
RUN_CPP_CST_TESTS=false # Disabled - source files don't exist yet

# C++ example test directory toggles
RUN_CPP_BASIC_SYNTAX_TESTS=true
RUN_CPP_COMPLEX_STRUCTURES_TESTS=true
RUN_CPP_MODERN_CPP_TESTS=true
RUN_CPP_TEMPLATES_TESTS=true

# Project root directory (assuming this script is in the root)
PROJECT_ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
CMAKE_PROJECT_BUILD_DIR="${PROJECT_ROOT_DIR}/build-cpp"

# Set parallel jobs for test execution
PARALLEL_JOBS=1

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

# Prepare build directory (clean or not, depending on flag)
prepare_clean_build_dir "$CMAKE_PROJECT_BUILD_DIR" "$CLEAN_BUILD"

# Setup CMake configuration using the shared library
setup_cmake_config "$PROJECT_ROOT_DIR"

# Define all C++ test targets and their display names
CPP_TEST_TARGETS=(
    "cpp_basic_ast_tests:C++ Basic AST Tests"
    "cpp_cst_tests:C++ CST Tests"
)

# Map from target to executable relpath
declare -A CPP_TEST_EXECUTABLES
CPP_TEST_EXECUTABLES["cpp_basic_ast_tests"]="core/tests/cpp_basic_ast_tests"
CPP_TEST_EXECUTABLES["cpp_cst_tests"]="core/tests/cpp_cst_tests"

# Loop over all C++ test targets that have their own executables
for target in "${CPP_TEST_TARGETS[@]}"; do
    IFS=':' read -r test_name test_description <<<"$target"

    # Determine if the test should run based on its toggle
    should_run=false
    case "$test_name" in
    "cpp_basic_ast_tests")
        if [ "$RUN_CPP_BASIC_AST_TESTS" = true ]; then should_run=true; fi
        ;;
    "cpp_cst_tests")
        if [ "$RUN_CPP_CST_TESTS" = true ]; then
            echo "[run_cpp_tests.sh] WARNING: $test_name is enabled but its source files do not exist yet. Skipping."
        fi
        continue
        ;;
    esac

    if [ "$should_run" = false ]; then
        continue
    fi

    build_test_target "$test_name" "$CMAKE_PROJECT_BUILD_DIR"
    build_result=$?
    if [ $build_result -ne 0 ]; then
        echo "[run_cpp_tests.sh] ERROR: Failed to build $test_name"
        ((TEST_FAILURES++))
        continue
    fi

    executable_path="${CMAKE_PROJECT_BUILD_DIR}/${CPP_TEST_EXECUTABLES[$test_name]}"
    if [ ! -f "$executable_path" ]; then
        echo "[run_cpp_tests.sh] ERROR: Executable not found at $executable_path"
        ((TEST_FAILURES++))
        continue
    fi

    run_test_suite "$test_description" "$executable_path"
    if [ $? -ne 0 ]; then ((TEST_FAILURES++)); fi
done

# Gather enabled C++ example test categories
CPP_TEST_CATEGORIES=()
if [ "$RUN_CPP_BASIC_SYNTAX_TESTS" = true ]; then CPP_TEST_CATEGORIES+=("basic_syntax"); fi
if [ "$RUN_CPP_COMPLEX_STRUCTURES_TESTS" = true ]; then CPP_TEST_CATEGORIES+=("complex_structures"); fi
if [ "$RUN_CPP_MODERN_CPP_TESTS" = true ]; then CPP_TEST_CATEGORIES+=("modern_cpp"); fi
if [ "$RUN_CPP_TEMPLATES_TESTS" = true ]; then CPP_TEST_CATEGORIES+=("templates"); fi

# Run per-directory C++ example tests if any are enabled
if [ "${#CPP_TEST_CATEGORIES[@]}" -gt 0 ]; then
    echo "[run_cpp_tests.sh] Building C++ example AST tests executable..."
    build_test_target "cpp_example_ast_tests" "$CMAKE_PROJECT_BUILD_DIR"
    build_result=$?
    if [ $build_result -ne 0 ]; then
        echo "[run_cpp_tests.sh] ERROR: Failed to build cpp_example_ast_tests, skipping example tests."
        ((TEST_FAILURES++))
    else
        process_language_tests cpp CPP_TEST_CATEGORIES "$CPP_EXAMPLE_AST_EXECUTABLE_RELPATH"
    fi
fi

# Let the shared library handle the final test summary and exit code
print_test_summary
