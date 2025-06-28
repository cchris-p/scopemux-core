#!/bin/bash
# test_runner_lib.sh - Common functions for all language test runners in ScopeMux

# Global variables
PARALLEL_JOBS=4
TMP_DIR=$(mktemp -d)
TEST_FAILURES=0
TOTAL_MISSING_JSON=0
START_TIME=$(date +%s)
# Associative array for per-suite results (Bash 4+)
declare -A TEST_SUITE_RESULTS

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

    echo "[test_runner_lib] Checking for test executable: ${executable_path}"
    
    if [ ! -f "${executable_path}" ]; then
        echo "❌ FAIL: ${test_suite_name}. Executable not found: ${executable_path}"
        # Try to help diagnose the issue
        echo "[test_runner_lib] Searching for the missing executable..."
        find "$(dirname "${executable_path}")" -type f -executable -name "$(basename "${executable_path}")" || true
        TEST_SUITE_RESULTS["$test_suite_name"]="FAIL"
        return 1
    fi

    echo "[test_runner_lib] Running ${test_suite_name}: ${executable_path}"

    # Execute directly with full path instead of changing directory
    # Capture output to a temporary log file first
    local raw_log="${test_log}.raw"
    "${executable_path}" >"$raw_log" 2>&1
    local test_exit_code=$?

    # Add test name prefix to each line for better readability with parallel execution
    awk -v prefix="[${test_suite_name}] " '{print prefix $0}' "$raw_log" >"$test_log"
    rm -f "$raw_log"
    
    # Output the test result log with line identification
    echo "[test_runner_lib] Test output from ${test_suite_name}:"
    cat "$test_log"
    
    # Remove misleading Criterion summary line from output (both stdout and stderr)
    grep -v "FAIL: .* (One or more tests failed)" "$test_log" >/dev/null 2>&1

    # Check for the summary line indicating all tests passed
    if grep -q "Failing: 0 | Crashing: 0" "$test_log"; then
        # All tests passed in suite
        echo -e "\033[1;32m✅✅ PASS: ${test_suite_name} (All tests passed)\033[0m"
        TEST_SUITE_RESULTS["$test_suite_name"]="PASS"
        return 0
    else
        echo -e "\033[1;31m❌ FAIL: ${test_suite_name} (One or more tests failed)\033[0m"
        TEST_SUITE_RESULTS["$test_suite_name"]="FAIL"
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
        
        # Create temporary files to track failures across parallel jobs
        local fail_counter="$TMP_DIR/${lang}_${category}_failures"
        local missing_counter="$TMP_DIR/${lang}_${category}_missing"
        echo 0 > "$fail_counter"
        echo 0 > "$missing_counter"

        # Create a semaphore with $PARALLEL_JOBS slots
        local sem="$TMP_DIR/semaphore_$$"
        mkfifo "$sem"
        exec 3<>"$sem"
        rm -f "$sem"

        # Initialize semaphore with $PARALLEL_JOBS tokens
        for ((i = 1; i <= parallel_jobs; i++)); do
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
                    local executable="./$(basename "${example_executable_path}")"
                    local test_result=1
                    local raw_log="${test_log}.raw"
                    
                    if [ -x "$executable" ]; then
                        "$executable" >"$raw_log" 2>&1
                        test_result=$?
                        awk -v prefix="[$test_name] " '{print prefix $0}' "$raw_log" >"$test_log"
                        grep -v "FAIL: .* (One or more tests failed)" "$test_log" || true
                    else
                        echo "[$test_name] ERROR: Executable not found: $executable" >"$test_log"
                        echo "[$test_name] This is likely due to a build failure. Check the build logs for errors." >>"$test_log"
                        cat "$test_log"
                    fi
                    
                    rm -f "$raw_log"
                    popd >/dev/null

                    # Don't need additional filtering as it's already done above

                    if [ $test_result -ne 0 ]; then
                        # Update the failure counter in the temp file atomically
                        local current_fails=$(cat "$fail_counter")
                        echo $((current_fails + 1)) > "$fail_counter"
                        echo "❌ FAIL: $test_name ($((i+1))/$total_tests)"
                    else
                        echo "✅ PASS: $test_name ($((i+1))/$total_tests)"
                    fi

                    unset SCOPEMUX_TEST_FILE
                    unset SCOPEMUX_EXPECTED_JSON
                else
                    echo "❌ ERROR: Missing expected JSON for test: $test_file"
                    # Update the missing counter in the temp file atomically
                    local current_missing=$(cat "$missing_counter")
                    echo $((current_missing + 1)) > "$missing_counter"
                fi

                # Return the token
                echo >&3
            } &
        done

        # Wait for all background jobs to finish
        for job in $(jobs -p); do
            wait $job
        done
        
        # Collect failure counts from temporary files
        failed_tests=$(cat "$fail_counter")
        missing_json=$(cat "$missing_counter")
        
        # Clean up temporary counter files
        rm -f "$fail_counter" "$missing_counter"

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

        local total_tests=${#test_files[@]}
        local passed_tests=$((total_tests - failed_tests - missing_json))
        if [ $dir_errors -eq 0 ]; then
            # All tests passed in directory
            echo -e "\033[1;32m✅✅ PASS: $lang/$category ($passed_tests/$total_tests tests passed)\033[0m"
            TEST_SUITE_RESULTS["$lang/$category"]="PASS"
        else
            # Some tests failed or missing JSON
            echo -e "\033[1;31m❌ FAIL: $lang/$category ($passed_tests/$total_tests tests passed, $dir_errors problems in directory)\033[0m"
            TEST_SUITE_RESULTS["$lang/$category"]="FAIL"
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

    # Per-suite summary table
    if [ ${#TEST_SUITE_RESULTS[@]} -gt 0 ]; then
        printf "\n%-40s | %-8s\n" "Test Suite" "Result"
        printf '%s\n' "------------------------------------------|----------"
        for suite in "${!TEST_SUITE_RESULTS[@]}"; do
            result="${TEST_SUITE_RESULTS[$suite]}"
            if [ "$result" = "PASS" ]; then
                printf "\033[1;32m%-40s | %-8s\033[0m\n" "$suite" "✅ PASS"
            else
                printf "\033[1;31m%-40s | %-8s\033[0m\n" "$suite" "❌ FAIL"
            fi
        done
        echo ""
    fi

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

