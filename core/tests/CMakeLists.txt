cmake_minimum_required(VERSION 3.16)  # Updated to match main CMakeLists.txt
project(ScopemuxBindingsCTests LANGUAGES C CXX)

# Set C++ standard (needed for pybind11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set C standard (Criterion tests are in C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find PkgConfig for locating Criterion via .pc file
find_package(PkgConfig REQUIRED)
# Use pkg-config to find Criterion
pkg_check_modules(Criterion REQUIRED criterion)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/scopemux
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/utilities
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/tree-sitter/lib/include
    ${Criterion_INCLUDE_DIRS}
    ${TS_CORE_INCLUDE_DIR} # For tree_sitter/api.h, from root CMake
    # Add other necessary include directories for your core C libraries if they are not covered
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

# --- Common Test Sources ---
# (Previously contained ast_extraction_tests.c which has been migrated to language-specific files)

# --- Edge Case Tests ---
set(EDGE_CASE_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common/edge_case_tests.c
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

# Link against Criterion and your project's C libraries
target_link_libraries(init_parser_tests PRIVATE
    ${Criterion_LIBRARIES} # Provided by pkg_check_modules
    m # Math library, often needed
    dl # Dynamic linking library, sometimes needed by test frameworks or tree-sitter
)

# Link C Basic AST tests
target_link_libraries(c_basic_ast_tests PRIVATE
    ${Criterion_LIBRARIES}
    m
    dl
)

# Link Python Basic AST tests
target_link_libraries(python_basic_ast_tests PRIVATE
    ${Criterion_LIBRARIES}
    m
    dl
)

# Link C++ Basic AST tests
target_link_libraries(cpp_basic_ast_tests PRIVATE
    ${Criterion_LIBRARIES}
    m
    dl
)

# Link Example AST tests
target_link_libraries(c_example_ast_tests PRIVATE
    ${Criterion_LIBRARIES}
    m
    dl
)

target_link_libraries(python_example_ast_tests PRIVATE
    ${Criterion_LIBRARIES}
    m
    dl
)

target_link_libraries(cpp_example_ast_tests PRIVATE
    ${Criterion_LIBRARIES}
    m
    dl
)

# Link Edge Case tests
target_link_libraries(edge_case_tests PRIVATE
    ${Criterion_LIBRARIES}
    m
    dl
)

# Add tests to CTest 
add_test(NAME InitParserTests COMMAND init_parser_tests)
add_test(NAME CBasicASTTests COMMAND c_basic_ast_tests)
add_test(NAME PythonBasicASTTests COMMAND python_basic_ast_tests)
add_test(NAME CPPBasicASTTests COMMAND cpp_basic_ast_tests)
add_test(NAME CExampleASTTests COMMAND c_example_ast_tests)
add_test(NAME PythonExampleASTTests COMMAND python_example_ast_tests)
add_test(NAME CPPExampleASTTests COMMAND cpp_example_ast_tests)
add_test(NAME EdgeCaseTests COMMAND edge_case_tests)

message(STATUS "Configured Scopemux Bindings C Tests with Criterion.")
