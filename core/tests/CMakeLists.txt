# Configuration for ScopeMux Bindings C Tests
# C/C++ standards are set in the root CMakeLists.txt

# Find PkgConfig for locating Criterion via .pc file
find_package(PkgConfig REQUIRED)
# Use pkg-config to find Criterion
pkg_check_modules(Criterion REQUIRED criterion)

# Include directories
include_directories(
    # Project directories
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/scopemux
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/utilities
    # Tree-sitter include directories (from root CMakeLists.txt)
    ${TS_CORE_INCLUDE_DIR}
    ${TS_C_INCLUDE_DIR}
    ${TS_CPP_INCLUDE_DIR}
    ${TS_PYTHON_INCLUDE_DIR}
    ${TS_JS_INCLUDE_DIR}
    ${TS_TS_INCLUDE_DIR}
    # Criterion
    ${Criterion_INCLUDE_DIRS}
)

# --- Test Utilities and Common Test Helpers ---
set(TEST_UTIL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/utilities/test_utils.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common/test_helpers.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common/json_validation.c
)

# --- Criterion Test Executable ---

# Define the core C source files from the main project needed for these tests
# These are compiled directly into the test executable.
set(SCOPEMUX_CORE_C_SOURCES_FOR_TESTS
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/parser.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/tree_sitter_integration.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/tree_sitter_parser.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/query_manager.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/ir_generator.c  # Commented out missing file
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/common/error_handling.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/common/logging.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/common/memory_management.c
    # Tree-sitter core and grammar sources are now linked as libraries
)

# --- Language-specific Test Sources ---
set(C_BASIC_AST_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/c/c_basic_ast_tests.c
)

set(PYTHON_BASIC_AST_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/python/python_basic_ast_tests.c
)

set(CPP_BASIC_AST_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp/cpp_basic_ast_tests.c
)

set(C_EXAMPLE_AST_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/c/c_example_ast_tests.c
)

set(PYTHON_EXAMPLE_AST_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/python/python_example_ast_tests.c
)

set(CPP_EXAMPLE_AST_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp/cpp_example_ast_tests.c
)

# JavaScript and TypeScript test sources
set(JS_BASIC_AST_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/js/js_basic_ast_tests.c
)

set(TS_BASIC_AST_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ts/ts_basic_ast_tests.c
)

set(JS_EXAMPLE_AST_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/js/js_example_ast_tests.c
)

set(TS_EXAMPLE_AST_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ts/ts_example_ast_tests.c
)

# --- Common Test Sources ---
# (Previously contained ast_extraction_tests.c which has been migrated to language-specific files)

# --- Edge Case Tests ---
set(EDGE_CASE_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/edge_case_tests.c
)

# --- Init Parser Tests ---
set(INIT_PARSER_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/init_parser_tests.c
)

# Define executables
add_executable(init_parser_tests 
    ${INIT_PARSER_TEST_SOURCES}
    ${TEST_UTIL_SOURCES}
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)

# Edge Case tests executable
add_executable(edge_case_tests
    ${EDGE_CASE_TEST_SOURCES}
    ${TEST_UTIL_SOURCES}
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)

# C Basic AST tests executable
add_executable(c_basic_ast_tests
    ${C_BASIC_AST_TEST_SOURCES}
    ${TEST_UTIL_SOURCES}
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)

# Python Basic AST tests executable
add_executable(python_basic_ast_tests
    ${PYTHON_BASIC_AST_TEST_SOURCES}
    ${TEST_UTIL_SOURCES}
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)

# C++ Basic AST tests executable
add_executable(cpp_basic_ast_tests
    ${CPP_BASIC_AST_TEST_SOURCES}
    ${TEST_UTIL_SOURCES}
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)

# Example AST test executables
add_executable(c_example_ast_tests
    ${C_EXAMPLE_AST_TEST_SOURCES}
    ${TEST_UTIL_SOURCES}
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)

