#!/bin/bash

# Clean build directory before running tests
rm -rf build

set -x # Enable debug output

# ================ IMPORTANT NOTE ================
# Test Case Toggles control BOTH building AND running of tests
# When a toggle is set to true, the test will be built AND run
# When a toggle is set to false, the test will NOT be built and NOT run
# ===============================================

# Test Case Toggles
RUN_INIT_PARSER_TESTS=true
# Add more toggles if you create other Criterion test executables

# --- Configuration ---
# Project root directory (assuming this script is in the root)
PROJECT_ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
CORE_DIR="${PROJECT_ROOT_DIR}/core"
TESTS_DIR="${CORE_DIR}/tests"

# Main CMake build directory for the entire project
CMAKE_PROJECT_BUILD_DIR="${PROJECT_ROOT_DIR}/build"

# Relative path from the CMAKE_PROJECT_BUILD_DIR to where the C test executable is located
INIT_PARSER_EXECUTABLE_RELPATH="core/tests/init_parser_tests"
set +x # Disable debug output

# Sample code and expected output paths
CPP_SAMPLE_DIR="${TESTS_DIR}/sample_code/cpp"
PYTHON_SAMPLE_DIR="${TESTS_DIR}/sample_code/python"
CPP_EXPECTED_DIR="${TESTS_DIR}/expected_output/cpp"
PYTHON_EXPECTED_DIR="${TESTS_DIR}/expected_output/python"

# --- Helper Functions ---

# Function to run a single Criterion test executable
# Expects: test_suite_name executable_path
run_criterion_test_executable() {
    local test_suite_name="$1"
    local executable_path="$2"

    echo "--------------------------------------------------"
    echo "Running Test Suite: ${test_suite_name}"

    if [ ! -f "${executable_path}" ]; then
        echo "FAIL: ${test_suite_name}. Executable not found: ${executable_path}"
        echo "Please build the tests first."
        return 1
    fi

    # Criterion executables run all tests within them and handle output.
    # They return 0 if all tests pass, non-zero if any fail.
    # Running from the build directory where the executable is located can help with relative paths in tests.
    local executable_dir=$(dirname "${executable_path}")
    local executable_name=$(basename "${executable_path}")

    (cd "${executable_dir}" && ./${executable_name} --verbose)
    local test_exit_code=$?

    if [ ${test_exit_code} -eq 0 ]; then
        echo "PASS: ${test_suite_name} (All tests passed)"
    else
        echo "FAIL: ${test_suite_name} (One or more tests failed - Exit Code: ${test_exit_code})"
    fi
    echo "--------------------------------------------------"
    return ${test_exit_code}
}

# --- Main Test Execution ---

echo "Starting C Bindings Test Suite..."

# --- Build C Tests ---
echo "Ensuring C tests are built..."

echo "DEBUG: PROJECT_ROOT_DIR is '${PROJECT_ROOT_DIR}'"
echo "DEBUG: CMAKE_PROJECT_BUILD_DIR is '${CMAKE_PROJECT_BUILD_DIR}'"

mkdir -p "${CMAKE_PROJECT_BUILD_DIR}"
if [ $? -ne 0 ]; then
    echo "ERROR: Could not create build directory: ${CMAKE_PROJECT_BUILD_DIR}"
    exit 1
fi

cd "${CMAKE_PROJECT_BUILD_DIR}"
if [ $? -ne 0 ]; then
    echo "ERROR: Could not navigate to build directory: ${CMAKE_PROJECT_BUILD_DIR}"
    exit 1
fi

echo "Configuring project with CMake... (from ${PWD})"
cmake "${CORE_DIR}"
if [ $? -ne 0 ]; then
    echo "ERROR: CMake configuration failed."
    cd "${PROJECT_ROOT_DIR}"
    exit 1
fi

# Build tests based on toggle settings
if [ "${RUN_INIT_PARSER_TESTS}" = true ]; then
    INIT_PARSER_TARGET_NAME="init_parser_tests" # Target name in CMake
    echo "Building C test target: ${INIT_PARSER_TARGET_NAME}..."
    make "${INIT_PARSER_TARGET_NAME}"
    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to build C test target '${INIT_PARSER_TARGET_NAME}'."
        cd "${PROJECT_ROOT_DIR}"
        exit 1
    fi
fi

# Add more build commands here based on toggles

cd "${PROJECT_ROOT_DIR}"
echo "C tests build process finished."

# --- Run C Tests ---
if [ "${RUN_INIT_PARSER_TESTS}" = true ]; then
    INIT_PARSER_EXECUTABLE_FULL_PATH="${CMAKE_PROJECT_BUILD_DIR}/${INIT_PARSER_EXECUTABLE_RELPATH}"
    run_criterion_test_executable "Init Parser Criterion Tests" \
        "${INIT_PARSER_EXECUTABLE_FULL_PATH}"
fi

# Add more test calls here based on toggles

echo "C Bindings Test Suite Finished."

# Note: This script doesn't aggregate overall pass/fail status yet.
# You could add a counter for failed tests and exit with a non-zero status if any test fails.

exit 0
