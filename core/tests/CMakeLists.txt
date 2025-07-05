# Configuration for ScopeMux Bindings C Tests
# C/C++ standards are set in the root CMakeLists.txt

# Include test utilities
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/TestUtils.cmake)

# Add compiler flags to suppress warnings in test code
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wstrict-prototypes -Wmissing-prototypes")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wimplicit-function-declaration -Wformat-security")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wpointer-arith -Wunused-variable")

    # Memory safety flags (enable in debug builds)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fsanitize=undefined")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-omit-frame-pointer -g")
        message(STATUS "Memory sanitizers enabled for debug build")
    endif()
endif()

# Option to enable memory debugging features
option(SCOPEMUX_DEBUG_MEMORY "Enable memory debugging and tracking" OFF)
if(SCOPEMUX_DEBUG_MEMORY)
    add_definitions(-DSCOPEMUX_DEBUG_MEMORY)
    message(STATUS "Memory debugging enabled: detailed tracking and validation will be active")
endif()

# Configure Valgrind support when requested
option(SCOPEMUX_VALGRIND_COMPATIBLE "Build with Valgrind compatibility" OFF)
if(SCOPEMUX_VALGRIND_COMPATIBLE)
    add_definitions(-DSCOPEMUX_VALGRIND_COMPATIBLE)
    message(STATUS "Building with Valgrind compatibility")
endif()

# Add compiler flags for Python C API
add_compile_options($<$<COMPILE_LANGUAGE:C>:-DPy_BUILD_CORE_MODULE>)

# Get the Python include directories explicitly
execute_process(
    COMMAND ${Python_EXECUTABLE} -c "import sysconfig; print(sysconfig.get_path('include'))"
    OUTPUT_VARIABLE PYTHON_INCLUDE_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "Python include path from sysconfig: ${PYTHON_INCLUDE_PATH}")

# Configure Python includes directly
if(NOT PYTHON_INCLUDE_PATH)
    set(PYTHON_INCLUDE_PATH "${Python_INCLUDE_DIRS}")
    message(STATUS "Using CMake's Python_INCLUDE_DIRS as fallback")
endif()

# Add custom compiler options to enforce include path and Python macro definitions
add_compile_options(
    $<$<COMPILE_LANGUAGE:C>:-I${PYTHON_INCLUDE_PATH}>
    $<$<COMPILE_LANGUAGE:C>:-DPY_SSIZE_T_CLEAN>
    $<$<COMPILE_LANGUAGE:C>:-DPy_BUILD_CORE_MODULE>
)

# Output Python information for debugging
message(STATUS "Python include dirs: ${Python_INCLUDE_DIRS}")
message(STATUS "Python libraries: ${Python_LIBRARIES}")

# Create test-specific libraries with different names to avoid conflicts
add_library(test_utilities STATIC
    src/common/test_helpers.c
    src/common/json_validation.c
    utilities/tree_sitter_test_util.c
)

# Create a lightweight interface library that other tests can link to for shared
# parser_core functionality without pulling in specific test sources.
add_library(test_parser INTERFACE)
# Link parser_core so any target that links to test_parser automatically gets the
# necessary core parser symbols.
target_link_libraries(test_parser INTERFACE parser_core)

# Link test libraries with core libraries
# For the interface library, link parser_core transitively for consumers
# (This line is safe even if the previous INTERFACE linkage already exists.)
target_link_libraries(test_parser INTERFACE parser_core)
target_link_libraries(test_utilities PRIVATE parser_core ${CRITERION_LIBRARIES})

# Set include directories for test targets
# include directories already set above; removed problematic macro call
# set_common_test_includes(test_utilities)

# Create test executable
add_executable(run_tests
    src/init_parser_tests.c
    src/edge_case_tests.c
)

target_link_libraries(run_tests PRIVATE
    test_parser
    test_utilities
    parser_core
    context_engine
)

# include directories already set above; removed problematic macro call
# set_common_test_includes(run_tests)

