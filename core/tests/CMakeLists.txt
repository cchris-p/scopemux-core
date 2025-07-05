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
# find_package(PkgConfig REQUIRED)
# # Use pkg-config to find Criterion
# pkg_check_modules(Criterion REQUIRED criterion)

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

# =============================================================================
# CRITICAL NOTE FOR ALL CONTRIBUTORS
# =============================================================================
# Common test helpers (test_helpers.c, json_validation.c, test_utils.c, tree_sitter_test_util.c)
# MUST ONLY be linked via the test_utilities static library.
# NEVER add these files directly to any test executable's source list.
# Violating this will cause duplicate symbol errors and catastrophic Criterion runtime failures.
# =============================================================================

# Set include directories for all targets in this directory
include_directories(
    # Tree-sitter headers (defined in the root CMakeLists.txt)
    ${TS_CORE_INCLUDE_DIR}
    ${TS_C_INCLUDE_DIR}
    ${TS_CPP_INCLUDE_DIR}
    ${TS_PYTHON_INCLUDE_DIR}
    ${TS_JS_INCLUDE_DIR}
    ${TS_TS_INCLUDE_DIR}
    # Project headers
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    # Add source directory to include path
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    # Python headers
    ${Python_INCLUDE_DIRS}
    ${pybind11_INCLUDE_DIRS}
)

# Add preprocessor defines for proper compilation
add_compile_definitions(
    SCOPEMUX_BUILDING=1
    PY_SSIZE_T_CLEAN
)

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
# First use sysconfig path, fallback to CMake's Python_INCLUDE_DIRS if that fails
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

# pybind11 is already loaded above

# Build parser core static library
add_library(parser_core STATIC
    # Refactored parser components
    src/parser/parser.c
    src/parser/ast_node.c
    src/parser/cst_node.c
    src/parser/memory_tracking.c
    src/parser/parser_context.c
    src/parser/query_processing.c
    # Project context components
    src/parser/project_context/project_context_facade.c
    src/parser/project_context/project_utils.c
    src/parser/project_context/project_symbol_extraction.c
    src/parser/project_context/symbol_management.c
    src/parser/project_context/symbol_collection.c
    # Tree-sitter integration
    src/parser/ts_init.c
    src/parser/ts_ast_builder.c
    src/parser/ts_cst_builder.c
    src/parser/ts_query_processor.c
    src/parser/query_manager.c
    src/parser/reference_resolvers/language_resolvers.c
    # Language compliance system
    src/ast_compliance.c
    src/parser/lang/c_compliance.c
    src/parser/lang/lang_compliance_registry.c
    # Common files
    src/common/memory_management.c
    src/common/error_handling.c
    src/common/logging.c
    # Memory debugging utilities
    src/utils/memory_debug.c
    src/utils/ts_resource_manager.c
    # Configuration
    src/config/node_type_mapping_loader.c
    # Processors
    src/processors/docstring_processor.c
    src/processors/ast_post_processor.c
    src/processors/test_processor.c
)

set_target_properties(parser_core PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    # Disable dependency file generation to avoid directory-related errors
    SKIP_DEPENDENCY_GENERATION TRUE
)

# Create a separate object library for test_processor.c to ensure it gets linked properly
add_library(test_processor_lib OBJECT
    src/processors/test_processor.c
)

# Disable dependency file generation to avoid directory-related errors
set_target_properties(test_processor_lib PROPERTIES SKIP_DEPENDENCY_GENERATION TRUE)

set(CONTEXT_ENGINE_SOURCES
    src/context_engine/compressor.c
    src/context_engine/expander.c
    src/context_engine/token_budgeter.c
)

set(COMMON_SOURCES
    src/common/error_handling.c
    src/common/memory_management.c
    src/common/logging.c
)

set(UTILS_SOURCES
    src/utils/memory_debug.c
    src/utils/ts_resource_manager.c
)

# Add library targets
add_library(context_engine STATIC ${CONTEXT_ENGINE_SOURCES} ${COMMON_SOURCES})

