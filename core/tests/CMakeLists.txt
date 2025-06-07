cmake_minimum_required(VERSION 3.10)
project(ScopemuxBindingsCTests)

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
    ${CMAKE_CURRENT_SOURCE_DIR}/utilities
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/tree-sitter/lib/include
    ${Criterion_INCLUDE_DIRS}
    ${TS_CORE_INCLUDE_DIR}                 # For tree_sitter/api.h, from root CMake
    # Add other necessary include directories for your core C libraries if they are not covered
)

# --- Test Utilities (can be compiled directly with tests or as a separate library) ---
set(TEST_UTIL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/utilities/test_utils.c
)
# If you decide to make it a library:
# add_library(scopemux_test_utils STATIC ${TEST_UTIL_SOURCES})
# target_include_directories(scopemux_test_utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/utilities)

# --- Criterion Test Executable ---

# Define the core C source files from the main project needed for these tests
# These are compiled directly into the test executable.
set(SCOPEMUX_CORE_C_SOURCES_FOR_TESTS
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/parser.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/tree_sitter_integration.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/ir_generator.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/common/error_handling.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/common/logging.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/common/memory_management.c
    # Tree-sitter core and grammar sources are now linked as libraries
)

set(C_PARSER_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/c_parser_tests.c
    src/c_parser_tests.c
    ${TEST_UTIL_SOURCES} # Compile utilities directly with the test executable
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)

add_executable(scopemux_c_parser_tests ${C_PARSER_TEST_SOURCES})

# Link against Criterion and your project's C libraries
# Link against Criterion (found via pkg-config) and other necessary system libraries
target_link_libraries(scopemux_c_parser_tests PRIVATE
    ${Criterion_LIBRARIES} # Provided by pkg_check_modules
    tree-sitter-core       # Tree-sitter core library
    tree-sitter-grammars   # Tree-sitter language grammar libraries
    m                      # Math library, often needed
    dl                     # Dynamic linking library, sometimes needed by test frameworks or tree-sitter
)

# Add test to CTest (optional, but good practice)
# This allows 'ctest' command to discover and run it.
include(CTest)
add_test(NAME CParserTests COMMAND scopemux_c_parser_tests)

message(STATUS "Configured Scopemux Bindings C Tests with Criterion.")
