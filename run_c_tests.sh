#!/bin/bash

# Clean build directory before running tests
rm -rf build
mkdir -p build

# Enhanced ScopeMux C Tests Runner Script
# Features:
# - Recursive directory scanning
# - Strict JSON validation
# - Alphabetical test ordering
# - Parallel test execution
# - Proper cleanup handling

# Set fixed parallel jobs for concurrent test execution
# Uses 4 concurrent jobs for optimal performance
PARALLEL_JOBS=4

# Exit on any error
set -e

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

# Create a temporary directory for test logs
TMP_DIR=$(mktemp -d)

# Cleanup function (called on exit)
cleanup() {
    echo "[run_c_tests.sh] Cleaning up temporary files..."
    rm -rf "$TMP_DIR"
    echo "[run_c_tests.sh] Done."
}

# Register cleanup function to run on script exit
trap cleanup EXIT

# Function to build a test with improved error handling
build_test() {
    local target_name="$1"
    local display_name="$2"
    local build_log="$TMP_DIR/${target_name}_build.log"

    echo "[run_c_tests.sh] Building ${display_name}..."
    make "${target_name}" >"$build_log" 2>&1
    local build_exit_code=$?

    if [ $build_exit_code -ne 0 ]; then
        echo "ERROR: Failed to build test target '${display_name}'."
        echo "Build log output:"
        cat "$build_log"
        exit 1
    else
        echo "[run_c_tests.sh] Successfully built ${display_name}"
    fi
}

# Function to run a test with improved logging
run_test() {
    local test_suite_name="$1"
    local executable_path="$2"
    # Use test name for log file instead of timestamp for easier debugging
    local test_log="$TMP_DIR/$(basename "${executable_path}").log"

    if [ ! -f "${executable_path}" ]; then
        echo "FAIL: ${test_suite_name}. Executable not found: ${executable_path}"
        return 1
    fi

    echo "[run_c_tests.sh] Running ${test_suite_name}..."

    pushd "$(dirname "${executable_path}")" >/dev/null
    # Capture output to a temporary log file first
    local raw_log="${test_log}.raw"
    "./$(basename "${executable_path}")" >"$raw_log" 2>&1
    local test_exit_code=$?
    
    # Add test name prefix to each line for better readability with parallel execution
    awk -v prefix="[${test_suite_name}] " '{print prefix $0}' "$raw_log" > "$test_log"
    rm -f "$raw_log"
    popd >/dev/null

    # Remove misleading Criterion summary line from output (both stdout and stderr)
    grep -v "FAIL: C Example AST Tests (One or more tests failed)" "$test_log"

    # Check for the summary line indicating all tests passed
    if grep -q "Failing: 0 | Crashing: 0" "$test_log"; then
        echo "✅ PASS: ${test_suite_name} (All tests passed)"
        return 0
    else
        echo "❌ FAIL: ${test_suite_name} (One or more tests failed)"
        if [ "$test_exit_code" -eq 0 ]; then
            return 1 # Ensure we return failure even if binary exited with 0
        else
            return "$test_exit_code"
        fi
    fi
}

# Command-line flag parsing for advanced options
CLEAN_BUILD=true

# Process command line arguments
for arg in "$@"; do
    case $arg in
    --no-clean)
        CLEAN_BUILD=false
        echo "[run_c_tests.sh] Skipping clean build"
        ;;
    --help)
        echo "Usage: ./run_c_tests.sh [options]"
        echo "Options:"
        echo "  --no-clean      : Skip cleaning build directory"
        echo "  --help          : Show this help message"
        exit 0
        ;;
    esac
done

# Start time tracking
START_TIME=$(date +%s)

# Handle build directory preparation
if [ "$CLEAN_BUILD" = true ] && [ -d "$CMAKE_PROJECT_BUILD_DIR" ]; then
    echo "[run_c_tests.sh] Cleaning build directory: $CMAKE_PROJECT_BUILD_DIR"
    rm -rf "$CMAKE_PROJECT_BUILD_DIR"
fi

# Create build directory if it doesn't exist
if [ ! -d "$CMAKE_PROJECT_BUILD_DIR" ]; then
    echo "[run_c_tests.sh] Creating build directory: $CMAKE_PROJECT_BUILD_DIR"
    mkdir -p "$CMAKE_PROJECT_BUILD_DIR"