# Set include directories for each target
target_include_directories(parser_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(context_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Link parser_core with Tree-sitter libraries
target_link_libraries(parser_core PRIVATE
    ts_core
    ts_c
    ts_cpp
    ts_python
    ts_javascript
    ts_typescript
)

# Python bindings module with test_processor bindings and implementation
pybind11_add_module(scopemux_core MODULE
    # Binding files
    src/bindings/module.c
    src/bindings/parser_bindings.c
    src/bindings/context_engine_bindings.c
    src/bindings/test_processor_bindings.c
    src/bindings/signal_handler.c
)

# Set include directories for the Python bindings module
target_include_directories(scopemux_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bindings
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/scopemux
)

# Link the module with necessary libraries
target_link_libraries(scopemux_core PRIVATE
    ${TREE_SITTER_LIBS}
    -ldl # Needed for dlopen/dlsym functions
    parser_core # Link with the static library directly so it is built and linked
)

# Add explicit linker flags to force export of symbols and include whole archive for parser_core
# This ensures all symbols from parser_core (including segfault_handler) are included
# regardless of whether they're directly referenced
target_link_options(scopemux_core PRIVATE
    -Wl,--export-dynamic
    -Wl,--whole-archive
    -Wl,$<TARGET_FILE:parser_core>
    -Wl,--no-whole-archive
)

# Ensure all symbols from our implementation are accessible
set_target_properties(scopemux_core PROPERTIES
    ENABLE_EXPORTS TRUE
)

# Link libraries to the Python module
target_link_libraries(scopemux_core PRIVATE
    parser_core
    context_engine
    ts_core
    ts_c
    ts_cpp
    ts_python
    ts_javascript
    ts_typescript
)

# Set compiler flags to export all symbols
target_compile_options(scopemux_core PRIVATE "-rdynamic")
set_target_properties(scopemux_core PROPERTIES ENABLE_EXPORTS ON)
set_target_properties(scopemux_core PROPERTIES LINK_FLAGS "-Wl,--export-dynamic")


# Set include directories for the Python module
target_include_directories(scopemux_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/scopemux
    ${PYTHON_INCLUDE_PATH}
    ${Python_INCLUDE_DIRS}
    ${pybind11_INCLUDE_DIRS}
)

# Installation rules
install(TARGETS scopemux_core DESTINATION ${CMAKE_INSTALL_PREFIX})

# Add subdirectories for tests if they exist
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    add_subdirectory(tests)
    add_subdirectory(src/adapters)
    add_subdirectory(src/config)
endif()

# Print configuration summary
message(STATUS "Configuration summary:")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "  Python: ${Python_VERSION}")
message(STATUS "  Installation prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Memory debugging: ${SCOPEMUX_DEBUG_MEMORY}")
message(STATUS "  Valgrind compatibility: ${SCOPEMUX_VALGRIND_COMPATIBLE}")

set(RESOLVER_RESOLUTION_TEST_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/reference_resolvers/resolver_resolution_tests.c
    ${CMAKE_CURRENT_SOURCE_DIR}/reference_resolvers/project_context_test_stubs.c
)

set(LANGUAGE_RESOLVER_TEST_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/language_resolver_tests.c
)

# Symbol Table and Project Context test sources for interfile testing
set(SYMBOL_TABLE_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/symbol_table_tests.c
)

set(PROJECT_CONTEXT_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/project_context_tests.c
)

# Define executables
add_executable(init_parser_tests
    # Removed reference to test_init_parser.c - file doesn't exist
    ${INIT_PARSER_TEST_SOURCES}
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)
target_link_libraries(init_parser_tests PRIVATE
    test_utilities
    ${Criterion_LIBRARIES}
    ts_core
    m
    dl
)
# Set common include directories
set_common_test_includes(init_parser_tests)

# Edge Case tests executable
add_executable(edge_case_tests
    ${EDGE_CASE_TEST_SOURCES}
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)
target_link_libraries(edge_case_tests PRIVATE
    test_utilities
    ${Criterion_LIBRARIES}
    ts_core
    m
    dl
)
# Set common include directories
set_common_test_includes(edge_case_tests)

# --- Interfile Resolution Test Executables ---

# Main Reference Resolver Tests
add_executable(reference_resolver_tests
    ${REFERENCE_RESOLVER_TEST_SRCS}
)
target_link_libraries(reference_resolver_tests PRIVATE
    test_utilities
    parser_core
    ${Criterion_LIBRARIES}
    parser_core
    ts_core
    m
    dl
    # Tree-sitter imported targets
    ts_core
    ts_c
    ts_cpp
    ts_python
    ts_javascript
    ts_typescript
)
# Set common include directories
set_common_test_includes(reference_resolver_tests)

# Resolver Core Tests
add_executable(resolver_core_tests
    ${RESOLVER_CORE_TEST_SRCS}
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)
target_link_libraries(resolver_core_tests PRIVATE
    test_utilities
    parser_core
    ${Criterion_LIBRARIES}
    parser_core
    ts_core
    m
    dl
    # Tree-sitter imported targets
    ts_core
    ts_c
    ts_cpp
    ts_python
    ts_javascript
    ts_typescript
)
# Set common include directories
set_common_test_includes(resolver_core_tests)

# Resolver Registration Tests
add_executable(resolver_registration_tests
    ${RESOLVER_REGISTRATION_TEST_SRCS}
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)
target_link_libraries(resolver_registration_tests PRIVATE
    test_utilities
    parser_core
    ${Criterion_LIBRARIES}
    parser_core
    ts_core
    m
    dl
    # Tree-sitter imported targets
    ts_core
    ts_c
    ts_cpp
    ts_python
    ts_javascript
    ts_typescript
)
# Set common include directories
set_common_test_includes(resolver_registration_tests)

# Resolver Resolution Tests
add_executable(resolver_resolution_tests
    ${RESOLVER_RESOLUTION_TEST_SRCS}
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)
target_link_libraries(resolver_resolution_tests PRIVATE
    test_utilities
    parser_core
    ${Criterion_LIBRARIES}
    parser_core
    ts_core
    m
    dl
    # Tree-sitter imported targets
    ts_core
    ts_c
    ts_cpp
    ts_python
    ts_javascript
    ts_typescript
)
# Set common include directories
set_common_test_includes(resolver_resolution_tests)

# Language Resolver Tests
add_executable(language_resolver_tests
    ${LANGUAGE_RESOLVER_TEST_SRCS}
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)
target_link_libraries(language_resolver_tests PRIVATE
    test_utilities
    parser_core
    ${Criterion_LIBRARIES}
    parser_core
    ts_core
    m
    dl
    # Tree-sitter imported targets
    ts_core
    ts_c
    ts_cpp
    ts_python
    ts_javascript
    ts_typescript
)
# Set common include directories
set_common_test_includes(language_resolver_tests)

# Symbol Table Tests
add_executable(symbol_table_tests
    ${PROJECT_SOURCE_DIR}/src/parser/symbol_table/symbol_core.c
    ${PROJECT_SOURCE_DIR}/src/parser/symbol_table/symbol_entry.c
    ${PROJECT_SOURCE_DIR}/src/parser/symbol_table/symbol_lookup.c
    ${PROJECT_SOURCE_DIR}/src/parser/symbol_table/symbol_table_registration.c
    ${SYMBOL_TABLE_TEST_SOURCES}
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)
target_link_libraries(symbol_table_tests PRIVATE
    test_utilities
    ${Criterion_LIBRARIES}
    parser_core
    ts_core
    m
    dl
    # Tree-sitter imported targets
    ts_core
    ts_c
    ts_cpp
    ts_python
    ts_javascript
    ts_typescript
)
# Set common include directories
set_common_test_includes(symbol_table_tests)

# Disable dependency file generation to avoid directory-related errors
set_target_properties(symbol_table_tests PROPERTIES SKIP_DEPENDENCY_GENERATION TRUE)

# Project Context Tests
add_executable(project_context_tests
    # Facade files needed for the public API
    ${PROJECT_SOURCE_DIR}/src/parser/project_context.c
    ${PROJECT_SOURCE_DIR}/src/parser/symbol_table.c
    # Core implementation files needed for project_context_tests
    ${PROJECT_SOURCE_DIR}/src/parser/project_context/project_utils.c
    ${PROJECT_SOURCE_DIR}/src/parser/project_context/dependency_management.c
    ${PROJECT_SOURCE_DIR}/src/parser/project_context/file_management.c
    ${PROJECT_SOURCE_DIR}/src/parser/project_context/symbol_management.c
    ${PROJECT_SOURCE_DIR}/src/parser/symbol_table/symbol_core.c
    ${PROJECT_SOURCE_DIR}/src/parser/symbol_table/symbol_entry.c
    ${PROJECT_SOURCE_DIR}/src/parser/symbol_table/symbol_lookup.c
    ${PROJECT_SOURCE_DIR}/src/parser/symbol_table/symbol_table_registration.c
    # Reference resolver implementation files
    ${PROJECT_SOURCE_DIR}/src/parser/reference_resolver.c
    ${PROJECT_SOURCE_DIR}/src/parser/reference_resolvers/language_resolvers.c
    ${PROJECT_SOURCE_DIR}/src/parser/reference_resolvers/resolver_implementation.c
    # Test sources
    ${PROJECT_CONTEXT_TEST_SOURCES}
    # DO NOT add test_helpers.c, json_validation.c, test_utils.c, or tree_sitter_test_util.c here
)
# CRITICAL: DO NOT MODIFY THE LINKING CONFIGURATION BELOW
# The project_context_tests target depends on specific symbols from parser_core
# and requires direct linking against the full set of libraries below.
# Removing any of these dependencies will cause undefined reference errors.
# This configuration was carefully set up to resolve build issues with the test suite.
target_link_libraries(project_context_tests PRIVATE
    test_utilities
    ${Criterion_LIBRARIES}
    parser_core
    ts_core
    m
    dl
    # Tree-sitter imported targets
    ts_core
    ts_c
    ts_cpp
    ts_python
    ts_javascript
    ts_typescript
)
# Set common include directories
set_common_test_includes(project_context_tests)

# Disable dependency file generation to avoid directory-related errors
set_target_properties(project_context_tests PROPERTIES SKIP_DEPENDENCY_GENERATION TRUE)

# --- Macro to create test executables ---
# Macro now takes test target and its source variable
macro(create_test_executable test_name source_var)
    add_executable(${test_name} ${${source_var}})
    target_link_libraries(${test_name} PRIVATE
        test_utilities
        ${Criterion_LIBRARIES}
        parser_core
        ts_core
        m
        dl
        ts_c
        ts_cpp
        ts_python
        ts_javascript
        ts_typescript
    )
    # Set common include directories
    set_common_test_includes(${test_name})

    # Disable dependency file generation to avoid directory-related errors
    set_target_properties(${test_name} PROPERTIES SKIP_DEPENDENCY_GENERATION TRUE)
endmacro()

# --- Create Language-Specific Test Executables ---
create_test_executable(c_basic_ast_tests C_BASIC_AST_TEST_SOURCES)
create_test_executable(python_basic_ast_tests PYTHON_BASIC_AST_TEST_SOURCES)
create_test_executable(cpp_basic_ast_tests CPP_BASIC_AST_TEST_SOURCES)
create_test_executable(c_example_ast_tests C_EXAMPLE_AST_TEST_SOURCES)
create_test_executable(python_example_ast_tests PYTHON_EXAMPLE_AST_TEST_SOURCES)
create_test_executable(cpp_example_ast_tests CPP_EXAMPLE_AST_TEST_SOURCES)
create_test_executable(js_basic_ast_tests JS_BASIC_AST_TEST_SOURCES)
create_test_executable(ts_basic_ast_tests TS_BASIC_AST_TEST_SOURCES)
create_test_executable(js_example_ast_tests JS_EXAMPLE_AST_TEST_SOURCES)
create_test_executable(ts_example_ast_tests TS_EXAMPLE_AST_TEST_SOURCES)

# Use whole-archive linker flags to ensure static libraries are fully linked
target_link_libraries(cpp_basic_ast_tests PRIVATE
    ${Criterion_LIBRARIES}
    parser_core
    # Start whole-archive section for Tree-sitter libraries
    # This forces inclusion of all symbols from static libraries
    -Wl,--whole-archive
    ts_c
    ts_cpp
    ts_python
    ts_javascript
    ts_typescript
    ts_core
    -Wl,--no-whole-archive
    # System libraries at the end
    m
    dl
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
    ts_core
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
    ts_core
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
    ts_core
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
    ts_core
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
    ts_core
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
    ts_core
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
    ts_core
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

# Add interfile resolution tests to CTest
add_test(NAME ReferenceResolverTests COMMAND reference_resolver_tests)
add_test(NAME ResolverCoreTests COMMAND resolver_core_tests)
add_test(NAME ResolverRegistrationTests COMMAND resolver_registration_tests)
add_test(NAME ResolverResolutionTests COMMAND resolver_resolution_tests)
add_test(NAME LanguageResolverTests COMMAND language_resolver_tests)
add_test(NAME SymbolTableTests COMMAND symbol_table_tests)
add_test(NAME ProjectContextTests COMMAND project_context_tests)

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

# Note: We no longer copy files to the c/ directory to avoid duplication
# Tests should look directly in the examples/c directory instead

# --- Memory Debug/Crash Handler Diagnostic Test ---
add_executable(test_memory_debug
    ${CMAKE_CURRENT_SOURCE_DIR}/test_memory_debug.c
)
target_link_libraries(test_memory_debug PRIVATE parser_core m dl)
add_test(NAME MemoryDebugTest COMMAND test_memory_debug)

message(STATUS "Configured Scopemux Bindings C Tests with Criterion.")
message(STATUS "Copied example files and expected JSON to build directory for validation.")
