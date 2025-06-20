#!/bin/bash

# C++ Language Test Toggles
RUN_CPP_BASIC_AST_TESTS=false
RUN_CPP_EXAMPLE_AST_TESTS=false
RUN_CPP_CST_TESTS=false

# Project root directory (assuming this script is in the root)
PROJECT_ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
CMAKE_PROJECT_BUILD_DIR="${PROJECT_ROOT_DIR}/build"

# C++ language test executables
CPP_BASIC_AST_EXECUTABLE_RELPATH="core/tests/cpp_basic_ast_tests"
CPP_EXAMPLE_AST_EXECUTABLE_RELPATH="core/tests/cpp_example_ast_tests"
CPP_CST_EXECUTABLE_RELPATH="core/tests/cpp_cst_tests"

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

# Build and run C++ language tests
cd "${CMAKE_PROJECT_BUILD_DIR}"

# C++ language tests
if [ "${RUN_CPP_BASIC_AST_TESTS}" = true ]; then
    build_test "cpp_basic_ast_tests" "C++ Basic AST Tests"
    run_test "C++ Basic AST Tests" "${CMAKE_PROJECT_BUILD_DIR}/${CPP_BASIC_AST_EXECUTABLE_RELPATH}"
fi

if [ "${RUN_CPP_EXAMPLE_AST_TESTS}" = true ]; then
    build_test "cpp_example_ast_tests" "C++ Example AST Tests"
    run_test "C++ Example AST Tests" "${CMAKE_PROJECT_BUILD_DIR}/${CPP_EXAMPLE_AST_EXECUTABLE_RELPATH}"
fi

if [ "${RUN_CPP_CST_TESTS}" = true ]; then
    build_test "cpp_cst_tests" "C++ CST Tests"
    run_test "C++ CST Tests" "${CMAKE_PROJECT_BUILD_DIR}/${CPP_CST_EXECUTABLE_RELPATH}"
fi
