# Configuration for ScopeMux Bindings C Tests
# C/C++ standards are set in the root CMakeLists.txt

# ============================================================================
# IMPORTANT TEST LINKING PATTERN
# ============================================================================
# This build system uses a specific pattern to avoid duplicate symbol errors:
# 
# 1. Common test utilities (json_validation.c, test_helpers.c, etc.) are compiled
#    into a single 'test_utilities' static library.
#
# 2. Individual test executables LINK to test_utilities rather than directly
#    including these source files.
#
# 3. Source files should NEVER be included in target_link_libraries commands.
#    This previously caused duplicate symbol errors where json_validation.c was
#    being linked twice.
#
# 4. The SCOPEMUX_CORE_C_SOURCES_FOR_TESTS variable should not contain any
#    source files that are already part of the test_utilities library.
# ============================================================================

# Find PkgConfig for locating Criterion via .pc file
find_package(PkgConfig REQUIRED)
# Use pkg-config to find Criterion
pkg_check_modules(Criterion REQUIRED criterion)

# Set include directories for all targets in this directory
include_directories(
    # Project directories
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/utilities

    # Core include directories - the main one contains the scopemux/ directory
    ${PROJECT_SOURCE_DIR}/core/include

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
# Removed duplicate include_directories line

# --- Test Utilities and Common Test Helpers ---
# IMPORTANT: This static library pattern prevents duplicate symbol errors
# Files included here (especially json_validation.c) should NOT be:
# 1. Added to individual test executable source lists
# 2. Directly linked via target_link_libraries
# 3. Included in SCOPEMUX_CORE_C_SOURCES_FOR_TESTS variable
#
# Instead, all test targets should link against this library using target_link_libraries
add_library(test_utilities STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/utilities/test_utils.c
    ${CMAKE_CURRENT_SOURCE_DIR}/utilities/tree_sitter_test_util.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common/test_helpers.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common/json_validation.c
)
target_include_directories(test_utilities PRIVATE
    ${PROJECT_SOURCE_DIR}/core/include
    ${PROJECT_SOURCE_DIR}/core/include/scopemux
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Criterion_INCLUDE_DIRS}
)

# --- Criterion Test Executable ---

# Define the core C source files from the main project needed for these tests
# These are compiled directly into the test executable.
set(SCOPEMUX_CORE_C_SOURCES_FOR_TESTS
    # Parser core components
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/parser.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/tree_sitter_integration.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/query_manager.c

    # Common utilities
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/common/error_handling.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/common/logging.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/common/memory_management.c

    # Adapters for language-specific functionality
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/adapters/adapter_registry.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/adapters/language_adapter.c

    # Processors that post-process the AST
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/processors/ast_post_processor.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/processors/docstring_processor.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/processors/test_processor.c

    # Configuration modules
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/config/node_type_mapping_loader.c
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
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)
target_link_libraries(init_parser_tests PRIVATE test_utilities)
target_include_directories(init_parser_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/core/include
    ${PROJECT_SOURCE_DIR}/core/include/scopemux
)

# Edge Case tests executable
add_executable(edge_case_tests
    ${EDGE_CASE_TEST_SOURCES}
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)
target_link_libraries(edge_case_tests PRIVATE test_utilities)
target_include_directories(edge_case_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/core/include
    ${PROJECT_SOURCE_DIR}/core/include/scopemux
)


# C Basic AST tests executable
add_executable(c_basic_ast_tests
    ${C_BASIC_AST_TEST_SOURCES}
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)
target_link_libraries(c_basic_ast_tests PRIVATE test_utilities)
target_include_directories(c_basic_ast_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/core/include
    ${PROJECT_SOURCE_DIR}/core/include/scopemux
)


# Python Basic AST tests executable
add_executable(python_basic_ast_tests
    ${PYTHON_BASIC_AST_TEST_SOURCES}
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)
target_link_libraries(python_basic_ast_tests PRIVATE test_utilities)
target_include_directories(python_basic_ast_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/core/include
    ${PROJECT_SOURCE_DIR}/core/include/scopemux
)


# C++ Basic AST tests executable
add_executable(cpp_basic_ast_tests
    ${CPP_BASIC_AST_TEST_SOURCES}
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)
target_link_libraries(cpp_basic_ast_tests PRIVATE test_utilities)
target_include_directories(cpp_basic_ast_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/core/include
    ${PROJECT_SOURCE_DIR}/core/include/scopemux
)


# Example AST test executables
add_executable(c_example_ast_tests
    ${C_EXAMPLE_AST_TEST_SOURCES}
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)
target_link_libraries(c_example_ast_tests PRIVATE test_utilities)
# Ensure correct include paths for all scopemux headers
# This prevents missing header errors for scopemux/logging.h, etc.
target_include_directories(c_example_ast_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/core/include
    ${PROJECT_SOURCE_DIR}/core/include/scopemux
)
get_target_property(EXAMPLE_TESTS_INCLUDES c_example_ast_tests INCLUDE_DIRECTORIES)
message(STATUS "[CMake Diagnostics] c_example_ast_tests include dirs: ${EXAMPLE_TESTS_INCLUDES}")


add_executable(python_example_ast_tests
    ${PYTHON_EXAMPLE_AST_TEST_SOURCES}
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)
target_include_directories(python_example_ast_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/core/include
    ${PROJECT_SOURCE_DIR}/core/include/scopemux
)


add_executable(cpp_example_ast_tests
    ${CPP_EXAMPLE_AST_TEST_SOURCES}
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)
target_include_directories(cpp_example_ast_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/core/include
    ${PROJECT_SOURCE_DIR}/core/include/scopemux
)


# JavaScript and TypeScript test executables
add_executable(js_basic_ast_tests
    ${JS_BASIC_AST_TEST_SOURCES}
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)
target_link_libraries(js_basic_ast_tests PRIVATE test_utilities)
target_include_directories(js_basic_ast_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/core/include
    ${PROJECT_SOURCE_DIR}/core/include/scopemux
)


add_executable(ts_basic_ast_tests
    ${TS_BASIC_AST_TEST_SOURCES}
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)
target_link_libraries(ts_basic_ast_tests PRIVATE test_utilities)
target_include_directories(ts_basic_ast_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/core/include
    ${PROJECT_SOURCE_DIR}/core/include/scopemux
)


add_executable(js_example_ast_tests
    ${JS_EXAMPLE_AST_TEST_SOURCES}
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)

add_executable(ts_example_ast_tests
    ${TS_EXAMPLE_AST_TEST_SOURCES}
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
# NOTE: Source files (including json_validation.c) should NEVER be directly
# included in target_link_libraries. This previously caused duplicate symbol errors
# since json_validation.c was already compiled into test_utilities.
# Always link against the appropriate library instead.
target_link_libraries(c_example_ast_tests PRIVATE
    test_utilities # This already contains json_validation.c
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
    test_utilities # This already contains json_validation.c
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
    test_utilities # This already contains json_validation.c
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
    test_utilities # This already contains json_validation.c
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
    test_utilities # This already contains json_validation.c
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

# --- Memory Debug/Crash Handler Diagnostic Test ---
add_executable(test_memory_debug
    ${CMAKE_CURRENT_SOURCE_DIR}/test_memory_debug.c
)
target_link_libraries(test_memory_debug PRIVATE parser_core m dl)
add_test(NAME MemoryDebugTest COMMAND test_memory_debug)

message(STATUS "Configured Scopemux Bindings C Tests with Criterion.")
message(STATUS "Copied example files and expected JSON to build directory for validation.")
