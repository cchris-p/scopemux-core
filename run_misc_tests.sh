#!/bin/bash

# All output from this script (stdout and stderr) is written to a file named after the script and the current datetime.
# Filename format: run_misc_tests-YYYYMMDD-HHMMSS.txt (UTC)
# This is useful for preserving logs for each test run.
SCRIPT_BASENAME="$(basename "$0" .sh)"
RUN_DATETIME="$(date -u +"%Y%m%d-%H%M%S")"
OUTPUT_FILE="${SCRIPT_BASENAME}-${RUN_DATETIME}.txt"
# Redirect all output to the log file (both stdout and stderr)
exec > >(tee "$OUTPUT_FILE") 2>&1

# ScopeMux Miscellaneous Tests Runner Script
# Uses the shared test runner library for standardized test execution
#
# NOTE: This script uses a unique build directory (build-misc) to allow parallel test execution across languages.
# This prevents race conditions and build directory conflicts with other test runners.

# Source the shared test runner library
source scripts/test_runner_lib.sh

# Exit on any error
set -e

# Initialize global counters
TEST_FAILURES=0

# -------- Miscellaneous Test Toggles --------
# Only enable tests that are not covered by language-specific scripts
RUN_INIT_PARSER_TESTS=true
RUN_EDGE_CASE_TESTS=true
# Add additional misc test toggles here as needed

# Project root directory (assuming this script is in the root)
PROJECT_ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
CORE_DIR="${PROJECT_ROOT_DIR}/core"
TESTS_DIR="${CORE_DIR}/tests"

# Main CMake build directory for the entire project
CMAKE_PROJECT_BUILD_DIR="${PROJECT_ROOT_DIR}/build-misc"

# Relative paths from the CMAKE_PROJECT_BUILD_DIR to where the misc test executables are located
INIT_PARSER_EXECUTABLE_RELPATH="core/tests/init_parser_tests"
EDGE_CASE_EXECUTABLE_RELPATH="core/tests/edge_case_tests"
# Add additional misc test executable paths here as needed

# Set parallel jobs for test execution
PARALLEL_JOBS=1

# Command-line flag parsing for advanced options
CLEAN_BUILD=true

# Process command line arguments
for arg in "$@"; do
    case $arg in
    --no-clean)
        CLEAN_BUILD=false
        echo "[run_misc_tests.sh] Skipping clean build"
        ;;
    --debug)
        DEBUG_MODE=true
        echo "[run_misc_tests.sh] Running in debug mode"
        ;;
    --help)
        echo "Usage: ./run_misc_tests.sh [options]"
        echo "Options:"
        echo "  --no-clean      : Skip cleaning build directory"
        echo "  --debug         : Run in debug mode with verbose output"
        echo "  --help          : Show this help message"
        exit 0
        ;;
    esac
done

# Prepare build directory (clean or not, depending on flag)
prepare_clean_build_dir "${CMAKE_PROJECT_BUILD_DIR}" "${CLEAN_BUILD}"

# Setup CMake configuration properly
cd "${CMAKE_PROJECT_BUILD_DIR}" || exit 1

# Execute CMake with explicit source and build directory specification
echo "[run_misc_tests.sh] Configuring CMake in build directory: ${CMAKE_PROJECT_BUILD_DIR}"
cmake -S "${PROJECT_ROOT_DIR}" -B "${CMAKE_PROJECT_BUILD_DIR}" -G "Unix Makefiles" >"${CMAKE_PROJECT_BUILD_DIR}/cmake_config.log" 2>&1

if [ $? -ne 0 ]; then
    echo "❌ ERROR: CMake configuration failed. See log for details:"
    cat "${CMAKE_PROJECT_BUILD_DIR}/cmake_config.log"
    exit 1
fi

# Verify that Makefiles were created
if [ ! -f "${CMAKE_PROJECT_BUILD_DIR}/Makefile" ]; then
    echo "❌ ERROR: CMake did not generate Makefiles in the build directory."
    echo "Contents of build directory:"
    ls -la "${CMAKE_PROJECT_BUILD_DIR}"
    exit 1
fi

# Run miscellaneous tests
echo "[run_misc_tests.sh] Running miscellaneous test suite"

# Build and run Init Parser Tests
if [ "${RUN_INIT_PARSER_TESTS}" = true ]; then
    # Make sure we're in the build directory before running make
    cd "${CMAKE_PROJECT_BUILD_DIR}" || exit 1

    # Try to build directly for better error visibility
    echo "[run_misc_tests.sh] Building init_parser_tests directly (for debugging)..."
    make "init_parser_tests" VERBOSE=1
    build_result=$?

    if [ $build_result -eq 0 ]; then
        echo "[run_misc_tests.sh] Successfully built init_parser_tests, running tests..."
        run_test_suite "Init Parser Tests" "${CMAKE_PROJECT_BUILD_DIR}/${INIT_PARSER_EXECUTABLE_RELPATH}"
        if [ $? -ne 0 ]; then TEST_FAILURES=$((TEST_FAILURES + 1)); fi
    else
        echo "❌ ERROR: Failed to build init_parser_tests"
        TEST_FAILURES=$((TEST_FAILURES + 1))
    fi
fi

# Build and run Edge Case Tests
if [ "${RUN_EDGE_CASE_TESTS}" = true ]; then
    # Make sure we're in the build directory before running make
    cd "${CMAKE_PROJECT_BUILD_DIR}" || exit 1
    build_test_target "edge_case_tests" "Edge Case Tests"
    run_test_suite "Edge Case Tests" "${CMAKE_PROJECT_BUILD_DIR}/${EDGE_CASE_EXECUTABLE_RELPATH}"
    if [ $? -ne 0 ]; then TEST_FAILURES=$((TEST_FAILURES + 1)); fi
fi

# Example for future: gather enabled misc categories and call process_language_tests here
# MISC_TEST_CATEGORIES=()
# if [ "$RUN_MISC_CATEGORY1_TESTS" = true ]; then
#     MISC_TEST_CATEGORIES+=("category1")
# fi
# if [ "${#MISC_TEST_CATEGORIES[@]}" -gt 0 ]; then
#     process_language_tests misc MISC_TEST_CATEGORIES "<misc_example_executable_path>"
# fi

# Return to project root before printing summary
cd "${PROJECT_ROOT_DIR}" || exit 1

# Let the shared library handle the final test summary and exit code
print_test_summary
