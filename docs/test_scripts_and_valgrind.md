# Test Scripts and Direct Binary Commands

This document explains the main test scripts in ScopeMux and the corresponding binary executables they invoke. It also includes an example for running tests under Valgrind for memory diagnostics.

---

## Test Scripts Overview

### `run_interfile_tests.sh`
Runs all inter-file and project context integration tests. It builds the project and then invokes the following test binaries:

```sh
./build/core/tests/project_context_tests
./build/core/tests/interfile_symbols_tests
./build/core/tests/delegation_tests
./build/core/tests/cross_file_resolution_tests
./build/core/tests/ast_query_integration_tests
```
- These binaries are rebuilt each time the script is run.
- Paths are relative to the project root after building.

---

## Test Runner Scripts and Their Binaries

### `run_interfile_tests.sh`
Runs all inter-file and project context integration tests. Invokes:
```sh
./build-interfile/core/tests/project_context_tests
./build-interfile/core/tests/symbol_table_tests
```

### `run_c_tests.sh`
Runs the main C test suite (unit and integration tests for C core):
```sh
./build-c/core/tests/c_basic_ast_tests
```

### `run_cpp_tests.sh`
Runs the C++ test suite:
```sh
./build-cpp/core/tests/cpp_basic_ast_tests
./build-cpp/core/tests/cpp_example_ast_tests
```

### `run_js_tests.sh`
Runs the JavaScript test suite. (No test binaries currently found in build-js/core/tests.)

### `run_misc_tests.sh`
Runs miscellaneous and basic parser tests:
```sh
./build-misc/core/tests/init_parser_tests
```

### `run_python_tests.sh`
Runs the Python test suite. (No test binaries currently found in build-python/core/tests.)

### `run_ts_tests.sh`
Runs the TypeScript test suite. (No test binaries currently found in build-ts/core/tests.)

---

---

## Running Binaries Directly
You can invoke any test binary directly for focused debugging or to run under tools like Valgrind. Example:

```sh
./build/core/tests/project_context_tests
```

---

## Using Valgrind for Memory Diagnostics
To check for memory leaks or invalid frees in a test binary, run:

```sh
valgrind --leak-check=full --show-leak-kinds=all ./build/core/tests/project_context_tests
```
- Replace the binary with any other test executable as needed.
- Valgrind will print detailed memory usage and leak reports to the terminal.

---

## Notes
- Always run test scripts from the project root directory.
- Scripts will automatically clean and rebuild the project before running tests.
- For more details on each script, see comments at the top of each `run_*.sh` file.
