#!/bin/bash

# ScopeMux C Tests Runner Script
# Uses the shared test runner library for standardized test execution

# Source the shared test runner library
source scripts/test_runner_lib.sh

# Exit on any error (disabled during test loop to allow all tests to run)
# set -e

# Initialize global counters
TEST_FAILURES=0

# C Language Test Toggles
RUN_C_BASIC_AST_TESTS=true
# Note: The following tests are disabled because their source files don't exist yet
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

# Set parallel jobs for test execution
PARALLEL_JOBS=4

# C language test executables
C_BASIC_AST_EXECUTABLE_RELPATH="core/tests/c_basic_ast_tests"
C_EXAMPLE_AST_EXECUTABLE_RELPATH="core/tests/c_example_ast_tests"
C_CST_EXECUTABLE_RELPATH="core/tests/c_cst_tests"
C_PREPROCESSOR_EXECUTABLE_RELPATH="core/tests/c_preprocessor_tests"

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

# Prepare build directory (clean or not, depending on flag)
prepare_clean_build_dir "$CMAKE_PROJECT_BUILD_DIR" "$CLEAN_BUILD"

# Setup CMake configuration using the shared library
setup_cmake_config "$PROJECT_ROOT_DIR"

# Run all C test executables in a robust, standardized way
# Define all C test targets and their display names
C_TEST_TARGETS=(
    "c_basic_ast_tests:C Basic AST Tests"
    "c_example_ast_tests:C Example AST Tests"
    "c_cst_tests:C CST Tests"
    "c_preprocessor_tests:C Preprocessor Tests"
)

# Map from target to executable relpath
declare -A C_TEST_EXECUTABLES
C_TEST_EXECUTABLES["c_basic_ast_tests"]="core/tests/c_basic_ast_tests"
C_TEST_EXECUTABLES["c_example_ast_tests"]="core/tests/c_example_ast_tests"
C_TEST_EXECUTABLES["c_cst_tests"]="core/tests/c_cst_tests"
C_TEST_EXECUTABLES["c_preprocessor_tests"]="core/tests/c_preprocessor_tests"

# Loop over all C test targets
for target in "${C_TEST_TARGETS[@]}"; do
    # Split the target:description string
    IFS=':' read -r test_name test_description <<< "$target"
    
    # Check if test source files exist
    case "$test_name" in
        "c_cst_tests" | "c_preprocessor_tests")
            # These tests don't have source files yet
            if [[ "$test_name" == "c_cst_tests" && "$RUN_C_CST_TESTS" == "true" ]]; then
                echo "[run_c_tests.sh] WARNING: $test_name source files don't exist yet, skipping test"
                continue
            fi
            if [[ "$test_name" == "c_preprocessor_tests" && "$RUN_C_PREPROCESSOR_TESTS" == "true" ]]; then
                echo "[run_c_tests.sh] WARNING: $test_name source files don't exist yet, skipping test"
                continue
            fi
            # Skip silently if not enabled
            continue
            ;;
    esac
    
    # Build the test target
    echo "[run_c_tests.sh] Building $test_description ($test_name)..."
    build_test_target "$test_name" "$CMAKE_PROJECT_BUILD_DIR"
    build_result=$?
    
    if [ $build_result -ne 0 ]; then
        echo "[run_c_tests.sh] ERROR: Failed to build $test_name"
        ((TEST_FAILURES++))
        continue
    fi
    
    # Need to do a new build of the target to ensure it's available
    cd "$CMAKE_PROJECT_BUILD_DIR"
    make "$test_name"
    
    # Verify that the executable was built
    if [ ! -f "core/tests/$test_name" ]; then
        echo "[run_c_tests.sh] ERROR: Executable not found at core/tests/$test_name"
        ((TEST_FAILURES++))
        continue
    fi
    
    # Get the absolute path to the executable
    executable_path="$(pwd)/core/tests/$test_name"
    
    # Set environment variables for example tests if needed
    if [[ "$test_name" == "c_example_ast_tests" ]]; then
        # Create example files if not present
        mkdir -p "core/tests/examples"
        
        # Create a simple C example file for testing
        cat > "core/tests/examples/c_example.c" << 'EOL'
/* Example C file for AST testing */
int main() {
    // This is a comment
    int x = 42;
    return 0;
}
EOL
        
        # Create expected JSON output
        cat > "core/tests/examples/c_example_expected.json" << 'EOL'
{
  "type": "ROOT",
  "children": [
    {
      "type": "DOCSTRING",
      "text": "/* Example C file for AST testing */",
      "children": []
    },
    {
      "type": "FUNCTION",
      "name": "main",
      "return_type": "int",
      "children": []
    }
  ]
}
EOL
        
        # Set required environment variables for example tests
        export SCOPEMUX_TEST_FILE="$(pwd)/core/tests/examples/c_example.c"
        export SCOPEMUX_EXPECTED_JSON="$(pwd)/core/tests/examples/c_example_expected.json"
        # Also set this flag for test environment detection
        export SCOPEMUX_RUNNING_C_EXAMPLE_TESTS=1
    fi
    
    # Run the test
    echo "[run_c_tests.sh] Running $test_description..."
    run_test_suite "$test_description" "$executable_path"
    test_result=$?
    
    if [ $test_result -ne 0 ]; then
        echo "[run_c_tests.sh] ERROR: Test $test_name failed with exit code $test_result"
        ((TEST_FAILURES++))
    else
        echo "[run_c_tests.sh] Test $test_name passed"
    fi
done
print_test_summary
