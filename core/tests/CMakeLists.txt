cmake_minimum_required(VERSION 3.10)
project(scopemux_tests)
# Configuration for ScopeMux Bindings C Tests
# C/C++ standards are set in the root CMakeLists.txt

# Include test utilities
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/TestUtils.cmake)

# Add Criterion for unit testing (via pkg-config workaround)
find_package(PkgConfig REQUIRED)
pkg_check_modules(CRITERION REQUIRED criterion)

add_library(scopemux_test_build_options INTERFACE)
target_compile_options(scopemux_test_build_options INTERFACE
    $<$<COMPILE_LANG_AND_ID:C,GNU,Clang>:-Wall;-Wextra;-pedantic;-Wstrict-prototypes;-Wmissing-prototypes;-Wimplicit-function-declaration;-Wformat-security;-Wpointer-arith;-Wunused-variable>
    $<$<AND:$<CONFIG:Debug>,$<COMPILE_LANG_AND_ID:C,GNU,Clang>>:-g;-fsanitize=address;-fsanitize=undefined;-fno-omit-frame-pointer>
)
target_link_options(scopemux_test_build_options INTERFACE
    $<$<CONFIG:Debug>:-fsanitize=address;-fsanitize=undefined>
)
target_compile_definitions(scopemux_test_build_options INTERFACE PY_SSIZE_T_CLEAN)

add_library(scopemux_test_includes INTERFACE)
target_include_directories(scopemux_test_includes INTERFACE
    ${CMAKE_SOURCE_DIR}/core/include
    ${CMAKE_SOURCE_DIR}/core/include/config
    ${CMAKE_SOURCE_DIR}/core/include/scopemux
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CRITERION_INCLUDE_DIRS}
)

# Option to enable memory debugging features
option(SCOPEMUX_DEBUG_MEMORY "Enable memory debugging and tracking" OFF)
if(SCOPEMUX_DEBUG_MEMORY)
    target_compile_definitions(scopemux_test_build_options INTERFACE SCOPEMUX_DEBUG_MEMORY)
    message(STATUS "Memory debugging enabled: detailed tracking and validation will be active")
endif()

# Configure Valgrind support when requested
option(SCOPEMUX_VALGRIND_COMPATIBLE "Build with Valgrind compatibility" OFF)
if(SCOPEMUX_VALGRIND_COMPATIBLE)
    target_compile_definitions(scopemux_test_build_options INTERFACE SCOPEMUX_VALGRIND_COMPATIBLE)
    message(STATUS "Building with Valgrind compatibility")
endif()

# Output Python information for debugging
message(STATUS "Python include dirs: ${Python_INCLUDE_DIRS}")
message(STATUS "Python libraries: ${Python_LIBRARIES}")

# Create test utilities library
add_library(test_utilities STATIC
    src/common/test_helpers.c
    src/common/json_validation.c
    src/common/symbol_test_helpers.c
    utilities/tree_sitter_test_util.c
    src/utilities/ast_test_utils.c
)

target_link_libraries(test_utilities PUBLIC scopemux_test_build_options scopemux_test_includes)

# Create a lightweight interface library that other tests can link to for shared
# parser_core functionality without pulling in specific test sources.
add_library(test_parser INTERFACE)
# Link parser_core so any target that links to test_parser automatically gets the
# necessary core parser symbols.
target_link_libraries(test_parser INTERFACE parser_core scopemux_test_build_options scopemux_test_includes)

# Link test libraries with core libraries
# For the interface library, link parser_core transitively for consumers
# (This line is safe even if the previous INTERFACE linkage already exists.)
target_link_libraries(test_utilities PRIVATE
    parser_core
    ${CRITERION_LIBRARIES}
    ts_core
    ts_c
    ts_cpp
    ts_python
    ts_javascript
    ts_typescript
)

function(add_scopemux_test target)
    add_executable(${target} ${ARGN})
    target_link_libraries(${target} PRIVATE
        test_parser
        test_utilities
        parser_core
        context_engine
        ${CRITERION_LIBRARIES}
        ts_core
        ts_c
        ts_cpp
        ts_python
        ts_javascript
        ts_typescript
    )
endfunction()

