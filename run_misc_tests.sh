#!/bin/bash

# Clean build directory before running tests
rm -rf build

# set -x # Enable debug output

# ================ IMPORTANT NOTE ================
# Test Case Toggles control BOTH building AND running of tests
# When a toggle is set to true, the test will be built AND run
# When a toggle is set to false, the test will NOT be built and NOT run
# ===============================================

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
CMAKE_PROJECT_BUILD_DIR="${PROJECT_ROOT_DIR}/build"

# Relative paths from the CMAKE_PROJECT_BUILD_DIR to where the misc test executables are located
INIT_PARSER_EXECUTABLE_RELPATH="core/tests/init_parser_tests"
EDGE_CASE_EXECUTABLE_RELPATH="core/tests/edge_case_tests"
# Add additional misc test executable paths here as needed


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

    # Set PROJECT_ROOT_DIR to the project root to help tests find expected JSON files
    local project_root="$(cd "$(dirname "$0")" && pwd)"
    export PROJECT_ROOT_DIR="${project_root}"
    
    pushd "${executable_dir}" >/dev/null
    echo "Running: ./${executable_name} with PROJECT_ROOT_DIR=${PROJECT_ROOT_DIR}"
    "./${executable_name}"
    local test_exit_code=$?

    # NOTE: All tests MUST have matching expected JSON files to pass validation.
    # No shortcuts or exceptions allowed - proper validation ensures code correctness.

    popd >/dev/null
    
    if [ ${test_exit_code} -eq 0 ]; then
        echo "PASS: ${test_suite_name} (All tests passed)"
    else
        echo "FAIL: ${test_suite_name} (One or more tests failed - Exit Code: ${test_exit_code})"
    fi
    echo "--------------------------------------------------"
    return ${test_exit_code}
}

# --- Main Test Execution ---

echo "Starting Miscellaneous Test Suite..."

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
    exit 1
fi

# Build misc tests based on toggle settings
build_test() {
    local target_name="$1"
    local display_name="$2"

    echo "Building test target: ${display_name}..."
    make "${target_name}"
    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to build test target '${display_name}'."
        cd "${PROJECT_ROOT_DIR}"
        exit 1
    fi
    echo "Successfully built: ${display_name}"
}

# Only build misc/common tests
if [ "${RUN_INIT_PARSER_TESTS}" = true ]; then
    build_test "init_parser_tests" "Init Parser Tests"
fi

if [ "${RUN_EDGE_CASE_TESTS}" = true ]; then
    build_test "edge_case_tests" "Edge Case Tests"
fi

cd "${PROJECT_ROOT_DIR}"
echo "Miscellaneous tests build process finished."

# --- Run Tests ---
echo "Running enabled misc tests..."

if [ "${RUN_INIT_PARSER_TESTS}" = true ]; then
    run_criterion_test_executable "Init Parser Tests" \
        "${CMAKE_PROJECT_BUILD_DIR}/${INIT_PARSER_EXECUTABLE_RELPATH}"
fi

if [ "${RUN_EDGE_CASE_TESTS}" = true ]; then
    run_criterion_test_executable "Edge Case Tests" \
        "${CMAKE_PROJECT_BUILD_DIR}/${EDGE_CASE_EXECUTABLE_RELPATH}"
fi

echo "Note: This script only runs tests not covered by dedicated language scripts."

exit 0
