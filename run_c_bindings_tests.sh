#!/bin/bash

# Clean build directory before running tests
rm -rf build

set -x # Enable debug output

# ================ IMPORTANT NOTE ================
# Test Case Toggles control BOTH building AND running of tests
# When a toggle is set to true, the test will be built AND run
# When a toggle is set to false, the test will NOT be built and NOT run
# ===============================================

# -------- Test Case Toggles --------
# Set to true to build and run, false to skip

# Common Test Toggles
RUN_INIT_PARSER_TESTS=true
RUN_EDGE_CASE_TESTS=false

# C Language Test Toggles
RUN_C_BASIC_AST_TESTS=false
RUN_C_EXAMPLE_AST_TESTS=false
RUN_C_CST_TESTS=false
RUN_C_PREPROCESSOR_TESTS=false

# Python Language Test Toggles
RUN_PYTHON_BASIC_AST_TESTS=false
RUN_PYTHON_EXAMPLE_AST_TESTS=false
RUN_PYTHON_CST_TESTS=false

# C++ Language Test Toggles
RUN_CPP_BASIC_AST_TESTS=false
RUN_CPP_EXAMPLE_AST_TESTS=false
RUN_CPP_CST_TESTS=false

# JavaScript Language Test Toggles
RUN_JS_BASIC_AST_TESTS=false
RUN_JS_EXAMPLE_AST_TESTS=false
RUN_JS_CST_TESTS=false

# TypeScript Language Test Toggles
RUN_TS_BASIC_AST_TESTS=false
RUN_TS_EXAMPLE_AST_TESTS=false
RUN_TS_CST_TESTS=false

# Project root directory (assuming this script is in the root)
PROJECT_ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
CORE_DIR="${PROJECT_ROOT_DIR}/core"
TESTS_DIR="${CORE_DIR}/tests"

# Main CMake build directory for the entire project
CMAKE_PROJECT_BUILD_DIR="${PROJECT_ROOT_DIR}/build"

# Relative paths from the CMAKE_PROJECT_BUILD_DIR to where the test executables are located
# Common test executables
INIT_PARSER_EXECUTABLE_RELPATH="core/tests/init_parser_tests"
EDGE_CASE_EXECUTABLE_RELPATH="core/tests/edge_case_tests"

# C language test executables
C_BASIC_AST_EXECUTABLE_RELPATH="core/tests/c_basic_ast_tests"
C_EXAMPLE_AST_EXECUTABLE_RELPATH="core/tests/c_example_ast_tests"
C_CST_EXECUTABLE_RELPATH="core/tests/c_cst_tests"
C_PREPROCESSOR_EXECUTABLE_RELPATH="core/tests/c_preprocessor_tests"

# Python language test executables
PYTHON_BASIC_AST_EXECUTABLE_RELPATH="core/tests/python_basic_ast_tests"
PYTHON_EXAMPLE_AST_EXECUTABLE_RELPATH="core/tests/python_example_ast_tests"
PYTHON_CST_EXECUTABLE_RELPATH="core/tests/python_cst_tests"

# C++ language test executables
CPP_BASIC_AST_EXECUTABLE_RELPATH="core/tests/cpp_basic_ast_tests"
CPP_EXAMPLE_AST_EXECUTABLE_RELPATH="core/tests/cpp_example_ast_tests"
CPP_CST_EXECUTABLE_RELPATH="core/tests/cpp_cst_tests"
# JavaScript language test executables
JS_BASIC_AST_EXECUTABLE_RELPATH="core/tests/js_basic_ast_tests"
JS_EXAMPLE_AST_EXECUTABLE_RELPATH="core/tests/js_example_ast_tests"
JS_CST_EXECUTABLE_RELPATH="core/tests/js_cst_tests"

# TypeScript language test executables
TS_BASIC_AST_EXECUTABLE_RELPATH="core/tests/ts_basic_ast_tests"
TS_EXAMPLE_AST_EXECUTABLE_RELPATH="core/tests/ts_example_ast_tests"
TS_CST_EXECUTABLE_RELPATH="core/tests/ts_cst_tests"


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
cmake "${PROJECT_ROOT_DIR}"
if [ $? -ne 0 ]; then
    echo "ERROR: CMake configuration failed."
    # JavaScript language tests
