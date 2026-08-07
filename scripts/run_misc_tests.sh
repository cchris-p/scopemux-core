#!/bin/bash

# -----------------------------------------------------------------------------
# ScopeMux Miscellaneous Tests Runner Script
#
# Purpose:
#   Runs core and infrastructure tests that are not covered by language-specific
#   test scripts (e.g., run_c_tests.sh, run_cpp_tests.sh, etc.).
#   This includes:
#     - Parser initialization and teardown (init_parser_tests)
#     - Edge-case and cross-language parsing scenarios (edge_case_tests)
#     - Any future categories of tests that are not language-specific
#
# Build Isolation:
#   Uses a dedicated build directory (build-misc) to avoid conflicts and enable
#   parallel test execution with other test runners.
#
# Logging:
#   All output (stdout and stderr) is written to a timestamped log file for
#   reproducibility and debugging.
#
# Test Runner:
#   Relies on the shared test runner library (scripts/test_runner_lib.sh) for
#   standardized test execution, summary, and cleanup.
#
# Test Toggles:
#   Enable or disable specific categories of miscellaneous tests using the
#   RUN_*_TESTS variables below.
#
# Caveats / TODO:
#   - Add new miscellaneous test categories as needed.
#   - Ensure that any new test categories are not duplicated in language-specific
#     scripts.
# -----------------------------------------------------------------------------

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

# ScopeMux Miscellaneous Tests Runner Script
# Uses the shared test runner library for standardized test execution
#
# NOTE: This script uses a unique build directory (build-misc) to allow parallel test execution across languages.
# This prevents race conditions and build directory conflicts with other test runners.

# Source the shared test runner library
source "${SCRIPT_DIR}/test_runner_lib.sh"

setup_runner_logging "$0" "$PROJECT_ROOT_DIR"

# Exit on any error
set -e

# Initialize global counters
TEST_FAILURES=0

# -------- Miscellaneous Test Toggles --------
# Only enable tests that are not covered by language-specific scripts
RUN_INIT_PARSER_TESTS=true
RUN_EDGE_CASE_TESTS=true
# Add additional misc test toggles here as needed

CORE_DIR="${PROJECT_ROOT_DIR}/core"
TESTS_DIR="${CORE_DIR}/tests"

initialize_runner_build_dir "$PROJECT_ROOT_DIR" "build-misc"
CMAKE_PROJECT_BUILD_DIR="$CMAKE_BUILD_DIR"

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
prepare_and_configure_build "$PROJECT_ROOT_DIR" "$CMAKE_PROJECT_BUILD_DIR" "$CLEAN_BUILD"

# Run miscellaneous tests
echo "[run_misc_tests.sh] Running miscellaneous test suite"

# Build and run Init Parser Tests
if [ "${RUN_INIT_PARSER_TESTS}" = true ]; then
    build_and_run_test_target "run_misc_tests.sh" "$CMAKE_PROJECT_BUILD_DIR" "init_parser_tests" "Init Parser Tests" "$INIT_PARSER_EXECUTABLE_RELPATH" || true
fi

# Build and run Edge Case Tests
if [ "${RUN_EDGE_CASE_TESTS}" = true ]; then
    build_and_run_test_target "run_misc_tests.sh" "$CMAKE_PROJECT_BUILD_DIR" "edge_case_tests" "Edge Case Tests" "$EDGE_CASE_EXECUTABLE_RELPATH" || true
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
