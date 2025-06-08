# Cross-Environment Testing for ScopeMux

This document outlines the testing strategy for the ScopeMux project, covering both its C core libraries and the Python bindings (`scopemux_core`) generated via Pybind11. The goal is to ensure the robustness of the C-native implementation and the correctness of its exposure to Python.

## Overview

Testing in ScopeMux is approached in two main layers:

1.  **C Core Testing (Criterion):** The underlying C libraries (parsers, context engine, common utilities) are tested directly using the [Criterion](https://criterion.readthedocs.io/) testing framework. This allows for fine-grained unit and integration tests of the C functions.
2.  **Python Bindings Testing:** The `scopemux_core` Python module, which wraps the C core, is tested from Python using standard Python testing tools (e.g., `pytest`). This validates the binding layer and the end-to-end functionality as a Python user would experience it.

## 1. Testing the C Core with Criterion

The C core components are tested using C test drivers built with the Criterion framework.

*   **Location of C Test Sources:**
    *   Main C test files (e.g., `c_parser_tests.c`) are located in `core/tests/src/`.
    *   Test utilities (e.g., `test_utils.c`, `test_utils.h`) can be found in `core/tests/utilities/`.
*   **Sample Data:**
    *   Sample input code files (e.g., `.c`, `.py` snippets for parsing tests) are in `core/tests/sample_code/`.
    *   Corresponding expected output files (e.g., JSON ASTs) are in `core/tests/expected_output/`.
*   **CMake Configuration:**
    *   The C tests are defined and built via `core/tests/CMakeLists.txt`. This file configures Criterion, lists test source files, and links against necessary libraries (like the core ScopeMux libraries and Tree-sitter).
    *   The primary test executable target is typically named `scopemux_c_parser_tests` (or similar, as defined in `core/tests/CMakeLists.txt`).

### Adding a New C Test (Criterion)

1.  **Open the relevant C test file** in `core/tests/src/` (e.g., `c_parser_tests.c`).
2.  **Include necessary headers:**
    ```c
    #include <criterion/criterion.h>
    #include <criterion/logging.h>
    // Your project's C headers
    #include "scopemux/parser.h" // Example
    // Test utilities
    #include "../utilities/test_utils.h" // Example
    ```
3.  **Write your test case using Criterion's `Test` macro:**
    ```c
    Test(suite_name, test_name, .init = setup_function, .fini = teardown_function) {
        // Arrange: Set up test data, load sample files
        // Act: Call the C function(s) you want to test
        // Assert: Use cr_assert macros to check conditions
        // Example: cr_assert_str_eq(actual_output, expected_output, "Output mismatch.");
        //          cr_assert_not_null(pointer, "Pointer was NULL.");
    }
    ```
    *   Replace `suite_name` and `test_name` appropriately.
    *   `setup_function` and `teardown_function` are optional.
    *   Refer to the Criterion documentation for various assertion macros and features.
4.  **Update `core/tests/CMakeLists.txt` (Rarely Needed for New Tests in Existing Files):**
    *   If you add a test to an *existing* `.c` file that is already part of a Criterion test executable target (like `scopemux_c_parser_tests`), you typically **do not** need to change the `CMakeLists.txt`.
    *   If you create a *new* `.c` file for tests or a new test executable, you'll need to add it to `core/tests/CMakeLists.txt`.

### Building C Tests

1.  Navigate to your CMake build directory (e.g., `build/` from the project root).
2.  Configure CMake (if new files were added to `CMakeLists.txt` or it's the first build):
    ```bash
    cd /path/to/scopemux/build
    cmake ..
    ```
3.  Compile the specific test target (e.g., `scopemux_c_parser_tests`) or all targets:
    ```bash
    make scopemux_c_parser_tests
    # or
    make
    ```
    The executable will be located in a path like `build/core/tests/scopemux_c_parser_tests`.

### Running C Tests

*   **Directly:**
    Execute the compiled test binary from the build directory:
    ```bash
    ./build/core/tests/scopemux_c_parser_tests
    ```
*   **Via the `run_c_bindings_tests.sh` script:**
    The project includes a script `/home/matrillo/apps/scopemux/run_c_bindings_tests.sh` designed to automate building and running C tests.
    ```bash
    /home/matrillo/apps/scopemux/run_c_bindings_tests.sh
    ```
    **Note:** Ensure the paths within `run_c_bindings_tests.sh` (like `C_TEST_EXECUTABLE_RELPATH`) correctly point to the C test executable's location (e.g., `core/tests/scopemux_c_parser_tests` relative to the build directory). If the script uses paths like `bindings/tests/...`, it might need updating to align with the `core/tests/` structure.

#### Adding a New Test to the Script

The `run_c_bindings_tests.sh` script uses toggle variables to control which tests are executed. When adding a new test executable, follow these steps to update the script:

1. **Add a Toggle Variable** at the top of the script in the "Test Case Toggles" section:
   ```bash
   # Test Case Toggles
   RUN_C_PARSER_CRITERION_TESTS=true
   RUN_MY_NEW_TEST=true  # New toggle for your test
   ```

2. **Add an Execution Block** at the bottom of the script in the "Run C Tests" section:
   ```bash
   # --- Run C Tests ---
   if [ "${RUN_MY_NEW_TEST}" = true ]; then
       MY_TEST_EXECUTABLE_RELPATH="core/tests/my_new_test_executable"
       MY_TEST_EXECUTABLE_FULL_PATH="${CMAKE_PROJECT_BUILD_DIR}/${MY_TEST_EXECUTABLE_RELPATH}"
       run_criterion_test_executable "My New Test Suite" \
           "${MY_TEST_EXECUTABLE_FULL_PATH}"
   fi
   ```

3. **Ensure Build Target** is included in the CMake build section if your test requires special handling beyond the standard targets:
   ```bash
   echo "Building C test target: my_new_test_executable..."
   make "my_new_test_executable"
   ```
   Note: This step is only necessary if your test isn't built by the default target or requires special build steps.

## 2. Testing the Python Bindings (`scopemux_core`)

The Python bindings are tested using Python's own testing ecosystem. `pytest` is recommended.

*   **Location of Python Test Files:**
    A dedicated directory like `core/tests/python/` or a top-level `tests/python/` is recommended for Python test scripts (e.g., `test_parser_bindings.py`).
*   **Sample Data:** Python tests can also use sample files from `core/tests/sample_code/` and `core/tests/expected_output/`.

### Adding a New Python Test (Example with `pytest`)

1.  **Create a Python test file** (e.g., `core/tests/python/test_my_feature.py`).
2.  **Write test functions:**
    ```python
    import scopemux_core
    import pytest # If using pytest

    def test_some_functionality():
        # Arrange: Prepare input data
        sample_code = "int main() { return 0; }"
        
        # Act: Call the function from the scopemux_core module
        result = scopemux_core.parse_string(sample_code, "c") # Example function
        
        # Assert: Check the result
        assert result is not None
        assert "ast" in result # Example assertion
        # Add more detailed assertions based on expected output
    
    # Add more test functions as needed
    ```
3.  **Ensure `scopemux_core` is importable:**
    Before running Python tests, the `scopemux_core` module must be built and available in your Python environment.
    *   For development, build in-place from the `core/` directory (where `setup.py` is located):
        ```bash
        cd /path/to/scopemux/core
        python setup.py build_ext --inplace
        ```
    *   Alternatively, install in editable mode:
        ```bash
        cd /path/to/scopemux/core
        pip install -e .
        ```

### Running Python Tests

1.  Install `pytest` if you haven't already: `pip install pytest`.
2.  Run `pytest` from the project root or the `core/` directory, pointing it to your Python test directory:
    ```bash
    # From project root, if tests are in core/tests/python/
    pytest core/tests/python/
    ```

## Overall Test Strategy

*   **C Criterion Tests:** Focus on unit testing individual C functions, internal data structures, and the logic within C modules. They ensure the core components are correct in isolation.
*   **Python Tests:** Focus on integration testing of the `scopemux_core` Python module. They verify that:
    *   The Pybind11 bindings correctly expose C functionality.
    *   Data is correctly marshaled between Python and C.
    *   The module behaves as expected from a Python user's perspective.
    *   End-to-end use cases involving multiple calls to the module work correctly.

By combining these two layers of testing, we can achieve comprehensive coverage and maintain a high level of quality for the ScopeMux project.
