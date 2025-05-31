cmake_minimum_required(VERSION 3.10)

# Set CMake policy version to ensure compatibility with dependencies like pybind11
# This should be set before the project() call.
if(POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW) # Handles project() version format
endif()
if(POLICY CMP0148)
    cmake_policy(SET CMP0148 NEW) # Handles modern Python discovery (FindPythonInterp/FindPythonLibs removal)
endif()
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.5") # Check if CMAKE_POLICY_VERSION_MINIMUM is supported
    set(CMAKE_POLICY_VERSION_MINIMUM 3.10) # Match our project's minimum
endif()

project(scopemux VERSION 0.1.0 LANGUAGES C CXX)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Set compiler flags
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
endif()

# Find Python packages needed for the project
find_package(Python 3.10 REQUIRED COMPONENTS Interpreter Development)

# If find_package doesn't work properly, use the known system include path
if(NOT Python_INCLUDE_DIRS)
    set(PYTHON_INCLUDE_DIRS "/usr/include/python3.10")
else()
    set(PYTHON_INCLUDE_DIRS ${Python_INCLUDE_DIRS})
endif()

message(STATUS "Using Python include dirs: ${PYTHON_INCLUDE_DIRS}")


# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/include
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/include/scopemux
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/tree-sitter/lib/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/tree-sitter-c/src
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/tree-sitter-cpp/src
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/tree-sitter-python/src
    ${PYTHON_INCLUDE_DIRS}
)

# Add preprocessor defines for proper compilation
add_compile_definitions(
    PY_SSIZE_T_CLEAN
)

# Use FetchContent for pybind11
include(FetchContent)
FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11
    GIT_TAG        v2.10.4
)
FetchContent_MakeAvailable(pybind11)

# Define Tree-sitter core library
set(TS_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/tree-sitter/lib/src)
set(TS_CORE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/tree-sitter/lib/include)

# Define Tree-sitter grammar sources
set(TS_C_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/tree-sitter-c/src)
set(TS_CPP_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/tree-sitter-cpp/src)
set(TS_PYTHON_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/tree-sitter-python/src)

# Tree-sitter core sources
set(TS_CORE_SOURCES
    ${TS_CORE_DIR}/alloc.c
    ${TS_CORE_DIR}/get_changed_ranges.c
    ${TS_CORE_DIR}/language.c
    ${TS_CORE_DIR}/lexer.c
    ${TS_CORE_DIR}/lib.c
    ${TS_CORE_DIR}/node.c
    ${TS_CORE_DIR}/parser.c
    ${TS_CORE_DIR}/query.c
    ${TS_CORE_DIR}/stack.c
    ${TS_CORE_DIR}/subtree.c
    ${TS_CORE_DIR}/tree.c
    ${TS_CORE_DIR}/tree_cursor.c
    ${TS_CORE_DIR}/wasm_store.c
)

# Tree-sitter grammar sources
set(TS_GRAMMAR_SOURCES
    ${TS_C_SRC_DIR}/parser.c
    ${TS_CPP_SRC_DIR}/parser.c
    ${TS_CPP_SRC_DIR}/scanner.c
    ${TS_PYTHON_SRC_DIR}/parser.c
    ${TS_PYTHON_SRC_DIR}/scanner.c
)

# Create a library for the Tree-sitter core
add_library(tree-sitter-core STATIC ${TS_CORE_SOURCES})
target_include_directories(tree-sitter-core PUBLIC ${TS_CORE_INCLUDE_DIR})

# Create a library for the Tree-sitter grammar parsers
add_library(tree-sitter-grammars STATIC ${TS_GRAMMAR_SOURCES})
target_include_directories(tree-sitter-grammars PUBLIC
    ${TS_CORE_INCLUDE_DIR}
    ${TS_C_SRC_DIR}
    ${TS_CPP_SRC_DIR}
    ${TS_PYTHON_SRC_DIR}
)
target_link_libraries(tree-sitter-grammars PRIVATE tree-sitter-core)

# Add extern declarations for each language parser
target_compile_definitions(tree-sitter-grammars PUBLIC
    TREE_SITTER_PARSERS_AVAILABLE=1
)

# Define source files
set(SCOPEMUX_SOURCES
    bindings/src/bindings/module.c
    bindings/src/bindings/parser_bindings.c
    bindings/src/parser/parser.c
    bindings/src/parser/tree_sitter_integration.c
    bindings/src/parser/ir_generator.c
    bindings/src/context_engine/compressor.c
    bindings/src/context_engine/expander.c
    bindings/src/context_engine/token_budgeter.c
    bindings/src/common/error_handling.c
    bindings/src/common/logging.c
    bindings/src/common/memory_management.c
    # Tree-sitter sources are now in separate libraries
)

# Python bindings module
pybind11_add_module(scopemux_core ${SCOPEMUX_SOURCES})

# Set include directories for the Python module
target_include_directories(scopemux_core PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/include
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/include/scopemux
    ${TS_CORE_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
)

# Link Tree-sitter libraries to the Python module
target_link_libraries(scopemux_core PRIVATE
    tree-sitter-core
    tree-sitter-grammars
)

# Add the test suite subdirectory
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bindings/tests/CMakeLists.txt")
    add_subdirectory(bindings/tests)
endif()

# Installation rules
install(TARGETS scopemux_core DESTINATION ${CMAKE_INSTALL_PREFIX})

# Print configuration summary
message(STATUS "Configuration summary:")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "  Python include dirs: ${PYTHON_INCLUDE_DIRS}")
message(STATUS "  Installation prefix: ${CMAKE_INSTALL_PREFIX}")
