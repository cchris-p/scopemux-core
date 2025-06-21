#!/bin/bash

# Clean build directory before running tests
rm -rf build
mkdir -p build

# JavaScript Language Test Toggles
RUN_JS_BASIC_AST_TESTS=false
RUN_JS_EXAMPLE_AST_TESTS=false
RUN_JS_CST_TESTS=false

# Project root directory (assuming this script is in the root)
PROJECT_ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
CMAKE_PROJECT_BUILD_DIR="${PROJECT_ROOT_DIR}/build"

# JavaScript language test executables
JS_BASIC_AST_EXECUTABLE_RELPATH="core/tests/js_basic_ast_tests"
JS_EXAMPLE_AST_EXECUTABLE_RELPATH="core/tests/js_example_ast_tests"
JS_CST_EXECUTABLE_RELPATH="core/tests/js_cst_tests"

# Function to build a test
build_test() {
    local target_name="$1"
    local display_name="$2"

    echo "Building test target: ${display_name}..."
    make "${target_name}"
    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to build test target '${display_name}'."
        exit 1
    fi
    echo "Successfully built: ${display_name}"
}

# Function to run a test
run_test() {
    local test_suite_name="$1"
    local executable_path="$2"

    echo "Running Test Suite: ${test_suite_name}"

    if [ ! -f "${executable_path}" ]; then
        echo "FAIL: ${test_suite_name}. Executable not found: ${executable_path}"
        return 1
    fi

    pushd "$(dirname "${executable_path}")" >/dev/null
    "./$(basename "${executable_path}")" 2>&1 | tee "$tmp_output"
    local test_exit_code=${PIPESTATUS[0]}
    popd >/dev/null

    # Remove misleading Criterion summary line from output (both stdout and stderr)
    grep -v "FAIL: .* (One or more tests failed)" "$tmp_output"

    # Check for the summary line indicating all tests passed
    if grep -q "Failing: 0 | Crashing: 0" "$tmp_output"; then
        echo "PASS: ${test_suite_name} (All tests passed)"
        rm "$tmp_output"
        return 0
    else
        echo "FAIL: ${test_suite_name} (One or more tests failed)"
        rm "$tmp_output"
        return ${test_exit_code}
    fi
}

# Build and run JavaScript language tests
cd "${CMAKE_PROJECT_BUILD_DIR}"

# Configure project with CMake
echo "Configuring project with CMake..."
cmake "${PROJECT_ROOT_DIR}"
if [ $? -ne 0 ]; then
    echo "ERROR: CMake configuration failed."
    exit 1
fi

# JavaScript language tests
if [ "${RUN_JS_BASIC_AST_TESTS}" = true ]; then
    build_test "js_basic_ast_tests" "JavaScript Basic AST Tests"
    run_test "JavaScript Basic AST Tests" "${CMAKE_PROJECT_BUILD_DIR}/${JS_BASIC_AST_EXECUTABLE_RELPATH}"
fi

if [ "${RUN_JS_EXAMPLE_AST_TESTS}" = true ]; then
    build_test "js_example_ast_tests" "JavaScript Example AST Tests"
    run_test "JavaScript Example AST Tests" "${CMAKE_PROJECT_BUILD_DIR}/${JS_EXAMPLE_AST_EXECUTABLE_RELPATH}"
fi

if [ "${RUN_JS_CST_TESTS}" = true ]; then
    build_test "js_cst_tests" "JavaScript CST Tests"
    run_test "JavaScript CST Tests" "${CMAKE_PROJECT_BUILD_DIR}/${JS_CST_EXECUTABLE_RELPATH}"
fi
