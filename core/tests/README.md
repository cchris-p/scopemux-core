# Testing Guide for Scopemux Python Bindings

This document outlines how to add new C++-based tests to validate the `scopemux_core` Python bindings generated via Pybind11. These tests call the C functions as they are exposed to Python, ensuring the binding layer and the underlying C logic work correctly together from a Python perspective.

## Overview

The testing process involves:
1.  Creating a C++ test driver.
2.  Providing sample input code and its expected output (e.g., an AST in JSON format).
3.  Updating the CMake configuration to build the new test driver.
4.  Adding a toggle and execution logic for the new test in the main test script (`run_c_bindings_tests.sh`).

## 1. Creating a New C++ Test Driver

Test drivers are C++ executables that use the functionality exposed by the `scopemux_core` bindings.

*   **Location:** Create your new C++ test source file in `bindings/tests/src/` (e.g., `test_my_new_feature.cpp`).
*   **Structure:**
    ```cpp
    #include <iostream>
    #include <fstream>
    #include <string>
    #include <vector>

    // 1. Include necessary headers from your scopemux_core bindings.
    //    (These would be the C headers that Pybind11 wraps, or if you are testing
    //     the Python module directly from C++, you'd need to embed Python).
    //    For simplicity, current tests assume you are calling underlying C functions
    //    that the bindings would also call, or testing utilities.
    // #include "scopemux.h" // Example: if your core C functions are here

    // 2. Include test utilities if needed.
    // #include "../utilities/test_utils.h"

    int main(int argc, char* argv[]) {
        if (argc < 3) {
            std::cerr << "Usage: " << argv[0] << " <sample_code_path> <expected_json_path>" << std::endl;
            return 1; // Indicate failure
        }

        std::string sample_code_path = argv[1];
        std::string expected_json_path = argv[2];

        std::cout << "Running My New Feature Test (Placeholder)" << std::endl;
        std::cout << "  Sample Code: " << sample_code_path << std::endl;
        std::cout << "  Expected JSON: " << expected_json_path << std::endl;

        // TODO: Implement your test logic:
        // a. Read the sample code from sample_code_path.
        // b. Call the relevant functions (from your C library that is bound by Pybind11)
        //    to process the sample code and generate an actual output (e.g., AST JSON).
        // c. Read the expected output from expected_json_path.
        // d. Compare the actual output with the expected output.
        //    (Consider using helper functions from test_utils for this).
        // e. Print a clear PASS or FAIL message to stdout.

        bool test_passed = false; // Replace with actual test result

        if (test_passed) {
            std::cout << "[TEST_RESULT] My New Feature: PASS" << std::endl;
            return 0; // Indicate success
        } else {
            std::cout << "[TEST_RESULT] My New Feature: FAIL" << std::endl;
            return 1; // Indicate failure
        }
    }
    ```

## 2. Creating Sample Code and Expected Output

*   **Sample Input Code:** Place your sample input files (e.g., `.cpp`, `.py` snippets) in the appropriate subdirectory under `bindings/tests/sample_code/` (e.g., `bindings/tests/sample_code/cpp/my_new_sample.cpp`).
*   **Expected Output:** Place the corresponding expected output files (e.g., JSON ASTs) in `bindings/tests/expected_output/` using the same subdirectory structure and naming convention (e.g., `bindings/tests/expected_output/cpp/my_new_sample.cpp.expected.json`).

## 3. Updating `bindings/tests/CMakeLists.txt`

To compile your new test driver, add an entry to `bindings/tests/CMakeLists.txt`:

```cmake
# ... other CMake configurations ...

# Test for My New Feature
add_executable(test_my_new_feature src/test_my_new_feature.cpp utilities/test_utils.c) # Add other sources if needed
target_link_libraries(test_my_new_feature PRIVATE
    # Link against the C libraries that Pybind11 wraps (e.g., parser_core, context_engine)
    # These are the same libraries your scopemux_core Python module uses.
    # Ensure these library targets are defined in the parent CMakeLists.txt or imported.
    # Example (actual names might differ based on your main CMakeLists.txt):
    # parser_core 
    # context_engine
    # common_utils

    # Link against tree-sitter runtime and specific language parsers if your test calls them directly
    # tree-sitter 
    # tree-sitter-cpp
)
```
**Note:** The C++ test drivers link against the underlying C libraries directly, testing the same components that the Pybind11 bindings wrap.

## 4. Updating `run_c_bindings_tests.sh`

The main test script is `/home/matrillo/apps/scopemux/run_c_bindings_tests.sh`. To add your new test:

1.  **Add a Toggle Variable:** At the top of the script, add a new boolean variable to enable/disable your test:
    ```bash
    # Test Case Toggles
    RUN_CPP_EXAMPLE1_TEST=true
    RUN_PYTHON_EXAMPLE1_TEST=true
    RUN_MY_NEW_FEATURE_TEST=true # New toggle
    ```

2.  **Add Test Execution Logic:** Further down in the script, add an `if` block to call the `run_test` function:
    ```bash
    if [ "${RUN_MY_NEW_FEATURE_TEST}" = true ]; then
        run_test "My New Feature Test (my_new_sample.cpp)" \
            "${TEST_BUILD_DIR}/test_my_new_feature" \
            "${CPP_SAMPLE_DIR}/my_new_sample.cpp" \
            "${CPP_EXPECTED_DIR}/my_new_sample.cpp.expected.json"
    fi
    ```
    *   Adjust the test name, executable name, sample file path, and expected file path accordingly.
    *   Ensure `TEST_BUILD_DIR` in the script correctly points to where CMake places the compiled test executables.

## 5. Building and Running Tests

1.  **Build:**
    *   Navigate to your CMake build directory (e.g., `/home/matrillo/apps/scopemux/build`).
    *   Run CMake to configure (if it's a new file or `CMakeLists.txt` changed): `cmake ..` (adjust path to source if needed).
    *   Compile the tests: `make test_my_new_feature` (or `make all`).
2.  **Run:**
    *   Execute the main test script from the project root: `/home/matrillo/apps/scopemux/run_c_bindings_tests.sh`.

## Clarification on Testing Scope

These C++ test drivers primarily serve to test the functionality of your C libraries that are *intended to be wrapped* by Pybind11 for the `scopemux_core` Python module. While they are C++ executables, they help ensure that the C components behave as expected before and after being exposed to Python. They are an important part of verifying the integrity of the code that the Python bindings will rely upon.