if [ "${RUN_JS_BASIC_AST_TESTS}" = true ]; then
    build_test "js_basic_ast_tests" "JavaScript Basic AST Tests"
fi

if [ "${RUN_JS_EXAMPLE_AST_TESTS}" = true ]; then
    build_test "js_example_ast_tests" "JavaScript Example AST Tests"
fi

if [ "${RUN_JS_CST_TESTS}" = true ]; then
    build_test "js_cst_tests" "JavaScript CST Tests"
fi

# TypeScript language tests
if [ "${RUN_TS_BASIC_AST_TESTS}" = true ]; then
    build_test "ts_basic_ast_tests" "TypeScript Basic AST Tests"
fi

if [ "${RUN_TS_EXAMPLE_AST_TESTS}" = true ]; then
    build_test "ts_example_ast_tests" "TypeScript Example AST Tests"
fi

if [ "${RUN_TS_CST_TESTS}" = true ]; then
    build_test "ts_cst_tests" "TypeScript CST Tests"
fi

    exit 1
fi

# Build tests based on toggle settings
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

# Common tests
if [ "${RUN_INIT_PARSER_TESTS}" = true ]; then
    build_test "init_parser_tests" "Init Parser Tests"
fi

if [ "${RUN_EDGE_CASE_TESTS}" = true ]; then
    build_test "edge_case_tests" "Edge Case Tests"
fi

# C language tests
if [ "${RUN_C_BASIC_AST_TESTS}" = true ]; then
    build_test "c_basic_ast_tests" "C Basic AST Tests"
fi

if [ "${RUN_C_EXAMPLE_AST_TESTS}" = true ]; then
    build_test "c_example_ast_tests" "C Example AST Tests"
fi

if [ "${RUN_C_CST_TESTS}" = true ]; then
    build_test "c_cst_tests" "C CST Tests"
fi

if [ "${RUN_C_PREPROCESSOR_TESTS}" = true ]; then
    build_test "c_preprocessor_tests" "C Preprocessor Tests"
fi

# Python language tests
if [ "${RUN_PYTHON_BASIC_AST_TESTS}" = true ]; then
    build_test "python_basic_ast_tests" "Python Basic AST Tests"
fi

if [ "${RUN_PYTHON_EXAMPLE_AST_TESTS}" = true ]; then
    build_test "python_example_ast_tests" "Python Example AST Tests"
fi

if [ "${RUN_PYTHON_CST_TESTS}" = true ]; then
    build_test "python_cst_tests" "Python CST Tests"
fi

# C++ language tests
if [ "${RUN_CPP_BASIC_AST_TESTS}" = true ]; then
    build_test "cpp_basic_ast_tests" "C++ Basic AST Tests"
fi

if [ "${RUN_CPP_EXAMPLE_AST_TESTS}" = true ]; then
    build_test "cpp_example_ast_tests" "C++ Example AST Tests"
fi

if [ "${RUN_CPP_CST_TESTS}" = true ]; then
    build_test "cpp_cst_tests" "C++ CST Tests"
fi

cd "${PROJECT_ROOT_DIR}"
echo "C tests build process finished."

# --- Run Tests ---
echo "Running enabled tests..."

# Common tests
if [ "${RUN_INIT_PARSER_TESTS}" = true ]; then
    run_criterion_test_executable "Init Parser Tests" \
        "${CMAKE_PROJECT_BUILD_DIR}/${INIT_PARSER_EXECUTABLE_RELPATH}"
fi

if [ "${RUN_EDGE_CASE_TESTS}" = true ]; then
    run_criterion_test_executable "Edge Case Tests" \
        "${CMAKE_PROJECT_BUILD_DIR}/${EDGE_CASE_EXECUTABLE_RELPATH}"
fi

# C language tests
if [ "${RUN_C_BASIC_AST_TESTS}" = true ]; then
    run_criterion_test_executable "C Basic AST Tests" \
        "${CMAKE_PROJECT_BUILD_DIR}/${C_BASIC_AST_EXECUTABLE_RELPATH}"
fi

if [ "${RUN_C_EXAMPLE_AST_TESTS}" = true ]; then
    run_criterion_test_executable "C Example AST Tests" \
        "${CMAKE_PROJECT_BUILD_DIR}/${C_EXAMPLE_AST_EXECUTABLE_RELPATH}"
fi

