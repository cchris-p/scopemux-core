#!/bin/bash

# All output from this script (stdout and stderr) is written to a file named after the script.
# Filename format: run_interfile_tests.txt
# This is useful for preserving logs for each test run.
SCRIPT_BASENAME="$(basename "$0" .sh)"
OUTPUT_FILE="run_interfile_tests.txt"
# Redirect all output to the log file (both stdout and stderr)
exec > >(tee "$OUTPUT_FILE") 2>&1

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
source scripts/test_runner_lib.sh

# Don't exit immediately on errors since we want to run all enabled tests
# and report comprehensive results at the end
set +e

# Initialize global counters
TEST_FAILURES=0
TOTAL_TESTS_RUN=0

# -------- Interfile Test Toggles --------
# Enable/disable specific interfile test categories
RUN_REFERENCE_RESOLVER_TESTS=true
RUN_SYMBOL_TABLE_TESTS=true
RUN_PROJECT_CONTEXT_TESTS=true
RUN_RESOLVER_CORE_TESTS=true
RUN_RESOLVER_REGISTRATION_TESTS=true
RUN_RESOLVER_RESOLUTION_TESTS=true
RUN_LANGUAGE_RESOLVER_TESTS=true


# Project root directory (assuming this script is in the root)
PROJECT_ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
CORE_DIR="${PROJECT_ROOT_DIR}/core"
TESTS_DIR="${CORE_DIR}/tests"

# Main CMake build directory for the entire project
CMAKE_PROJECT_BUILD_DIR="${PROJECT_ROOT_DIR}/build-interfile"

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
prepare_clean_build_dir "${CMAKE_PROJECT_BUILD_DIR}" "${CLEAN_BUILD}"

# Setup CMake configuration properly
cd "${CMAKE_PROJECT_BUILD_DIR}" || exit 1

# Execute CMake with explicit source and build directory specification
echo "[run_interfile_tests.sh] Configuring CMake in build directory: ${CMAKE_PROJECT_BUILD_DIR}"
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

# Run interfile resolution tests
echo "[run_interfile_tests.sh] Running interfile resolution test suite"

# Build and run Reference Resolver Tests (main module)
if [ "${RUN_REFERENCE_RESOLVER_TESTS}" = true ]; then
    cd "${CMAKE_PROJECT_BUILD_DIR}" || exit 1
    build_test_target "reference_resolver_tests" "Reference Resolver Tests"
    run_test_suite "Reference Resolver Tests" "${CMAKE_PROJECT_BUILD_DIR}/${REFERENCE_RESOLVER_EXECUTABLE_RELPATH}"
    if [ $? -ne 0 ]; then TEST_FAILURES=$((TEST_FAILURES + 1)); fi
fi

# Build and run Symbol Table Tests
if [ "${RUN_SYMBOL_TABLE_TESTS}" = true ]; then
    cd "${CMAKE_PROJECT_BUILD_DIR}" || exit 1
    build_test_target "symbol_table_tests" "Symbol Table Tests"
    run_test_suite "Symbol Table Tests" "${CMAKE_PROJECT_BUILD_DIR}/${SYMBOL_TABLE_EXECUTABLE_RELPATH}"
    if [ $? -ne 0 ]; then TEST_FAILURES=$((TEST_FAILURES + 1)); fi
fi

# Build and run Project Context Tests
if [ "${RUN_PROJECT_CONTEXT_TESTS}" = true ]; then
    cd "${CMAKE_PROJECT_BUILD_DIR}" || exit 1
    build_test_target "project_context_tests" "Project Context Tests"
    run_test_suite "Project Context Tests" "${CMAKE_PROJECT_BUILD_DIR}/${PROJECT_CONTEXT_EXECUTABLE_RELPATH}"
    if [ $? -ne 0 ]; then TEST_FAILURES=$((TEST_FAILURES + 1)); fi
fi

