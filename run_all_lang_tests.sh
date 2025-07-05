#!/bin/bash

# ScopeMux Parallel Test Runner
# Executes all language tests in parallel with comprehensive logging and reporting
#
# This script runs the following test suites in parallel:
# - C tests (run_c_tests.sh)
# - C++ tests (run_cpp_tests.sh)
# - Python tests (run_python_tests.sh)
# - JavaScript tests (run_js_tests.sh)
# - TypeScript tests (run_ts_tests.sh)
#
# Features:
# - Parallel execution with configurable job limits
# - Individual log files for each language
# - Comprehensive summary reporting
# - Color-coded output
# - Error handling and cleanup
# - Progress tracking

set -euo pipefail

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT_DIR="$SCRIPT_DIR"
PARALLEL_JOBS=${PARALLEL_JOBS:-4} # Default to 4 parallel jobs
CLEAN_BUILD=${CLEAN_BUILD:-true}  # Default to clean builds

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test suite definitions
declare -A TEST_SUITES=(
    ["c"]="C Language Tests"
    ["cpp"]="C++ Language Tests"
    ["python"]="Python Language Tests"
    ["js"]="JavaScript Language Tests"
    ["ts"]="TypeScript Language Tests"
)

# Test script paths
declare -A TEST_SCRIPTS=(
    ["c"]="run_c_tests.sh"
    ["cpp"]="run_cpp_tests.sh"
    ["python"]="run_python_tests.sh"
    ["js"]="run_js_tests.sh"
    ["ts"]="run_ts_tests.sh"
)

# Global variables
START_TIME=$(date +%s)
TOTAL_TESTS=${#TEST_SUITES[@]}
COMPLETED_TESTS=0
FAILED_TESTS=0
PASSED_TESTS=0
SKIPPED_TESTS=0

# Results tracking
declare -A TEST_RESULTS
declare -A TEST_PIDS
declare -A TEST_LOG_FILES

# Temporary directory for parallel execution
TMP_DIR=$(mktemp -d)
SEMAPHORE="$TMP_DIR/semaphore"

# Cleanup function
cleanup() {
    echo -e "\n${BLUE}[Parallel Test Runner]${NC} Cleaning up..."
    rm -rf "$TMP_DIR"

    # Kill any remaining background processes
    for pid in "${TEST_PIDS[@]}"; do
        if kill -0 "$pid" 2>/dev/null; then
            echo -e "${YELLOW}[Parallel Test Runner]${NC} Terminating background process $pid"
            kill -TERM "$pid" 2>/dev/null || true
        fi
    done

    # Wait for all background processes to finish
    wait 2>/dev/null || true
}

# Setup semaphore for parallel execution
setup_semaphore() {
    mkfifo "$SEMAPHORE"
    exec 3<>"$SEMAPHORE"
    rm -f "$SEMAPHORE"

    # Initialize semaphore with tokens
    for ((i = 1; i <= PARALLEL_JOBS; i++)); do
        echo >&3
    done
}

# Print header
print_header() {
    echo -e "\n${BLUE}========================================${NC}"
    echo -e "${BLUE}  ScopeMux Parallel Test Runner${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo -e "Project Root: ${PROJECT_ROOT_DIR}"
    echo -e "Parallel Jobs: ${PARALLEL_JOBS}"
    echo -e "Clean Build: ${CLEAN_BUILD}"
    echo -e "Start Time: $(date)"
    echo -e "${BLUE}========================================${NC}\n"
}

# Print progress
print_progress() {
    local current=$1
    local total=$2
    local percentage=$((current * 100 / total))
    local bar_length=30
    local filled=$((percentage * bar_length / 100))
    local empty=$((bar_length - filled))

    printf "\r${BLUE}[Progress]${NC} ["
    printf "%${filled}s" | tr ' ' '#'
    printf "%${empty}s" | tr ' ' '-'
    printf "] %d%% (%d/%d)" "$percentage" "$current" "$total"
}

# Run a single test suite
run_test_suite() {
    local lang="$1"
    local script_name="${TEST_SCRIPTS[$lang]}"
    local script_path="${PROJECT_ROOT_DIR}/${script_name}"
    local log_file="${PROJECT_ROOT_DIR}/run_${lang}_tests_parallel.log"

    # Wait for semaphore slot
    read -u 3

    {
        echo -e "${BLUE}[$lang]${NC} Starting ${TEST_SUITES[$lang]}..."
        echo -e "${BLUE}[$lang]${NC} Log file: $log_file"

        # Set environment variables for the test script
        export PROJECT_ROOT_DIR
        export PARALLEL_JOBS=1 # Individual scripts run single-threaded
        export CLEAN_BUILD

        # Run the test script with output redirected to log file
        if bash "$script_path" >"$log_file" 2>&1; then
            echo -e "${GREEN}[$lang]${NC} ${TEST_SUITES[$lang]} completed successfully"
            TEST_RESULTS["$lang"]="PASS"
            ((PASSED_TESTS++))
        else
            echo -e "${RED}[$lang]${NC} ${TEST_SUITES[$lang]} failed"
            TEST_RESULTS["$lang"]="FAIL"
            ((FAILED_TESTS++))
        fi

        ((COMPLETED_TESTS++))
        print_progress "$COMPLETED_TESTS" "$TOTAL_TESTS"

        # Release semaphore slot
        echo >&3
    } &

    # Store the PID and log file
    TEST_PIDS["$lang"]=$!
    TEST_LOG_FILES["$lang"]="$log_file"
}

# Print detailed results
print_results() {
    echo -e "\n\n${BLUE}========================================${NC}"
    echo -e "${BLUE}  Test Results Summary${NC}"
    echo -e "${BLUE}========================================${NC}"

    local end_time=$(date +%s)
    local duration=$((end_time - START_TIME))

    echo -e "Duration: ${duration} seconds"
    echo -e "Total Tests: ${TOTAL_TESTS}"
    echo -e "Passed: ${PASSED_TESTS}"
    echo -e "Failed: ${FAILED_TESTS}"
    echo -e "Skipped: ${SKIPPED_TESTS}"

    echo -e "\n${BLUE}Detailed Results:${NC}"
    for lang in "${!TEST_SUITES[@]}"; do
        local result="${TEST_RESULTS[$lang]:-UNKNOWN}"
        local log_file="${TEST_LOG_FILES[$lang]}"

        case "$result" in
        "PASS")
            echo -e "  ${GREEN}✓${NC} $lang: ${TEST_SUITES[$lang]}"
            ;;
        "FAIL")
            echo -e "  ${RED}✗${NC} $lang: ${TEST_SUITES[$lang]}"
            echo -e "    Log: $log_file"
            ;;
        *)
            echo -e "  ${YELLOW}?${NC} $lang: ${TEST_SUITES[$lang]} (Unknown status)"
            ;;
        esac
    done

    echo -e "\n${BLUE}Log Files:${NC}"
    for lang in "${!TEST_SUITES[@]}"; do
        local log_file="${TEST_LOG_FILES[$lang]}"
        if [ -f "$log_file" ]; then
            echo -e "  $lang: $log_file"
        fi
    done

    # Overall result
    if [ $FAILED_TESTS -eq 0 ]; then
        echo -e "\n${GREEN}========================================${NC}"
        echo -e "${GREEN}  All tests passed! 🎉${NC}"
        echo -e "${GREEN}========================================${NC}"
        exit 0
    else
        echo -e "\n${RED}========================================${NC}"
        echo -e "${RED}  Some tests failed! ❌${NC}"
        echo -e "${RED}========================================${NC}"
        exit 1
    fi
}