if [ "${RUN_C_CST_TESTS}" = true ]; then
    run_criterion_test_executable "C CST Tests" \
        "${CMAKE_PROJECT_BUILD_DIR}/${C_CST_EXECUTABLE_RELPATH}"
fi

if [ "${RUN_C_PREPROCESSOR_TESTS}" = true ]; then
    run_criterion_test_executable "C Preprocessor Tests" \
        "${CMAKE_PROJECT_BUILD_DIR}/${C_PREPROCESSOR_EXECUTABLE_RELPATH}"
fi

# Python language tests
if [ "${RUN_PYTHON_BASIC_AST_TESTS}" = true ]; then
    run_criterion_test_executable "Python Basic AST Tests" \
        "${CMAKE_PROJECT_BUILD_DIR}/${PYTHON_BASIC_AST_EXECUTABLE_RELPATH}"
fi

if [ "${RUN_PYTHON_EXAMPLE_AST_TESTS}" = true ]; then
    run_criterion_test_executable "Python Example AST Tests" \
        "${CMAKE_PROJECT_BUILD_DIR}/${PYTHON_EXAMPLE_AST_EXECUTABLE_RELPATH}"
fi

if [ "${RUN_PYTHON_CST_TESTS}" = true ]; then
    run_criterion_test_executable "Python CST Tests" \
        "${CMAKE_PROJECT_BUILD_DIR}/${PYTHON_CST_EXECUTABLE_RELPATH}"
fi

# C++ language tests
if [ "${RUN_CPP_BASIC_AST_TESTS}" = true ]; then
    run_criterion_test_executable "C++ Basic AST Tests" \
        "${CMAKE_PROJECT_BUILD_DIR}/${CPP_BASIC_AST_EXECUTABLE_RELPATH}"
fi

if [ "${RUN_CPP_EXAMPLE_AST_TESTS}" = true ]; then
    run_criterion_test_executable "C++ Example AST Tests" \
        "${CMAKE_PROJECT_BUILD_DIR}/${CPP_EXAMPLE_AST_EXECUTABLE_RELPATH}"
fi

if [ "${RUN_CPP_CST_TESTS}" = true ]; then
    run_criterion_test_executable "C++ CST Tests" \
        "${CMAKE_PROJECT_BUILD_DIR}/${CPP_CST_EXECUTABLE_RELPATH}"
fi

# JavaScript language tests
if [ "${RUN_JS_BASIC_AST_TESTS}" = true ]; then
    run_criterion_test_executable "JavaScript Basic AST Tests" \
        "${CMAKE_PROJECT_BUILD_DIR}/${JS_BASIC_AST_EXECUTABLE_RELPATH}"
fi

if [ "${RUN_JS_EXAMPLE_AST_TESTS}" = true ]; then
    run_criterion_test_executable "JavaScript Example AST Tests" \
        "${CMAKE_PROJECT_BUILD_DIR}/${JS_EXAMPLE_AST_EXECUTABLE_RELPATH}"
fi

if [ "${RUN_JS_CST_TESTS}" = true ]; then
    run_criterion_test_executable "JavaScript CST Tests" \
        "${CMAKE_PROJECT_BUILD_DIR}/${JS_CST_EXECUTABLE_RELPATH}"
fi

# TypeScript language tests
if [ "${RUN_TS_BASIC_AST_TESTS}" = true ]; then
    run_criterion_test_executable "TypeScript Basic AST Tests" \
        "${CMAKE_PROJECT_BUILD_DIR}/${TS_BASIC_AST_EXECUTABLE_RELPATH}"
fi

if [ "${RUN_TS_EXAMPLE_AST_TESTS}" = true ]; then
    run_criterion_test_executable "TypeScript Example AST Tests" \
        "${CMAKE_PROJECT_BUILD_DIR}/${TS_EXAMPLE_AST_EXECUTABLE_RELPATH}"
fi

if [ "${RUN_TS_CST_TESTS}" = true ]; then
    run_criterion_test_executable "TypeScript CST Tests" \
        "${CMAKE_PROJECT_BUILD_DIR}/${TS_CST_EXECUTABLE_RELPATH}"
fi

echo "Note: Tests marked as false in the toggle section were skipped."

# Note: This script doesn't aggregate overall pass/fail status yet.
# You could add a counter for failed tests and exit with a non-zero status if any test fails.

exit 0