add_executable(python_example_ast_tests
    ${PYTHON_EXAMPLE_AST_TEST_SOURCES}
    ${TEST_UTIL_SOURCES}
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)

add_executable(cpp_example_ast_tests
    ${CPP_EXAMPLE_AST_TEST_SOURCES}
    ${TEST_UTIL_SOURCES}
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)

# JavaScript and TypeScript test executables
add_executable(js_basic_ast_tests
    ${JS_BASIC_AST_TEST_SOURCES}
    ${TEST_UTIL_SOURCES}
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)

add_executable(ts_basic_ast_tests
    ${TS_BASIC_AST_TEST_SOURCES}
    ${TEST_UTIL_SOURCES}
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)

add_executable(js_example_ast_tests
    ${JS_EXAMPLE_AST_TEST_SOURCES}
    ${TEST_UTIL_SOURCES}
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)

add_executable(ts_example_ast_tests
    ${TS_EXAMPLE_AST_TEST_SOURCES}
    ${TEST_UTIL_SOURCES}
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)

# Link against Criterion and your project's C libraries
target_link_libraries(init_parser_tests PRIVATE
    ${Criterion_LIBRARIES} # Provided by pkg_check_modules
    parser_core
    m # Math library, often needed
    dl # Dynamic linking library, sometimes needed by test frameworks or tree-sitter
    # Tree-sitter imported targets from root CMake
    ts_core
    ts_c
    ts_cpp
    ts_python
    ts_javascript
    ts_typescript
)

# Link C Basic AST tests
target_link_libraries(c_basic_ast_tests PRIVATE
    ${Criterion_LIBRARIES}
    parser_core
    m
    dl
    # Tree-sitter imported targets from root CMake
    ts_core
    ts_c
    ts_cpp
    ts_python
    ts_javascript
    ts_typescript
)

# Link Python Basic AST tests
target_link_libraries(python_basic_ast_tests PRIVATE
    ${Criterion_LIBRARIES}
    parser_core
    m
    dl
    # Tree-sitter imported targets from root CMake
    ts_core
    ts_c
    ts_cpp
    ts_python
    ts_javascript
    ts_typescript
)

# Link C++ Basic AST tests
target_link_libraries(cpp_basic_ast_tests PRIVATE
    ${Criterion_LIBRARIES}
    parser_core
    m
    dl
    # Tree-sitter imported targets from root CMake
    ts_core
    ts_c
    ts_cpp
    ts_python
    ts_javascript
    ts_typescript
)

# Link Example AST tests
target_link_libraries(c_example_ast_tests PRIVATE
    ${Criterion_LIBRARIES}
    parser_core
    m
    dl
    # Tree-sitter imported targets from root CMake
    ts_core
    ts_c
    ts_cpp
    ts_python
    ts_javascript
    ts_typescript
)

target_link_libraries(python_example_ast_tests PRIVATE
    ${Criterion_LIBRARIES}
    parser_core
    m
    dl
    # Tree-sitter imported targets from root CMake
    ts_core
    ts_c
    ts_cpp
    ts_python
    ts_javascript
    ts_typescript
)

target_link_libraries(cpp_example_ast_tests PRIVATE
    ${Criterion_LIBRARIES}
    parser_core
    m
    dl
    # Tree-sitter imported targets from root CMake
    ts_core
    ts_c
    ts_cpp
    ts_python
    ts_javascript
    ts_typescript
)

# Link JavaScript and TypeScript tests
target_link_libraries(js_basic_ast_tests PRIVATE
    ${Criterion_LIBRARIES}
    parser_core
    m
    dl
    # Tree-sitter imported targets from root CMake
    ts_core
    ts_c
    ts_cpp
    ts_python
    ts_javascript
    ts_typescript
)

target_link_libraries(ts_basic_ast_tests PRIVATE
    ${Criterion_LIBRARIES}
    parser_core
    m
    dl
    # Tree-sitter imported targets from root CMake
    ts_core
    ts_c
    ts_cpp
    ts_python
    ts_javascript
    ts_typescript
)

