#!/bin/bash

# Clean build directory before running tests
rm -rf build
mkdir -p build

# C Language Test Toggles
RUN_C_BASIC_AST_TESTS=true
RUN_C_CST_TESTS=false
RUN_C_PREPROCESSOR_TESTS=false

# C example test directory toggles
RUN_C_BASIC_SYNTAX_TESTS=true
RUN_C_COMPLEX_STRUCTURES_TESTS=true
RUN_C_FILE_IO_TESTS=true
RUN_C_MEMORY_MANAGEMENT_TESTS=true
RUN_C_STRUCT_UNION_ENUM_TESTS=true

# Project root directory (assuming this script is in the root)
PROJECT_ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
CMAKE_PROJECT_BUILD_DIR="${PROJECT_ROOT_DIR}/build"

# C language test executables
C_BASIC_AST_EXECUTABLE_RELPATH="core/tests/c_basic_ast_tests"
C_EXAMPLE_AST_EXECUTABLE_RELPATH="core/tests/c_example_ast_tests"
C_CST_EXECUTABLE_RELPATH="core/tests/c_cst_tests"
C_PREPROCESSOR_EXECUTABLE_RELPATH="core/tests/c_preprocessor_tests"

# Function to build a test
build_test() {
    local target_name="$1"
    local display_name="$2"

    make "${target_name}"
    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to build test target '${display_name}'."
        exit 1
    fi
}

# Function to run a test
run_test() {
    local test_suite_name="$1"
    local executable_path="$2"

    if [ ! -f "${executable_path}" ]; then
        echo "FAIL: ${test_suite_name}. Executable not found: ${executable_path}"
        return 1
    fi

    pushd "$(dirname "${executable_path}")" >/dev/null
    # Capture output to a temp file
    local tmp_output
    tmp_output=$(mktemp)
    "./$(basename "${executable_path}")" 2>&1 | tee "$tmp_output"
    local test_exit_code=${PIPESTATUS[0]}
    popd >/dev/null

    # Remove misleading Criterion summary line from output (both stdout and stderr)
    grep -v "FAIL: C Example AST Tests (One or more tests failed)" "$tmp_output"

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


# Ensure a clean build directory every test run
if [ -d "$CMAKE_PROJECT_BUILD_DIR" ]; then
    echo "[run_c_tests.sh] Cleaning build directory: $CMAKE_PROJECT_BUILD_DIR"
    rm -rf "$CMAKE_PROJECT_BUILD_DIR"
fi
mkdir -p "$CMAKE_PROJECT_BUILD_DIR"

# Always run CMake configuration in the build directory
echo "[run_c_tests.sh] Running CMake configuration in $CMAKE_PROJECT_BUILD_DIR"
cd "$CMAKE_PROJECT_BUILD_DIR"
cmake "$PROJECT_ROOT_DIR"
if [ $? -ne 0 ]; then
    echo "ERROR: CMake configuration failed."
    exit 1
fi

# Build and run C language tests

# C language tests
if [ "${RUN_C_BASIC_AST_TESTS}" = true ]; then
    build_test "c_basic_ast_tests" "C Basic AST Tests"
    run_test "C Basic AST Tests" "${CMAKE_PROJECT_BUILD_DIR}/${C_BASIC_AST_EXECUTABLE_RELPATH}"
fi

# Process C example test directories
process_c_example_tests() {
    local dir="$1"
    local test_files=($(find "${PROJECT_ROOT_DIR}/core/tests/examples/c/${dir}" -name "*.c" 2>/dev/null))
    
    for test_file in "${test_files[@]}"; do
        local json_file="${test_file}.expected.json"
        local example_exe="${CMAKE_PROJECT_BUILD_DIR}/core/tests/c_example_ast_tests"
        if [ -f "$json_file" ]; then
            # Ensure the example test binary is built before running
            if [ ! -f "$example_exe" ]; then
                build_test "c_example_ast_tests" "C Example AST Example Tests"
            fi
            export SCOPEMUX_TEST_FILE="$test_file"
            export SCOPEMUX_EXPECTED_JSON="$json_file"
            run_test "C Example Test: ${test_file}" "$example_exe"
            unset SCOPEMUX_TEST_FILE
            unset SCOPEMUX_EXPECTED_JSON
        else
            echo "WARN: Missing expected JSON for ${test_file}"
        fi
    done
}

# Run enabled C example tests
if [ "${RUN_C_BASIC_SYNTAX_TESTS}" = true ]; then
    process_c_example_tests "basic_syntax"
fi
if [ "${RUN_C_COMPLEX_STRUCTURES_TESTS}" = true ]; then
    process_c_example_tests "complex_structures"
fi
if [ "${RUN_C_FILE_IO_TESTS}" = true ]; then
    process_c_example_tests "file_io"
fi
if [ "${RUN_C_MEMORY_MANAGEMENT_TESTS}" = true ]; then
    process_c_example_tests "memory_management"
fi
if [ "${RUN_C_STRUCT_UNION_ENUM_TESTS}" = true ]; then
    process_c_example_tests "struct_union_enum"
fi

if [ "${RUN_C_CST_TESTS}" = true ]; then
    build_test "c_cst_tests" "C CST Tests"
    run_test "C CST Tests" "${CMAKE_PROJECT_BUILD_DIR}/${C_CST_EXECUTABLE_RELPATH}"
fi

if [ "${RUN_C_PREPROCESSOR_TESTS}" = true ]; then
    build_test "c_preprocessor_tests" "C Preprocessor Tests"
    run_test "C Preprocessor Tests" "${CMAKE_PROJECT_BUILD_DIR}/${C_PREPROCESSOR_EXECUTABLE_RELPATH}"
fi
