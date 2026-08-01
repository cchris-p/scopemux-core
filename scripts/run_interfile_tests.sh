#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

# ScopeMux Interfile Relationship Tests Runner Script
# Uses the shared test runner library for standardized test execution
#
# NOTE: This script uses a unique build directory (build-interfile) to allow parallel test execution across languages.
# This prevents race conditions and build directory conflicts with other test runners.
#
# This script is designed to run tests specifically focused on cross-file
# functionality including symbol resolution, reference tracking, and
# project-wide parsing capabilities.

# Source the shared test runner library
source "${SCRIPT_DIR}/test_runner_lib.sh"

setup_runner_logging "$0" "$PROJECT_ROOT_DIR"

# Don't exit immediately on errors since we want to run all enabled tests
# and report comprehensive results at the end
set +e

# Initialize global counters
TEST_FAILURES=0
TOTAL_TESTS_RUN=0

# 06-30-2025 - tests marked as false are passing, test cases are still stubs

# -------- Interfile Test Toggles --------
# Enable/disable specific interfile test categories
RUN_REFERENCE_RESOLVER_TESTS=false
RUN_SYMBOL_TABLE_TESTS=true
RUN_PROJECT_CONTEXT_TESTS=true
RUN_RESOLVER_CORE_TESTS=true
RUN_RESOLVER_REGISTRATION_TESTS=true
RUN_RESOLVER_RESOLUTION_TESTS=true
RUN_LANGUAGE_RESOLVER_TESTS=true

CORE_DIR="${PROJECT_ROOT_DIR}/core"
TESTS_DIR="${CORE_DIR}/tests"

initialize_runner_build_dir "$PROJECT_ROOT_DIR" "build-interfile"
CMAKE_PROJECT_BUILD_DIR="$CMAKE_BUILD_DIR"

# Relative paths from the CMAKE_PROJECT_BUILD_DIR to test executables
REFERENCE_RESOLVER_EXECUTABLE_RELPATH="core/tests/reference_resolver_tests"
SYMBOL_TABLE_EXECUTABLE_RELPATH="core/tests/symbol_table_tests"
PROJECT_CONTEXT_EXECUTABLE_RELPATH="core/tests/project_context_tests"
RESOLVER_CORE_EXECUTABLE_RELPATH="core/tests/resolver_core_tests"
RESOLVER_REGISTRATION_EXECUTABLE_RELPATH="core/tests/resolver_registration_tests"
RESOLVER_RESOLUTION_EXECUTABLE_RELPATH="core/tests/resolver_resolution_tests"
LANGUAGE_RESOLVER_EXECUTABLE_RELPATH="core/tests/language_resolver_tests"

# Set parallel jobs for test execution
PARALLEL_JOBS=1

# Command-line flag parsing for advanced options
CLEAN_BUILD=true
DEBUG_OUTPUT=true

# Process command line arguments
for arg in "$@"; do
    case $arg in
    --no-clean)
        CLEAN_BUILD=false
        echo "[run_interfile_tests.sh] Skipping clean build"
        ;;
    --debug)
        DEBUG_MODE=true
        echo "[run_interfile_tests.sh] Running in debug mode"
        ;;
    --help)
        echo "Usage: ./run_interfile_tests.sh [options]"
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

# Run interfile resolution tests
echo "[run_interfile_tests.sh] Running interfile resolution test suite"

# Build and run Reference Resolver Tests (main module)
if [ "${RUN_REFERENCE_RESOLVER_TESTS}" = true ]; then
    build_and_run_test_target "run_interfile_tests.sh" "$CMAKE_PROJECT_BUILD_DIR" "reference_resolver_tests" "Reference Resolver Tests" "$REFERENCE_RESOLVER_EXECUTABLE_RELPATH"
    TOTAL_TESTS_RUN=$((TOTAL_TESTS_RUN + 1))
fi

# Build and run Symbol Table Tests
if [ "${RUN_SYMBOL_TABLE_TESTS}" = true ]; then
    build_and_run_test_target "run_interfile_tests.sh" "$CMAKE_PROJECT_BUILD_DIR" "symbol_table_tests" "Symbol Table Tests" "$SYMBOL_TABLE_EXECUTABLE_RELPATH"
    TOTAL_TESTS_RUN=$((TOTAL_TESTS_RUN + 1))
fi

# Build and run Project Context Tests
if [ "${RUN_PROJECT_CONTEXT_TESTS}" = true ]; then
    build_and_run_test_target "run_interfile_tests.sh" "$CMAKE_PROJECT_BUILD_DIR" "project_context_tests" "Project Context Tests" "$PROJECT_CONTEXT_EXECUTABLE_RELPATH"
    TOTAL_TESTS_RUN=$((TOTAL_TESTS_RUN + 1))
fi

# Build and run Resolver Core Tests (modular component)
if [ "${RUN_RESOLVER_CORE_TESTS}" = true ]; then
    build_and_run_test_target "run_interfile_tests.sh" "$CMAKE_PROJECT_BUILD_DIR" "resolver_core_tests" "Resolver Core Tests" "$RESOLVER_CORE_EXECUTABLE_RELPATH"
    TOTAL_TESTS_RUN=$((TOTAL_TESTS_RUN + 1))
fi