# Build and run Resolver Core Tests (modular component)
if [ "${RUN_RESOLVER_CORE_TESTS}" = true ]; then
    cd "${CMAKE_PROJECT_BUILD_DIR}" || exit 1
    build_test_target "resolver_core_tests" "Resolver Core Tests"
    run_test_suite "Resolver Core Tests" "${CMAKE_PROJECT_BUILD_DIR}/${RESOLVER_CORE_EXECUTABLE_RELPATH}"
    if [ $? -ne 0 ]; then TEST_FAILURES=$((TEST_FAILURES + 1)); fi
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
        echo "[run_interfile_tests.sh] ❌ Failed to build resolver_registration_tests"
        TEST_FAILURES=$((TEST_FAILURES + 1))
    else
        echo "[run_interfile_tests.sh] ✅ Successfully built resolver_registration_tests"

        # Find the built executable
        TEST_EXECUTABLE=$(find "${CMAKE_PROJECT_BUILD_DIR}/core/tests" -name "resolver_registration_tests" -type f -executable)

        if [ -z "$TEST_EXECUTABLE" ]; then
            echo "[run_interfile_tests.sh] ❌ Could not find resolver_registration_tests executable"
            TEST_FAILURES=$((TEST_FAILURES + 1))
        else
            echo "[run_interfile_tests.sh] Found test executable: $TEST_EXECUTABLE"

            # Run the test directly
            echo "[run_interfile_tests.sh] Running resolver registration tests directly:"
            "$TEST_EXECUTABLE"
            test_status=$?

            TOTAL_TESTS_RUN=$((TOTAL_TESTS_RUN + 1))
            if [ $test_status -ne 0 ]; then
                echo "[run_interfile_tests.sh] ❌ Resolver registration tests failed with exit code: $test_status"
                TEST_FAILURES=$((TEST_FAILURES + 1))
            else
                echo "[run_interfile_tests.sh] ✅ Resolver registration tests passed"
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
        echo "[run_interfile_tests.sh] ❌ Failed to build resolver_resolution_tests"
        TEST_FAILURES=$((TEST_FAILURES + 1))
    else
        echo "[run_interfile_tests.sh] ✅ Successfully built resolver_resolution_tests"

        # Find the built executable
        TEST_EXECUTABLE=$(find "${CMAKE_PROJECT_BUILD_DIR}/core/tests" -name "resolver_resolution_tests" -type f -executable)

        if [ -z "$TEST_EXECUTABLE" ]; then
            echo "[run_interfile_tests.sh] ❌ Could not find resolver_resolution_tests executable"
            TEST_FAILURES=$((TEST_FAILURES + 1))
        else
            echo "[run_interfile_tests.sh] Found test executable: $TEST_EXECUTABLE"

            # Run the test directly to ensure it executes
            echo "[run_interfile_tests.sh] Running resolver tests directly:"
            "$TEST_EXECUTABLE"
            test_status=$?

            TOTAL_TESTS_RUN=$((TOTAL_TESTS_RUN + 1))
            if [ $test_status -ne 0 ]; then
                echo "[run_interfile_tests.sh] ❌ Resolver resolution tests failed with exit code: $test_status"
                TEST_FAILURES=$((TEST_FAILURES + 1))
            else
                echo "[run_interfile_tests.sh] ✅ Resolver resolution tests passed"
            fi
        fi
    fi
fi

# Build and run Language Resolver Tests
if [ "${RUN_LANGUAGE_RESOLVER_TESTS}" = true ]; then
    cd "${CMAKE_PROJECT_BUILD_DIR}" || exit 1
    build_test_target "language_resolver_tests" "Language Resolver Tests"
    run_test_suite "Language Resolver Tests" "${CMAKE_PROJECT_BUILD_DIR}/${LANGUAGE_RESOLVER_EXECUTABLE_RELPATH}"
    if [ $? -ne 0 ]; then TEST_FAILURES=$((TEST_FAILURES + 1)); fi
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
    echo "✅✅ ALL TESTS PASSED ✅✅"
    exit 0
else
    echo "❌❌ ${TEST_FAILURES} TEST SUITES FAILED ❌❌"
    echo "Please check the output above for detailed error messages"
    exit 1
fi