# Add Criterion for unit testing (via pkg-config workaround)
find_package(PkgConfig REQUIRED)
pkg_check_modules(CRITERION REQUIRED criterion)

# Add init_parser_tests target
add_executable(init_parser_tests
    src/init_parser_tests.c
)
# Criterion provides its own main; include its headers and link libraries.
target_include_directories(init_parser_tests PRIVATE ${CRITERION_INCLUDE_DIRS})
# Link against utilities, core libraries, and Criterion.
target_link_libraries(init_parser_tests PRIVATE
    test_utilities
    parser_core
    context_engine
    ${CRITERION_LIBRARIES}
)
# include directories already set above; removed problematic macro call
# set_common_test_includes(init_parser_tests)

# Add edge_case_tests target
add_executable(edge_case_tests
    src/edge_case_tests.c
)
target_include_directories(edge_case_tests PRIVATE ${CRITERION_INCLUDE_DIRS})
target_link_libraries(edge_case_tests PRIVATE
    test_utilities
    parser_core
    context_engine
    ${CRITERION_LIBRARIES}
)
# include directories already set above; removed problematic macro call
# set_common_test_includes(edge_case_tests)

# Add c_example_ast_tests target
add_executable(c_example_ast_tests
    src/c/c_example_ast_tests.c
)
target_include_directories(c_example_ast_tests PRIVATE ${CRITERION_INCLUDE_DIRS})
target_link_libraries(c_example_ast_tests PRIVATE
    test_parser
    test_utilities
    parser_core
    context_engine
    ${CRITERION_LIBRARIES}
)
# include directories already set above; removed problematic macro call
# set_common_test_includes(c_example_ast_tests)

# Add cpp_example_ast_tests target
add_executable(cpp_example_ast_tests
    src/cpp/cpp_example_ast_tests.c
)
target_include_directories(cpp_example_ast_tests PRIVATE ${CRITERION_INCLUDE_DIRS})
target_link_libraries(cpp_example_ast_tests PRIVATE
    test_parser
    test_utilities
    parser_core
    context_engine
    ${CRITERION_LIBRARIES}
)
# include directories already set above; removed problematic macro call
# set_common_test_includes(cpp_example_ast_tests)

# Add cpp_basic_ast_tests target
add_executable(cpp_basic_ast_tests
    src/cpp/cpp_basic_ast_tests.c
)
target_include_directories(cpp_basic_ast_tests PRIVATE ${CRITERION_INCLUDE_DIRS})
target_link_libraries(cpp_basic_ast_tests PRIVATE
    test_parser
    test_utilities
    parser_core
    context_engine
    ${CRITERION_LIBRARIES}
)
# include directories already set above; removed problematic macro call
# set_common_test_includes(cpp_basic_ast_tests)

# Add js_basic_ast_tests target
add_executable(js_basic_ast_tests
    src/js/js_basic_ast_tests.c
)
target_include_directories(js_basic_ast_tests PRIVATE ${CRITERION_INCLUDE_DIRS})
target_link_libraries(js_basic_ast_tests PRIVATE
    test_parser
    test_utilities
    parser_core
    context_engine
    ${CRITERION_LIBRARIES}
)
# include directories already set above; removed problematic macro call
# set_common_test_includes(js_basic_ast_tests)

# Add ts_basic_ast_tests target
add_executable(ts_basic_ast_tests
    src/ts/ts_basic_ast_tests.c
)
target_include_directories(ts_basic_ast_tests PRIVATE ${CRITERION_INCLUDE_DIRS})
target_link_libraries(ts_basic_ast_tests PRIVATE
    test_parser
    test_utilities
    parser_core
    context_engine
    ${CRITERION_LIBRARIES}
)
# include directories already set above; removed problematic macro call
# set_common_test_includes(ts_basic_ast_tests)

