#!/bin/bash
set -x # Enable debug output

# Test Case Toggles
RUN_C_PARSER_CRITERION_TESTS=true
# Add more toggles if you create other Criterion test executables

# --- Configuration ---
# Project root directory (assuming this script is in the root)
PROJECT_ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
BINDINGS_DIR="${PROJECT_ROOT_DIR}/bindings"
TESTS_DIR="${BINDINGS_DIR}/tests"

# Main CMake build directory for the entire project
CMAKE_PROJECT_BUILD_DIR="${PROJECT_ROOT_DIR}/build"

# Relative path from the CMAKE_PROJECT_BUILD_DIR to where the C test executable is located
C_TEST_EXECUTABLE_RELPATH="bindings/tests/scopemux_c_parser_tests"
set +x # Disable debug output

# Sample code and expected output paths
CPP_SAMPLE_DIR="${TESTS_DIR}/sample_code/cpp"
PYTHON_SAMPLE_DIR="${TESTS_DIR}/sample_code/python"
CPP_EXPECTED_DIR="${TESTS_DIR}/expected_output/cpp"
PYTHON_EXPECTED_DIR="${TESTS_DIR}/expected_output/python"

# --- Helper Functions ---

# (ensure_build_dir function removed as build steps are now included below)

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
cmake "${PROJECT_ROOT_DIR}"
if [ $? -ne 0 ]; then
    echo "ERROR: CMake configuration failed."
    cd "${PROJECT_ROOT_DIR}"
    exit 1
fi

C_TEST_TARGET_NAME="scopemux_c_parser_tests" # Make sure this matches the target name in CMake
echo "Building C test target: ${C_TEST_TARGET_NAME}..."
make "${C_TEST_TARGET_NAME}"
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to build C test target '${C_TEST_TARGET_NAME}'."
    cd "${PROJECT_ROOT_DIR}"
    exit 1
fi

cd "${PROJECT_ROOT_DIR}"
echo "C tests build process finished."

# --- Run C Tests ---
if [ "${RUN_C_PARSER_CRITERION_TESTS}" = true ]; then
    C_TEST_EXECUTABLE_FULL_PATH="${CMAKE_PROJECT_BUILD_DIR}/${C_TEST_EXECUTABLE_RELPATH}"
    run_criterion_test_executable "C Parser Criterion Tests" \
        "${C_TEST_EXECUTABLE_FULL_PATH}"
fi

# Add more test calls here based on toggles

echo "C Bindings Test Suite Finished."

# Note: This script doesn't aggregate overall pass/fail status yet.
# You could add a counter for failed tests and exit with a non-zero status if any test fails.

exit 0