# Check if test scripts exist
check_test_scripts() {
    echo -e "${BLUE}[Parallel Test Runner]${NC} Checking test scripts..."

    local missing_scripts=()
    for lang in "${!TEST_SCRIPTS[@]}"; do
        local script_path="${PROJECT_ROOT_DIR}/${TEST_SCRIPTS[$lang]}"
        if [ ! -f "$script_path" ]; then
            missing_scripts+=("$script_path")
        fi
    done

    if [ ${#missing_scripts[@]} -gt 0 ]; then
        echo -e "${RED}Error:${NC} Missing test scripts:"
        for script in "${missing_scripts[@]}"; do
            echo -e "  - $script"
        done
        exit 1
    fi

    echo -e "${GREEN}✓${NC} All test scripts found"
}

# Main execution
main() {
    # Set up cleanup trap
    trap cleanup EXIT INT TERM

    # Print header
    print_header

    # Check test scripts
    check_test_scripts

    # Setup semaphore for parallel execution
    setup_semaphore

    echo -e "${BLUE}[Parallel Test Runner]${NC} Starting parallel test execution..."
    echo -e "${BLUE}[Parallel Test Runner]${NC} Running ${TOTAL_TESTS} test suites with ${PARALLEL_JOBS} parallel jobs"
    echo

    # Start all test suites
    for lang in "${!TEST_SUITES[@]}"; do
        run_test_suite "$lang"
    done

    # Wait for all background processes to complete
    echo -e "\n${BLUE}[Parallel Test Runner]${NC} Waiting for all tests to complete..."
    for pid in "${TEST_PIDS[@]}"; do
        wait "$pid" 2>/dev/null || true
    done

    # Print final results
    print_results
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
    --jobs | -j)
        PARALLEL_JOBS="$2"
        shift 2
        ;;
    --no-clean)
        CLEAN_BUILD=false
        shift
        ;;
    --help | -h)
        echo "Usage: $0 [options]"
        echo "Options:"
        echo "  --jobs, -j <number>    Number of parallel jobs (default: 4)"
        echo "  --no-clean             Skip clean builds"
        echo "  --help, -h            Show this help message"
        exit 0
        ;;
    *)
        echo "Unknown option: $1"
        echo "Use --help for usage information"
        exit 1
        ;;
    esac
done

# Validate parallel jobs
if ! [[ "$PARALLEL_JOBS" =~ ^[0-9]+$ ]] || [ "$PARALLEL_JOBS" -lt 1 ]; then
    echo -e "${RED}Error:${NC} Invalid number of parallel jobs: $PARALLEL_JOBS"
    exit 1
fi

# Run main function
main "$@"