# Add python_basic_ast_tests target
add_executable(python_basic_ast_tests
    src/python/python_basic_ast_tests.c
)
target_include_directories(python_basic_ast_tests PRIVATE ${CRITERION_INCLUDE_DIRS})
target_link_libraries(python_basic_ast_tests PRIVATE
    test_parser
    test_utilities
    parser_core
    context_engine
    ${CRITERION_LIBRARIES}
)
# include directories already set above; removed problematic macro call
# set_common_test_includes(python_basic_ast_tests)

# Add python_example_ast_tests target
add_executable(python_example_ast_tests
    src/python/python_example_ast_tests.c
)
target_include_directories(python_example_ast_tests PRIVATE ${CRITERION_INCLUDE_DIRS})
target_link_libraries(python_example_ast_tests PRIVATE
    test_parser
    test_utilities
    parser_core
    context_engine
    ${CRITERION_LIBRARIES}
)
# include directories already set above; removed problematic macro call
# set_common_test_includes(python_example_ast_tests)

# Add interfile test targets
# Symbol Table Tests
add_executable(symbol_table_tests
    symbol_table_tests.c
)
target_include_directories(symbol_table_tests PRIVATE ${CRITERION_INCLUDE_DIRS})
target_link_libraries(symbol_table_tests PRIVATE
    test_parser
    test_utilities
    parser_core
    context_engine
    ${CRITERION_LIBRARIES}
)

# Project Context Tests
add_executable(project_context_tests
    project_context_tests.c
)
target_include_directories(project_context_tests PRIVATE ${CRITERION_INCLUDE_DIRS})
target_link_libraries(project_context_tests PRIVATE
    test_parser
    test_utilities
    parser_core
    context_engine
    ${CRITERION_LIBRARIES}
)

# Reference Resolver Tests
add_executable(reference_resolver_tests
    reference_resolver_tests.c
)
target_include_directories(reference_resolver_tests PRIVATE ${CRITERION_INCLUDE_DIRS})
target_link_libraries(reference_resolver_tests PRIVATE
    test_parser
    test_utilities
    parser_core
    context_engine
    ${CRITERION_LIBRARIES}
)

# Resolver Core Tests
add_executable(resolver_core_tests
    resolver_core_tests.c
)
target_include_directories(resolver_core_tests PRIVATE ${CRITERION_INCLUDE_DIRS})
target_link_libraries(resolver_core_tests PRIVATE
    test_parser
    test_utilities
    parser_core
    context_engine
    ${CRITERION_LIBRARIES}
)

# Resolver Registration Tests
add_executable(resolver_registration_tests
    resolver_registration_tests.c
)
target_include_directories(resolver_registration_tests PRIVATE ${CRITERION_INCLUDE_DIRS})
target_link_libraries(resolver_registration_tests PRIVATE
    test_parser
    test_utilities
    parser_core
    context_engine
    ${CRITERION_LIBRARIES}
)

# Resolver Resolution Tests
add_executable(resolver_resolution_tests
    resolver_resolution_tests.c
)
target_include_directories(resolver_resolution_tests PRIVATE ${CRITERION_INCLUDE_DIRS})
target_link_libraries(resolver_resolution_tests PRIVATE
    test_parser
    test_utilities
    parser_core
    context_engine
    ${CRITERION_LIBRARIES}
)

# Language Resolver Tests
add_executable(language_resolver_tests
    language_resolver_tests.c
)
target_include_directories(language_resolver_tests PRIVATE ${CRITERION_INCLUDE_DIRS})
target_link_libraries(language_resolver_tests PRIVATE
    test_parser
    test_utilities
    parser_core
    context_engine
    ${CRITERION_LIBRARIES}
)

# Installation rules
install(TARGETS run_tests DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

message(STATUS "Configured Scopemux Bindings C Tests with Criterion.")
message(STATUS "Copied example files and expected JSON to build directory for validation.")

target_include_directories(test_utilities PRIVATE ${CRITERION_INCLUDE_DIRS}) 