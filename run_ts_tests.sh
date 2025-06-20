#!/bin/bash

# Clean build directory before running tests
rm -rf build
mkdir -p build

# TypeScript Language Test Toggles
RUN_TS_BASIC_AST_TESTS=false
RUN_TS_EXAMPLE_AST_TESTS=false
RUN_TS_CST_TESTS=false

# Project root directory (assuming this script is in the root)
PROJECT_ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
CMAKE_PROJECT_BUILD_DIR="${PROJECT_ROOT_DIR}/build"

# TypeScript language test executables
TS_BASIC_AST_EXECUTABLE_RELPATH="core/tests/ts_basic_ast_tests"
TS_EXAMPLE_AST_EXECUTABLE_RELPATH="core/tests/ts_example_ast_tests"
TS_CST_EXECUTABLE_RELPATH="core/tests/ts_cst_tests"

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
    "./$(basename "${executable_path}")"
    local test_exit_code=$?
    popd >/dev/null

    if [ ${test_exit_code} -eq 0 ]; then
        echo "PASS: ${test_suite_name} (All tests passed)"
    else
        echo "FAIL: ${test_suite_name} (One or more tests failed)"
    fi
    return ${test_exit_code}
}

# Build and run TypeScript language tests
cd "${CMAKE_PROJECT_BUILD_DIR}"

# Configure project with CMake
echo "Configuring project with CMake..."
cmake "${PROJECT_ROOT_DIR}"
if [ $? -ne 0 ]; then
    echo "ERROR: CMake configuration failed."
    exit 1
fi

# TypeScript language tests
if [ "${RUN_TS_BASIC_AST_TESTS}" = true ]; then
    build_test "ts_basic_ast_tests" "TypeScript Basic AST Tests"
    run_test "TypeScript Basic AST Tests" "${CMAKE_PROJECT_BUILD_DIR}/${TS_BASIC_AST_EXECUTABLE_RELPATH}"
fi

if [ "${RUN_TS_EXAMPLE_AST_TESTS}" = true ]; then
    build_test "ts_example_ast_tests" "TypeScript Example AST Tests"
    run_test "TypeScript Example AST Tests" "${CMAKE_PROJECT_BUILD_DIR}/${TS_EXAMPLE_AST_EXECUTABLE_RELPATH}"
fi

if [ "${RUN_TS_CST_TESTS}" = true ]; then
    build_test "ts_cst_tests" "TypeScript CST Tests"
    run_test "TypeScript CST Tests" "${CMAKE_PROJECT_BUILD_DIR}/${TS_CST_EXECUTABLE_RELPATH}"
fi
