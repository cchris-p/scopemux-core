# ScopeMux Parallel Test Runners

This directory contains scripts to run all language tests in parallel for the ScopeMux project.

## Available Scripts

### 1. `run_all_lang_tests.sh` - Comprehensive Parallel Test Runner

**Features:**
- Parallel execution with configurable job limits
- Individual log files for each language
- Comprehensive summary reporting with progress bars
- Color-coded output
- Error handling and cleanup
- Duration tracking
- Detailed results with log file locations

**Usage:**
```bash
# Run with default settings (4 parallel jobs, clean builds)
./run_all_lang_tests.sh

# Run with custom number of parallel jobs
./run_all_lang_tests.sh --jobs 6

# Skip clean builds for faster execution
./run_all_lang_tests.sh --no-clean

# Show help
./run_all_lang_tests.sh --help
```

**Output:**
- Progress bar showing completion percentage
- Individual log files: `run_c_tests_parallel.log`, `run_cpp_tests_parallel.log`, etc.
- Comprehensive summary with pass/fail status for each language
- Duration and statistics

### 2. `run_all_lang_tests_simple.sh` - Simple Parallel Test Runner

**Features:**
- Simplified parallel execution
- Basic logging and reporting
- Color-coded output
- Error handling and cleanup
- Faster execution with minimal overhead

**Usage:**
```bash
# Run with default settings (2 parallel jobs)
./run_all_lang_tests_simple.sh

# Run with custom number of parallel jobs
./run_all_lang_tests_simple.sh --jobs 4

# Show help
./run_all_lang_tests_simple.sh --help
```

**Output:**
- Simple status messages for each language
- Individual log files: `parallel_c_tests.log`, `parallel_cpp_tests.log`, etc.
- Basic summary with pass/fail status

## Test Languages Covered

Both scripts run tests for the following languages:
- **C** (`run_c_tests.sh`)
- **C++** (`run_cpp_tests.sh`)
- **Python** (`run_python_tests.sh`)
- **JavaScript** (`run_js_tests.sh`)
- **TypeScript** (`run_ts_tests.sh`)

## Parallel Execution

The scripts use a semaphore-based approach to limit concurrent execution:
- Each language test runs in its own process
- Configurable job limits prevent system overload
- Individual log files prevent output mixing
- Proper cleanup ensures no orphaned processes

## Log Files

Each language test generates its own log file:
- **Comprehensive runner**: `run_{lang}_tests_parallel.log`
- **Simple runner**: `parallel_{lang}_tests.log`

These files contain the complete output from each test suite for debugging purposes.

## Exit Codes

- **0**: All tests passed
- **1**: One or more tests failed

## Recommendations

- **For CI/CD**: Use `run_all_lang_tests.sh` for comprehensive reporting
- **For development**: Use `run_all_lang_tests_simple.sh` for faster feedback
- **For debugging**: Check individual log files for detailed error information
- **For performance**: Use `--no-clean` flag to skip clean builds when appropriate

## Example Usage

```bash
# Quick test run during development
./run_all_lang_tests_simple.sh --jobs 3

# Comprehensive test run for CI
./run_all_lang_tests.sh --jobs 4

# Fast test run without clean builds
./run_all_lang_tests.sh --jobs 6 --no-clean
``` 