# Build and run Resolver Registration Tests
if [ "${RUN_RESOLVER_REGISTRATION_TESTS}" = true ]; then
    echo "[run_interfile_tests.sh] Starting Resolver Registration Tests"
    cd "${CMAKE_PROJECT_BUILD_DIR}" || exit 1

    # Explicitly build the resolver registration tests
    echo "[run_interfile_tests.sh] Building resolver_registration_tests target"
    make resolver_registration_tests -j${PARALLEL_JOBS}
    build_status=$?

    if [ $build_status -ne 0 ]; then
        echo "[run_interfile_tests.sh] ERROR: Failed to build resolver_registration_tests"
        TEST_FAILURES=$((TEST_FAILURES + 1))
    else
        echo "[run_interfile_tests.sh] OK: Successfully built resolver_registration_tests"

        # Find the built executable
        TEST_EXECUTABLE=$(find "${CMAKE_PROJECT_BUILD_DIR}/core/tests" -name "resolver_registration_tests" -type f -executable)

        if [ -z "$TEST_EXECUTABLE" ]; then
            echo "[run_interfile_tests.sh] ERROR: Could not find resolver_registration_tests executable"
            TEST_FAILURES=$((TEST_FAILURES + 1))
        else
            echo "[run_interfile_tests.sh] Found test executable: $TEST_EXECUTABLE"

            # Run the test directly
            echo "[run_interfile_tests.sh] Running resolver registration tests directly:"
            "$TEST_EXECUTABLE"
            test_status=$?

            TOTAL_TESTS_RUN=$((TOTAL_TESTS_RUN + 1))
            if [ $test_status -ne 0 ]; then
                echo "[run_interfile_tests.sh] ERROR: Resolver registration tests failed with exit code: $test_status"
                TEST_FAILURES=$((TEST_FAILURES + 1))
            else
                echo "[run_interfile_tests.sh] OK: Resolver registration tests passed"
            fi
        fi
    fi
fi

# Build and run Resolver Resolution Tests
if [ "${RUN_RESOLVER_RESOLUTION_TESTS}" = true ]; then
    echo "[run_interfile_tests.sh] Starting Resolver Resolution Tests"
    cd "${CMAKE_PROJECT_BUILD_DIR}" || exit 1

    # Print current directory for debugging
    echo "[run_interfile_tests.sh] Current directory: $(pwd)"

    # First verify the test target exists in CMake
    echo "[run_interfile_tests.sh] Available resolver targets:"
    cmake --build . --target help | grep resolver

    # Explicitly build the resolver tests
    echo "[run_interfile_tests.sh] Building resolver_resolution_tests target"
    make resolver_resolution_tests -j${PARALLEL_JOBS}
    build_status=$?

    if [ $build_status -ne 0 ]; then
        echo "[run_interfile_tests.sh] ERROR: Failed to build resolver_resolution_tests"
        TEST_FAILURES=$((TEST_FAILURES + 1))
    else
        echo "[run_interfile_tests.sh] OK: Successfully built resolver_resolution_tests"

        # Find the built executable
        TEST_EXECUTABLE=$(find "${CMAKE_PROJECT_BUILD_DIR}/core/tests" -name "resolver_resolution_tests" -type f -executable)

        if [ -z "$TEST_EXECUTABLE" ]; then
            echo "[run_interfile_tests.sh] ERROR: Could not find resolver_resolution_tests executable"
            TEST_FAILURES=$((TEST_FAILURES + 1))
        else
            echo "[run_interfile_tests.sh] Found test executable: $TEST_EXECUTABLE"

            # Run the test directly to ensure it executes
            echo "[run_interfile_tests.sh] Running resolver tests directly:"
            "$TEST_EXECUTABLE"
            test_status=$?

            TOTAL_TESTS_RUN=$((TOTAL_TESTS_RUN + 1))
            if [ $test_status -ne 0 ]; then
                echo "[run_interfile_tests.sh] ERROR: Resolver resolution tests failed with exit code: $test_status"
                TEST_FAILURES=$((TEST_FAILURES + 1))
            else
                echo "[run_interfile_tests.sh] OK: Resolver resolution tests passed"
            fi
        fi
    fi
fi

# Build and run Language Resolver Tests
if [ "${RUN_LANGUAGE_RESOLVER_TESTS}" = true ]; then
    build_and_run_test_target "run_interfile_tests.sh" "$CMAKE_PROJECT_BUILD_DIR" "language_resolver_tests" "Language Resolver Tests" "$LANGUAGE_RESOLVER_EXECUTABLE_RELPATH"
    TOTAL_TESTS_RUN=$((TOTAL_TESTS_RUN + 1))
fi

# Group test suites by category for easier management
INTERFILE_TEST_CATEGORIES=()

if [ "$RUN_REFERENCE_RESOLVER_TESTS" = true ]; then
    INTERFILE_TEST_CATEGORIES+=("reference_resolver")
fi

if [ "$RUN_SYMBOL_TABLE_TESTS" = true ]; then
    INTERFILE_TEST_CATEGORIES+=("symbol_table")
fi

if [ "$RUN_PROJECT_CONTEXT_TESTS" = true ]; then
    INTERFILE_TEST_CATEGORIES+=("project_context")
fi

# Only process grouped categories if any are enabled
if [ "${#INTERFILE_TEST_CATEGORIES[@]}" -gt 0 ]; then
    # Optional: Add integrated tests that test across multiple modules
    echo "[run_interfile_tests.sh] Running integrated cross-module tests"
    # Example usage of process_language_tests if needed for integrated tests
    # process_language_tests interfile INTERFILE_TEST_CATEGORIES "${CMAKE_PROJECT_BUILD_DIR}/core/tests/integrated_tests"
fi

# Return to project root before printing summary
cd "${PROJECT_ROOT_DIR}" || exit 1

# Main script cleanup and report generation
echo ""
echo "===== INTERFILE TEST SUMMARY ====="
echo "Total test suites run: ${TOTAL_TESTS_RUN}"

if [ ${TEST_FAILURES} -eq 0 ]; then
    echo "ALL TESTS PASSED"
    exit 0
else
    echo "${TEST_FAILURES} TEST SUITES FAILED"
    echo "Please check the output above for detailed error messages"
    exit 1
fi