fi

# Always run CMake configuration in the build directory
echo "[run_c_tests.sh] Running CMake configuration in $CMAKE_PROJECT_BUILD_DIR"
cd "$CMAKE_PROJECT_BUILD_DIR"
cmake "$PROJECT_ROOT_DIR"
if [ $? -ne 0 ]; then
    echo "[run_c_tests.sh] ❌ ERROR: CMake configuration failed."
    exit 1
fi

# Initialize global counters
TEST_FAILURES=0
TOTAL_MISSING_JSON=0

# Build and run C language tests

# C language tests
if [ "${RUN_C_BASIC_AST_TESTS}" = true ]; then
    build_test "c_basic_ast_tests" "C Basic AST Tests"
    run_test "C Basic AST Tests" "${CMAKE_PROJECT_BUILD_DIR}/${C_BASIC_AST_EXECUTABLE_RELPATH}"
fi

# Enhanced C example test processor with recursive scanning, sorting, and parallelism
process_c_example_tests() {
    local dir="$1"
    local base_path="${PROJECT_ROOT_DIR}/core/tests/examples/c/${dir}"

    # Check if directory exists
    if [ ! -d "$base_path" ]; then
        echo "[run_c_tests.sh] Warning: Directory does not exist: $base_path"
        return 0
    fi

    echo "[run_c_tests.sh] Scanning for C test files in $base_path (recursive)..."

    # Use find to recursively locate all .c files and sort them alphabetically
    local test_files=()
    # Save the old IFS and set it to handle newlines properly
    local OLD_IFS="$IFS"
    IFS=$'\n'
    test_files=($(find "$base_path" -type f -name "*.c" | sort))
    IFS="$OLD_IFS"

    if [ ${#test_files[@]} -eq 0 ]; then
        echo "[run_c_tests.sh] No test files found in $base_path"
        return 0
    fi

    echo "[run_c_tests.sh] Found ${#test_files[@]} test files"

    # Ensure the example test binary is built before running any tests
    local example_exe="${CMAKE_PROJECT_BUILD_DIR}/core/tests/c_example_ast_tests"
    if [ ! -f "$example_exe" ]; then
        build_test "c_example_ast_tests" "C Example AST Example Tests"
    fi

    # Create a temporary FIFO for parallel execution control
    local fifo="$TMP_DIR/test_fifo"
    mkfifo "$fifo"

    # Start background process to manage the semaphore
    exec 3<>"$fifo"
    rm "$fifo"

    # Initialize the semaphore with $PARALLEL_JOBS tokens
    for ((i = 1; i <= PARALLEL_JOBS; i++)); do
        echo >&3
    done

    # Track if any tests failed
    local failed_tests=0
    local missing_json=0

    # Process each test file in parallel (limited by semaphore)
    for test_file in "${test_files[@]}"; do
        # Get a token
        read -u 3

        # Run the test in background
        {
            local json_file="${test_file}.expected.json"
            local test_name=$(basename "$test_file")
            local test_log="${TMP_DIR}/${test_name}.log"

            # Check if JSON file exists
            if [ -f "$json_file" ]; then
                export SCOPEMUX_TEST_FILE="$test_file"
                export SCOPEMUX_EXPECTED_JSON="$json_file"

                run_test "C Example Test: ${test_file}" "$example_exe"
                local test_result=$?

                if [ $test_result -ne 0 ]; then
                    failed_tests=$((failed_tests + 1))
                fi

                unset SCOPEMUX_TEST_FILE
                unset SCOPEMUX_EXPECTED_JSON
            else
                echo "❌ ERROR: Missing required JSON file for ${test_file}"
                missing_json=$((missing_json + 1))
            fi

            # Return the token
            echo >&3
        } &
    done

    # Wait for all background jobs to complete
    wait

    # Close the semaphore
    exec 3>&-

    # Return cumulative error status (don't return immediately on first error)
    local dir_errors=0
    
    if [ $failed_tests -gt 0 ]; then
        echo "[run_c_tests.sh] $failed_tests tests failed in directory: $dir"
        dir_errors=$((dir_errors + failed_tests))
    fi
    
    if [ $missing_json -gt 0 ]; then
        echo "[run_c_tests.sh] $missing_json JSON files missing in directory: $dir"
        dir_errors=$((dir_errors + missing_json))
        # Track missing JSON files globally
        TOTAL_MISSING_JSON=$((TOTAL_MISSING_JSON + missing_json))
    fi
    
    if [ $dir_errors -eq 0 ]; then
        echo "[run_c_tests.sh] All tests passed in directory: $dir"
        return 0
    else
        return $dir_errors
    fi
}

# Build and run C language tests

# Run C language test suite with enhanced features
echo "[run_c_tests.sh] Running C language test suite"

# Run standard tests
if [ "${RUN_C_BASIC_AST_TESTS}" = true ]; then
    build_test "c_basic_ast_tests" "C Basic AST Tests"
    run_test "C Basic AST Tests" "${CMAKE_PROJECT_BUILD_DIR}/${C_BASIC_AST_EXECUTABLE_RELPATH}"
    if [ $? -ne 0 ]; then TEST_FAILURES=$((TEST_FAILURES + 1)); fi
fi

# Run C example tests with enhanced parallel processing
echo "[run_c_tests.sh] Processing C example test directories (with recursive scanning)"

# Run enabled C example tests - now with improved handling
if [ "${RUN_C_BASIC_SYNTAX_TESTS}" = true ]; then
    process_c_example_tests "basic_syntax"
    if [ $? -ne 0 ]; then TEST_FAILURES=$((TEST_FAILURES + 1)); fi
fi

if [ "${RUN_C_COMPLEX_STRUCTURES_TESTS}" = true ]; then
    process_c_example_tests "complex_structures"
    if [ $? -ne 0 ]; then TEST_FAILURES=$((TEST_FAILURES + 1)); fi
fi

if [ "${RUN_C_FILE_IO_TESTS}" = true ]; then
    process_c_example_tests "file_io"
    if [ $? -ne 0 ]; then TEST_FAILURES=$((TEST_FAILURES + 1)); fi
fi

if [ "${RUN_C_MEMORY_MANAGEMENT_TESTS}" = true ]; then
    process_c_example_tests "memory_management"
    if [ $? -ne 0 ]; then TEST_FAILURES=$((TEST_FAILURES + 1)); fi
fi

if [ "${RUN_C_STRUCT_UNION_ENUM_TESTS}" = true ]; then
    process_c_example_tests "struct_union_enum"
    if [ $? -ne 0 ]; then TEST_FAILURES=$((TEST_FAILURES + 1)); fi
fi

# Run other test types
if [ "${RUN_C_CST_TESTS}" = true ]; then
    build_test "c_cst_tests" "C CST Tests"
    run_test "C CST Tests" "${CMAKE_PROJECT_BUILD_DIR}/${C_CST_EXECUTABLE_RELPATH}"
    if [ $? -ne 0 ]; then TEST_FAILURES=$((TEST_FAILURES + 1)); fi
fi

if [ "${RUN_C_PREPROCESSOR_TESTS}" = true ]; then
    build_test "c_preprocessor_tests" "C Preprocessor Tests"
    run_test "C Preprocessor Tests" "${CMAKE_PROJECT_BUILD_DIR}/${C_PREPROCESSOR_EXECUTABLE_RELPATH}"
    if [ $? -ne 0 ]; then TEST_FAILURES=$((TEST_FAILURES + 1)); fi
fi

# Calculate execution time
END_TIME=$(date +%s)
TOTAL_TIME=$((END_TIME - START_TIME))

# Print test execution summary with timing info and missing JSON count
echo ""
echo "======================================================================="
echo "                      TEST EXECUTION SUMMARY                          "
echo "======================================================================="
echo "Total execution time: $TOTAL_TIME seconds"

# Report on missing JSON files even if tests pass
if [ $TOTAL_MISSING_JSON -gt 0 ]; then
    echo "⚠️ Missing JSON files: $TOTAL_MISSING_JSON"
fi

if [ $TEST_FAILURES -eq 0 ]; then
    echo "✅ ALL TESTS PASSED"
    exit 0
else
    echo "❌ TESTS FAILED: $TEST_FAILURES"
    exit 1
fi
