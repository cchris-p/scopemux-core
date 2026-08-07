#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

# ScopeMux C++ Tests Runner Script
# Uses the shared test runner library for standardized test execution
#
# NOTE: This script uses a unique build directory (build-cpp) to allow parallel test execution across languages.
# This prevents race conditions and build directory conflicts with other test runners.

# Source the shared test runner library
source "${SCRIPT_DIR}/test_runner_lib.sh"

setup_runner_logging "$0" "$PROJECT_ROOT_DIR"
initialize_runner_build_dir "$PROJECT_ROOT_DIR" "build-cpp"

# Exit on any error (disabled during test loop to allow all tests to run)
# set -e

# Initialize global counters
TEST_FAILURES=0

# C++ Language Test Toggles
RUN_CPP_BASIC_AST_TESTS=true
RUN_CPP_EXAMPLE_AST_TESTS=true
RUN_CPP_CST_TESTS=false # Disabled - source files don't exist yet

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
        echo "Usage: $0 [options]"
        echo "Options:"
        echo "  --no-clean      : Skip cleaning build directory"
        echo "  --help          : Show this help message"
        exit 0
        ;;
    esac
done

prepare_and_configure_build "$PROJECT_ROOT_DIR" "$CMAKE_BUILD_DIR" "$CLEAN_BUILD"

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

    build_and_run_test_target "run_cpp_tests.sh" "$CMAKE_BUILD_DIR" "$test_name" "$test_description" "${CPP_TEST_EXECUTABLES[$test_name]}"
done

# Run manifest-defined C++ example tests if enabled
if [ "${RUN_CPP_EXAMPLE_AST_TESTS}" = true ]; then
    load_example_categories cpp CPP_TEST_CATEGORIES || exit 1
    echo "[run_cpp_tests.sh] Building C++ example AST tests executable..."
    build_test_target "cpp_example_ast_tests" "$CMAKE_BUILD_DIR"
    build_result=$?
    if [ $build_result -ne 0 ]; then
        echo "[run_cpp_tests.sh] ERROR: Failed to build cpp_example_ast_tests, skipping example tests."
        ((TEST_FAILURES++))
    else
        process_language_tests cpp CPP_TEST_CATEGORIES "$CMAKE_BUILD_DIR/$CPP_EXAMPLE_AST_EXECUTABLE_RELPATH"
    fi
fi

# Let the shared library handle the final test summary and exit code
print_test_summary