target_link_libraries(js_example_ast_tests PRIVATE
    ${Criterion_LIBRARIES}
    parser_core
    m
    dl
    # Tree-sitter imported targets from root CMake
    ts_core
    ts_c
    ts_cpp
    ts_python
    ts_javascript
    ts_typescript
)

target_link_libraries(ts_example_ast_tests PRIVATE
    ${Criterion_LIBRARIES}
    parser_core
    m
    dl
    # Tree-sitter imported targets from root CMake
    ts_core
    ts_c
    ts_cpp
    ts_python
    ts_javascript
    ts_typescript
)

# Add tests to CTest 
add_test(NAME InitParserTests COMMAND init_parser_tests)
add_test(NAME CBasicASTTests COMMAND c_basic_ast_tests)
add_test(NAME PythonBasicASTTests COMMAND python_basic_ast_tests)
add_test(NAME CPPBasicASTTests COMMAND cpp_basic_ast_tests)
add_test(NAME CExampleASTTests COMMAND c_example_ast_tests)
add_test(NAME PythonExampleASTTests COMMAND python_example_ast_tests)
add_test(NAME CPPExampleASTTests COMMAND cpp_example_ast_tests)
add_test(NAME JSBasicASTTests COMMAND js_basic_ast_tests)
add_test(NAME TSBasicASTTests COMMAND ts_basic_ast_tests)
add_test(NAME JSExampleASTTests COMMAND js_example_ast_tests)
add_test(NAME TSExampleASTTests COMMAND ts_example_ast_tests)
add_test(NAME EdgeCaseTests COMMAND edge_case_tests)

# Link Edge Case tests
target_link_libraries(edge_case_tests PRIVATE
    ${Criterion_LIBRARIES}
    parser_core
    m
    dl
    # Tree-sitter imported targets from root CMake
    ts_core
    ts_c
    ts_cpp
    ts_python
    ts_javascript
    ts_typescript
)

# Copy test example files and their expected JSON outputs to the build directory for testing
# This ensures the tests can find the JSON files where they're looking for them

# Process C example files and expected JSON outputs
file(GLOB_RECURSE C_EXAMPLES "${CMAKE_CURRENT_SOURCE_DIR}/examples/c/**/*.c")
file(GLOB_RECURSE C_EXPECTED_JSON "${CMAKE_CURRENT_SOURCE_DIR}/examples/c/**/*.expected.json")

# First copy to the standard examples location in build dir
foreach(EXAMPLE_FILE ${C_EXAMPLES} ${C_EXPECTED_JSON})
    file(RELATIVE_PATH REL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/examples ${EXAMPLE_FILE})
    get_filename_component(REL_DIR ${REL_PATH} DIRECTORY)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/examples/${REL_DIR})
    configure_file(${EXAMPLE_FILE} ${CMAKE_CURRENT_BINARY_DIR}/examples/${REL_PATH} COPYONLY)
endforeach()

# Now copy to the direct test location to match test search paths
foreach(EXPECTED_JSON ${C_EXPECTED_JSON})
    get_filename_component(JSON_FILENAME ${EXPECTED_JSON} NAME)
    get_filename_component(JSON_DIR ${EXPECTED_JSON} DIRECTORY)
    get_filename_component(CATEGORY ${JSON_DIR} NAME)
    string(REPLACE ".c.expected.json" ".expected.json" DEST_NAME ${JSON_FILENAME})
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/c/${CATEGORY})
    configure_file(${EXPECTED_JSON} ${CMAKE_CURRENT_BINARY_DIR}/c/${CATEGORY}/${DEST_NAME} COPYONLY)
endforeach()

message(STATUS "Configured Scopemux Bindings C Tests with Criterion.")
message(STATUS "Copied example files and expected JSON to build directory for validation.")
