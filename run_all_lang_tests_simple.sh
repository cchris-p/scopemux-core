#!/bin/bash

# ScopeMux Simple All Language Tests Runner
# Executes all language tests in parallel with basic logging

set -euo pipefail

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT_DIR="$SCRIPT_DIR"
PARALLEL_JOBS=${PARALLEL_JOBS:-2} # Default to 2 parallel jobs for safety

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Test suites
declare -A TEST_SUITES=(
    ["c"]="C Language Tests"
    ["cpp"]="C++ Language Tests"
    ["python"]="Python Language Tests"
    ["js"]="JavaScript Language Tests"
    ["ts"]="TypeScript Language Tests"
)

# Test scripts
declare -A TEST_SCRIPTS=(
    ["c"]="run_c_tests.sh"
    ["cpp"]="run_cpp_tests.sh"
    ["python"]="run_python_tests.sh"
    ["js"]="run_js_tests.sh"
    ["ts"]="run_ts_tests.sh"
)

# Global variables
declare -A TEST_RESULTS
declare -A TEST_PIDS
declare -A TEST_LOG_FILES
TOTAL_TESTS=${#TEST_SUITES[@]}
COMPLETED_TESTS=0
FAILED_TESTS=0
PASSED_TESTS=0

# Temporary directory
TMP_DIR=$(mktemp -d)
SEMAPHORE="$TMP_DIR/semaphore"

# Cleanup function
cleanup() {
    echo -e "\n${BLUE}[Parallel Runner]${NC} Cleaning up..."
    rm -rf "$TMP_DIR"

    # Kill background processes
    for pid in "${TEST_PIDS[@]}"; do
        if kill -0 "$pid" 2>/dev/null; then
            kill -TERM "$pid" 2>/dev/null || true
        fi
    done
    wait 2>/dev/null || true
}

# Setup semaphore
setup_semaphore() {
    mkfifo "$SEMAPHORE"
    exec 3<>"$SEMAPHORE"
    rm -f "$SEMAPHORE"

    for ((i = 1; i <= PARALLEL_JOBS; i++)); do
        echo >&3
    done
}

# Run a single test suite
run_test_suite() {
    local lang="$1"
    local script_name="${TEST_SCRIPTS[$lang]}"
    local script_path="${PROJECT_ROOT_DIR}/${script_name}"
    local log_file="${PROJECT_ROOT_DIR}/parallel_${lang}_tests.log"

    # Wait for semaphore slot
    read -u 3

    {
        echo -e "${BLUE}[$lang]${NC} Starting ${TEST_SUITES[$lang]}..."

        # Set environment
        export PROJECT_ROOT_DIR
        export PARALLEL_JOBS=1
        export CLEAN_BUILD=true

        # Run test script
        if bash "$script_path" >"$log_file" 2>&1; then
            echo -e "${GREEN}[$lang]${NC} ${TEST_SUITES[$lang]} PASSED"
            TEST_RESULTS["$lang"]="PASS"
            ((PASSED_TESTS++))
        else
            echo -e "${RED}[$lang]${NC} ${TEST_SUITES[$lang]} FAILED"
            TEST_RESULTS["$lang"]="FAIL"
            ((FAILED_TESTS++))
        fi

        ((COMPLETED_TESTS++))

        # Release semaphore
        echo >&3
    } &

    TEST_PIDS["$lang"]=$!
    TEST_LOG_FILES["$lang"]="$log_file"
}

# Print results
print_results() {
    echo -e "\n${BLUE}========================================${NC}"
    echo -e "${BLUE}  Test Results Summary${NC}"
    echo -e "${BLUE}========================================${NC}"

    echo -e "Total Tests: ${TOTAL_TESTS}"
    echo -e "Passed: ${PASSED_TESTS}"
    echo -e "Failed: ${FAILED_TESTS}"

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
            echo -e "  ${YELLOW}?${NC} $lang: ${TEST_SUITES[$lang]} (Unknown)"
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

    if [ $FAILED_TESTS -eq 0 ]; then
        echo -e "\n${GREEN}All tests passed! 🎉${NC}"
        exit 0
    else
        echo -e "\n${RED}Some tests failed! ❌${NC}"
        exit 1
    fi
}

# Check test scripts
check_test_scripts() {
    echo -e "${BLUE}[Parallel Runner]${NC} Checking test scripts..."

    local missing=()
    for lang in "${!TEST_SCRIPTS[@]}"; do
        local script_path="${PROJECT_ROOT_DIR}/${TEST_SCRIPTS[$lang]}"
        if [ ! -f "$script_path" ]; then
            missing+=("$script_path")
        fi
    done

    if [ ${#missing[@]} -gt 0 ]; then
        echo -e "${RED}Error:${NC} Missing test scripts:"
        for script in "${missing[@]}"; do
            echo -e "  - $script"
        done
        exit 1
    fi

    echo -e "${GREEN}✓${NC} All test scripts found"
}

# Main function
main() {
    trap cleanup EXIT INT TERM

    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}  ScopeMux All Language Tests Runner${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo -e "Project Root: ${PROJECT_ROOT_DIR}"
    echo -e "Parallel Jobs: ${PARALLEL_JOBS}"
    echo -e "Total Tests: ${TOTAL_TESTS}"
    echo -e "${BLUE}========================================${NC}\n"

    check_test_scripts
    setup_semaphore

    echo -e "${BLUE}[Parallel Runner]${NC} Starting parallel execution..."

    # Start all test suites
    for lang in "${!TEST_SUITES[@]}"; do
        run_test_suite "$lang"
    done

    # Wait for completion
    echo -e "${BLUE}[Parallel Runner]${NC} Waiting for all tests to complete..."
    for pid in "${TEST_PIDS[@]}"; do
        wait "$pid" 2>/dev/null || true
    done

    print_results
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
    --jobs | -j)
        PARALLEL_JOBS="$2"
        shift 2
        ;;
    --help | -h)
        echo "Usage: $0 [options]"
        echo "Options:"
        echo "  --jobs, -j <number>    Number of parallel jobs (default: 2)"
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

main "$@"