# Set include directories for test targets
# include directories already set above; removed problematic macro call
# set_common_test_includes(test_utilities)

# Create test executable
add_scopemux_test(run_tests
    src/init_parser_tests.c
    src/edge_case_tests.c
)


# include directories already set above; removed problematic macro call
# set_common_test_includes(run_tests)

# Add init_parser_tests target
add_scopemux_test(init_parser_tests
    src/init_parser_tests.c
)
# include directories already set above; removed problematic macro call
# set_common_test_includes(init_parser_tests)

# Add edge_case_tests target
add_scopemux_test(edge_case_tests
    src/edge_case_tests.c
)
# include directories already set above; removed problematic macro call
# set_common_test_includes(edge_case_tests)

# Add c_example_ast_tests target
add_scopemux_test(c_example_ast_tests
    src/c/c_example_ast_tests.c
)
# include directories already set above; removed problematic macro call
# set_common_test_includes(c_example_ast_tests)

# Add cpp_example_ast_tests target
add_scopemux_test(cpp_example_ast_tests
    src/cpp/cpp_example_ast_tests.c
)
# include directories already set above; removed problematic macro call
# set_common_test_includes(cpp_example_ast_tests)

# Add cpp_basic_ast_tests target
add_scopemux_test(cpp_basic_ast_tests
    src/cpp/cpp_basic_ast_tests.c
)
# include directories already set above; removed problematic macro call
# set_common_test_includes(cpp_basic_ast_tests)

# Add js_basic_ast_tests target
add_scopemux_test(js_basic_ast_tests
    src/js/js_basic_ast_tests.c
)
# include directories already set above; removed problematic macro call
# set_common_test_includes(js_basic_ast_tests)

# Add js_example_ast_tests target
add_scopemux_test(js_example_ast_tests
    src/js/js_example_ast_tests.c
)
# include directories already set above; removed problematic macro call
# set_common_test_includes(js_example_ast_tests)

# Add c_basic_ast_tests target
add_scopemux_test(c_basic_ast_tests
    src/c/c_basic_ast_tests.c
)
# include directories already set above; removed problematic macro call
# set_common_test_includes(c_basic_ast_tests)

# Add ts_basic_ast_tests target
add_scopemux_test(ts_basic_ast_tests
    src/ts/ts_basic_ast_tests.c
)
# include directories already set above; removed problematic macro call
# set_common_test_includes(ts_basic_ast_tests)

# Add ts_example_ast_tests target
add_scopemux_test(ts_example_ast_tests
    src/ts/ts_example_ast_tests.c
)
# include directories already set above; removed problematic macro call
# set_common_test_includes(ts_example_ast_tests)

# Add python_basic_ast_tests target
add_scopemux_test(python_basic_ast_tests
    src/python/python_basic_ast_tests.c
)
# include directories already set above; removed problematic macro call
# set_common_test_includes(python_basic_ast_tests)

# Add python_example_ast_tests target
add_scopemux_test(python_example_ast_tests
    src/python/python_example_ast_tests.c
)
# include directories already set above; removed problematic macro call
# set_common_test_includes(python_example_ast_tests)

# Add interfile test targets
# Project Context Tests
add_scopemux_test(project_context_tests
    project_context/project_context_tests.c
)

# Resolver Registration Tests
add_scopemux_test(resolver_registration_tests reference_resolvers/resolver_registration_tests.c)

# Resolver Resolution Tests
add_scopemux_test(resolver_resolution_tests reference_resolvers/resolver_resolution_tests.c)

# Language Resolver Tests
add_scopemux_test(language_resolver_tests
    language_resolver/language_resolver_tests.c
)

# Resolver Core Tests
add_scopemux_test(resolver_core_tests reference_resolvers/resolver_core_tests.c)

# Symbol Table Tests
add_scopemux_test(symbol_table_tests symbol_table/symbol_table_tests.c)

# Installation rules
install(TARGETS run_tests DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

# Copy the examples directory (including .expected.json files) to the build directory for test data access
# Preserve the directory structure by copying to core/tests/examples
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/examples DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/core/tests)

message(STATUS "Configured Scopemux Bindings C Tests with Criterion.")
message(STATUS "Copied example files and expected JSON to build directory for validation.")
