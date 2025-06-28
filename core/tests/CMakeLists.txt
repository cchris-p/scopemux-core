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

    ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/query_manager.c
    
    # Tree-sitter implementation files
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/ts_init.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/ts_ast_builder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/ts_cst_builder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/ts_query_processor.c

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

# --- Interfile Resolution Tests ---
# Define source files for reference resolver implementation stubs
set(REFERENCE_RESOLVER_STUB_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/reference_resolvers/reference_resolver_stubs.c
  ${CMAKE_CURRENT_SOURCE_DIR}/reference_resolvers/resolver_test_adapter.c
)

# Define source files for each interfile reference resolution test target
set(REFERENCE_RESOLVER_TEST_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/reference_resolver_tests.c
  ${CMAKE_CURRENT_SOURCE_DIR}/reference_resolvers/symbol_test_helpers.c
)

set(RESOLVER_CORE_TEST_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/reference_resolvers/resolver_core_tests.c
    ${CMAKE_CURRENT_SOURCE_DIR}/reference_resolvers/symbol_test_helpers.c
)

set(RESOLVER_REGISTRATION_TEST_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/reference_resolvers/resolver_registration_tests.c
  ${CMAKE_CURRENT_SOURCE_DIR}/reference_resolvers/symbol_test_helpers.c
)

set(RESOLVER_RESOLUTION_TEST_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/reference_resolvers/resolver_resolution_tests.c
    ${CMAKE_CURRENT_SOURCE_DIR}/reference_resolvers/symbol_test_helpers.c
    ${CMAKE_CURRENT_SOURCE_DIR}/reference_resolvers/project_context_test_stubs.c
)

set(LANGUAGE_RESOLVER_TEST_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/reference_resolvers/language_resolver_tests.c
  ${CMAKE_CURRENT_SOURCE_DIR}/reference_resolvers/symbol_test_helpers.c
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

# --- Reference Resolver Stub Library ---
add_library(reference_resolver_stubs STATIC ${REFERENCE_RESOLVER_STUB_SOURCES})
target_include_directories(reference_resolver_stubs PRIVATE
    ${PROJECT_SOURCE_DIR}/core/include
    ${PROJECT_SOURCE_DIR}/core/include/scopemux
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/reference_resolvers
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Criterion_INCLUDE_DIRS}
)

# --- Interfile Resolution Test Executables ---

# Main Reference Resolver Tests
add_executable(reference_resolver_tests
    ${REFERENCE_RESOLVER_TEST_SRCS}
)
target_link_libraries(reference_resolver_tests PRIVATE 
    test_utilities
    reference_resolver_stubs
    ${Criterion_LIBRARIES}
    parser_core
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
target_include_directories(reference_resolver_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/core/include
    ${PROJECT_SOURCE_DIR}/core/include/scopemux
    ${Criterion_INCLUDE_DIRS}
)

# Resolver Core Tests
add_executable(resolver_core_tests
    ${RESOLVER_CORE_TEST_SRCS}
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)
target_link_libraries(resolver_core_tests PRIVATE 
    test_utilities
    reference_resolver_stubs
    ${Criterion_LIBRARIES}
    parser_core
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
target_include_directories(resolver_core_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/core/include
    ${PROJECT_SOURCE_DIR}/core/include/scopemux
    ${Criterion_INCLUDE_DIRS}
)

# Resolver Registration Tests
add_executable(resolver_registration_tests
    ${RESOLVER_REGISTRATION_TEST_SRCS}
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)
target_link_libraries(resolver_registration_tests PRIVATE 
    test_utilities
    reference_resolver_stubs
    ${Criterion_LIBRARIES}
    parser_core
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
target_include_directories(resolver_registration_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/core/include
    ${PROJECT_SOURCE_DIR}/core/include/scopemux
    ${Criterion_INCLUDE_DIRS}
)

# Resolver Resolution Tests
add_executable(resolver_resolution_tests
    ${RESOLVER_RESOLUTION_TEST_SRCS}
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)
target_link_libraries(resolver_resolution_tests PRIVATE 
    test_utilities
    reference_resolver_stubs
    ${Criterion_LIBRARIES}
    parser_core
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
target_include_directories(resolver_resolution_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/core/include
    ${PROJECT_SOURCE_DIR}/core/include/scopemux
    ${Criterion_INCLUDE_DIRS}
)

# Language Resolver Tests
add_executable(language_resolver_tests
    ${LANGUAGE_RESOLVER_TEST_SRCS}
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)
target_link_libraries(language_resolver_tests PRIVATE 
    test_utilities
    reference_resolver_stubs
    ${Criterion_LIBRARIES}
    parser_core
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
target_include_directories(language_resolver_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/core/include
    ${PROJECT_SOURCE_DIR}/core/include/scopemux
    ${Criterion_INCLUDE_DIRS}
)

# Symbol Table Tests
add_executable(symbol_table_tests
    ${SYMBOL_TABLE_TEST_SOURCES}
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)
target_link_libraries(symbol_table_tests PRIVATE 
    test_utilities
    ${Criterion_LIBRARIES}
    parser_core
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
target_include_directories(symbol_table_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/core/include
    ${PROJECT_SOURCE_DIR}/core/include/scopemux
    ${Criterion_INCLUDE_DIRS}
)

# Project Context Tests
add_executable(project_context_tests
    ${PROJECT_CONTEXT_TEST_SOURCES}
    ${SCOPEMUX_CORE_C_SOURCES_FOR_TESTS}
)
target_link_libraries(project_context_tests PRIVATE 
    test_utilities
    ${Criterion_LIBRARIES}
    parser_core
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
target_include_directories(project_context_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/core/include
    ${PROJECT_SOURCE_DIR}/core/include/scopemux
    ${Criterion_INCLUDE_DIRS}
)

# --- Macro to create test executables ---
# Macro now takes test target and its source variable
macro(create_test_executable test_name source_var)
    add_executable(${test_name} ${${source_var}})
    target_link_libraries(${test_name} PRIVATE
        test_utilities
        parser_core
        ${Criterion_LIBRARIES}
        m
        dl
        ts_core
        ts_c
        ts_cpp
        ts_python
        ts_javascript
        ts_typescript
    )
    target_include_directories(${test_name} PRIVATE
        ${PROJECT_SOURCE_DIR}/core/include
        ${PROJECT_SOURCE_DIR}/core/include/scopemux
    )
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
