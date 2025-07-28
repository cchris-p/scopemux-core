cmake_minimum_required(VERSION 3.14)
project(scopemux_core)

# Set compiler flags for this directory
# Note: C/C++ standards are already set in the root CMakeLists.txt

# Enhanced compiler flags for memory safety and early error detection
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

# Find Python packages needed for the project
find_package(Python 3.10...3.11 REQUIRED COMPONENTS Interpreter Development)

# Fetch pybind11 first, before we use any of its variables
include(FetchContent)
FetchContent_Declare(
    pybind11
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/pybind11
)
FetchContent_MakeAvailable(pybind11)

# Include directories - make sure all targets can find the headers
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
add_library(parser_core_obj OBJECT
    # Refactored parser components
    src/parser/parser.c
    src/parser/ast_node.c
    src/parser/cst_node.c
    src/parser/memory_tracking.c
    src/parser/parser_context.c
    src/parser/query_processing.c
    # Project context components
    src/parser/project_context/project_utils.c
    src/parser/project_context/project_symbol_extraction.c
    # Reference resolver components
    src/parser/reference_resolvers/reference_resolver_core.c
    src/parser/reference_resolvers/resolver_core.c
    src/parser/reference_resolvers/resolver_implementation.c
    src/parser/reference_resolvers/resolver_registration.c
    src/parser/reference_resolvers/resolver_resolution.c
    src/parser/project_context/symbol_management.c
    src/parser/project_context/symbol_collection.c
    src/parser/project_context/file_management.c
    src/parser/project_context/dependency_management.c
    src/parser/project_context.c
    # Tree-sitter integration
    src/parser/ts_init.c
    src/parser/ts_ast_builder.c
    src/parser/ts_cst_builder.c
    src/parser/ts_query_processor.c
    src/parser/tree_sitter_integration.c
    src/parser/query_manager.c
    # Reference resolver system (add all .c files)
    src/parser/reference_resolvers/language_resolvers.c
    src/parser/reference_resolvers/resolver_registration.c
    src/parser/reference_resolvers/resolver_core.c

    src/parser/reference_resolvers/reference_resolver_core.c
    src/parser/reference_resolvers/c_resolver.c
    src/parser/reference_resolvers/cpp_resolver.c
    src/parser/reference_resolvers/c_cpp_resolver_shared_utils.c
    src/parser/reference_resolvers/typescript_resolver.c
    src/parser/reference_resolvers/python_resolver.c
    src/parser/reference_resolvers/javascript_resolver.c
    src/parser/reference_resolvers/js_ts_resolver_shared_utils.c
    # Reference resolver components
    src/parser/reference_resolvers/reference_resolver_core.c
    src/parser/reference_resolvers/resolver_core.c
    src/parser/reference_resolvers/resolver_registration.c
    src/parser/reference_resolvers/resolver_resolution.c
    src/parser/reference_resolver.c
    # Language compliance system (add all .c files)
    src/ast_compliance.c
    src/parser/lang/c_compliance.c
    src/parser/lang/lang_compliance_registry.c
    src/parser/lang/python_ast_compliance.c
    src/parser/lang/javascript_ast_compliance.c
    src/parser/lang/typescript_ast_compliance.c
    src/parser/lang/c_ast_compliance.c
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
    # Symbol table components
    src/parser/symbol_table.c
    src/parser/symbol_table/symbol_core.c
    src/parser/symbol_table/symbol_entry.c
    src/parser/symbol_table/symbol_lookup.c
    src/parser/symbol_table/symbol_table_registration.c
)
set_target_properties(parser_core_obj PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(parser_core STATIC $<TARGET_OBJECTS:parser_core_obj>)

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
    src/context_engine/context_engine.c
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

# Set properties for context_engine
set_target_properties(context_engine PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    SKIP_DEPENDENCY_GENERATION TRUE
)

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
    $<TARGET_OBJECTS:parser_core_obj>
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
    -ldl
)

# Add explicit linker flags to force export of symbols and include whole archive for parser_core
# This ensures all symbols from parser_core (including segfault_handler) are included
# regardless of whether they're directly referenced


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

# Add target_link_options block for scopemux_core to force --whole-archive inclusion of all Tree-sitter static libraries (ts_c, ts_cpp, ts_python, ts_javascript, ts_typescript)
if(TARGET ts_c AND TARGET ts_cpp AND TARGET ts_python AND TARGET ts_javascript AND TARGET ts_typescript)
  target_link_options(scopemux_core PRIVATE
    -Wl,--whole-archive
    -Wl,$<TARGET_FILE:ts_c>
    -Wl,$<TARGET_FILE:ts_cpp>
    -Wl,$<TARGET_FILE:ts_python>
    -Wl,$<TARGET_FILE:ts_javascript>
    -Wl,$<TARGET_FILE:ts_typescript>
    -Wl,--no-whole-archive
  )
endif()

# Add target_link_options block for scopemux_core to force --whole-archive inclusion of context_engine
target_link_options(scopemux_core PRIVATE
    -Wl,--whole-archive
    -Wl,$<TARGET_FILE:context_engine>
    -Wl,--no-whole-archive
)

# Installation rules
install(TARGETS scopemux_core DESTINATION ${CMAKE_INSTALL_PREFIX})

# Add subdirectories for other components if they exist
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/adapters)
    add_subdirectory(src/adapters)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/config)
    add_subdirectory(src/config)
endif()

# Print configuration summary
message(STATUS "Configuration summary:")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "  Python: ${Python_VERSION}")
message(STATUS "  Installation prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Memory debugging: ${SCOPEMUX_DEBUG_MEMORY}")
message(STATUS "  Valgrind compatibility: ${SCOPEMUX_VALGRIND_COMPATIBLE}")
