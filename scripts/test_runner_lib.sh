#!/bin/bash
# test_runner_lib.sh - Common functions for all language test runners in ScopeMux

# Global variables
PARALLEL_JOBS=4
TMP_DIR=$(mktemp -d)
TEST_FAILURES=0
TOTAL_MISSING_JSON=0
START_TIME=$(date +%s)

# Standardized error handling
# Handles errors by printing a message and returning 1 (does not exit the script).
handle_error() {
    echo "❌ ERROR: $1"
    return 1
}

# Cleanup function (called on exit)
cleanup() {
    echo "[test_runner_lib] Cleaning up temporary files..."
    rm -rf "$TMP_DIR"
    echo "[test_runner_lib] Done."
}

# Register cleanup function to run on script exit
trap cleanup EXIT

# Standardized build function with improved logging
# Builds a test target. On failure, prints the build log and returns 1 (does not exit the script).
build_test_target() {
    local target_name="$1"
    local display_name="$2"
    local build_log="$TMP_DIR/${target_name}_build.log"

    echo "[test_runner_lib] Building ${display_name}..."
    make "${target_name}" >"$build_log" 2>&1
    local build_exit_code=$?

    if [ $build_exit_code -ne 0 ]; then
        echo "❌ ERROR: Failed to build test target '${display_name}'."
        echo "Build log output:"
        cat "$build_log"
        return 1
    else
        echo "[test_runner_lib] Successfully built ${display_name}"
    fi
}

# Standardized test execution with improved logging
run_test_suite() {
    local test_suite_name="$1"
    local executable_path="$2"
    # Use test name for log file instead of timestamp for easier debugging
    local test_log="$TMP_DIR/$(basename "${executable_path}").log"

    if [ ! -f "${executable_path}" ]; then
        echo "❌ FAIL: ${test_suite_name}. Executable not found: ${executable_path}"
        return 1
    fi

    echo "[test_runner_lib] Running ${test_suite_name}..."

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
    grep -v "FAIL: .* (One or more tests failed)" "$test_log"

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

# Standardized directory processing (recursive, parallel, sorted)
# Handles testing source files with expected JSON output
process_language_tests() {
    local lang=$1
    local -n categories=$2
    local example_executable_path=$3
    local parallel_jobs=${4:-$PARALLEL_JOBS}
    # Default extension is '.c', but can be overridden for other languages
    local file_extension=${5:-.c}

    for category in "${categories[@]}"; do
        local dir="core/tests/examples/$lang/$category"
        if [ ! -d "$dir" ]; then
            echo "[test_runner_lib] Warning: Directory does not exist: $dir"
            continue
        fi
        
        echo "[test_runner_lib] Processing $lang/$category tests..."
        
        # Local counters for this directory
        local failed_tests=0
        local missing_json=0
        
        # Create a semaphore with $PARALLEL_JOBS slots
        local sem="$TMP_DIR/semaphore_$$"
        mkfifo "$sem"
        exec 3<>"$sem"
        rm -f "$sem"
        
        # Initialize semaphore with $PARALLEL_JOBS tokens
        for ((i=1; i<=parallel_jobs; i++)); do
            echo >&3
        done
        
        # Process all test files in directory (sorted alphabetically)
        # Use find to recursively locate all test files and sort them alphabetically
        local OLD_IFS="$IFS"
        IFS=$'\n'
        test_files=($(find "$dir" -type f -name "*.${file_extension#.}" | sort))
        IFS="$OLD_IFS"
        
        if [ ${#test_files[@]} -eq 0 ]; then
            echo "[test_runner_lib] No test files found in $dir"
            continue
        fi
        
        echo "[test_runner_lib] Found ${#test_files[@]} test files"
        
        # Process each test file in parallel (limited by semaphore)
        for test_file in "${test_files[@]}"; do
            # Wait for a semaphore slot
            read -u 3
            
            # Run in background
            {
                # Generate expected JSON filename
                expected_json_file="${test_file}.expected.json"
                
                # Check if expected JSON exists
                if [ -f "$expected_json_file" ]; then
                    # Set environment for tests
                    export SCOPEMUX_TEST_FILE="$test_file"
                    export SCOPEMUX_EXPECTED_JSON="$expected_json_file"
                    
                    local test_name="$lang Example Test: $(basename "$test_file")"
                    echo "[test_runner_lib] Testing: $test_file"

                    local test_log="$TMP_DIR/$(basename "${test_file}").log"
                    pushd "$(dirname "${example_executable_path}")" >/dev/null
                    local raw_log="${test_log}.raw"
                    "./$(basename "${example_executable_path}")" >"$raw_log" 2>&1
                    local test_result=$?
                    awk -v prefix="[$test_name] " '{print prefix $0}' "$raw_log" > "$test_log"
                    rm -f "$raw_log"
                    popd >/dev/null

                    grep -v "FAIL: .* (One or more tests failed)" "$test_log" || true

                    if [ $test_result -ne 0 ]; then
                        failed_tests=$((failed_tests + 1))
                        echo "❌ FAIL: $test_name"
                    else
                        echo "✅ PASS: $test_name"
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
            echo "[test_runner_lib] $failed_tests tests failed in directory: $dir"
            dir_errors=$((dir_errors + failed_tests))
        fi
        
        if [ $missing_json -gt 0 ]; then
            echo "[test_runner_lib] $missing_json JSON files missing in directory: $dir"
            dir_errors=$((dir_errors + missing_json))
            # Track missing JSON files globally
            TOTAL_MISSING_JSON=$((TOTAL_MISSING_JSON + missing_json))
        fi
        
        if [ $dir_errors -eq 0 ]; then
            echo "[test_runner_lib] All tests passed in directory: $dir"
        else
            TEST_FAILURES=$((TEST_FAILURES + 1))
        fi
    done
}

# Setup CMake configuration
setup_cmake_config() {
    local project_root_dir=$1
    local build_dir="${project_root_dir}/build"
    
    # Always run CMake configuration in the build directory
    if [ ! -d "$build_dir" ]; then
        mkdir -p "$build_dir"
    fi
    
    echo "[test_runner_lib] Running CMake configuration: cmake -S $project_root_dir -B $build_dir -G 'Unix Makefiles'"
    cmake -S "$project_root_dir" -B "$build_dir" -G "Unix Makefiles"
    if [ $? -ne 0 ]; then
        handle_error "CMake configuration failed"
    fi
}

# Prepare a clean build directory (optionally skip cleaning)
# Usage: prepare_clean_build_dir <build_dir> <clean_flag>
prepare_clean_build_dir() {
    local build_dir="$1"
    local clean_flag="$2"
    if [ "$clean_flag" = true ] && [ -d "$build_dir" ]; then
        echo "[test_runner_lib] Cleaning build directory: $build_dir"
        rm -rf "$build_dir"
    fi
    if [ ! -d "$build_dir" ]; then
        echo "[test_runner_lib] Creating build directory: $build_dir"
        mkdir -p "$build_dir"
    fi
}

# Print test summary
print_test_summary() {
    local end_time=$(date +%s)
    local total_time=$((end_time - START_TIME))
    
    echo ""
    echo "=======================================================================" 
    echo "                      TEST EXECUTION SUMMARY                          "
    echo "======================================================================="
    echo "Total execution time: $total_time seconds"
    
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
}
