cmake_minimum_required(VERSION 3.10)
project(scopemux VERSION 0.1.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Set compiler flags
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
endif()

# Specify Python include directories directly
set(PYTHON_INCLUDE_DIRS "/home/matrillo/.pyenv/versions/scopemux/include/python3.10")
message(STATUS "Using Python include dirs: ${PYTHON_INCLUDE_DIRS}")

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/include
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/include/scopemux
    ${TS_CORE_INCLUDE_DIR}
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
    ${TS_CORE_DIR}/language.c
    ${TS_CORE_DIR}/lexer.c
    ${TS_CORE_DIR}/node.c
    ${TS_CORE_DIR}/parser.c
    ${TS_CORE_DIR}/tree.c
    ${TS_CORE_DIR}/tree_cursor.c
)

# Tree-sitter grammar sources
set(TS_GRAMMAR_SOURCES
    ${TS_C_SRC_DIR}/parser.c
    ${TS_CPP_SRC_DIR}/parser.c
    ${TS_CPP_SRC_DIR}/scanner.cc
    ${TS_PYTHON_SRC_DIR}/parser.c
    ${TS_PYTHON_SRC_DIR}/scanner.c
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
    ${TS_CORE_SOURCES}
    ${TS_GRAMMAR_SOURCES}
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

# Installation rules
install(TARGETS scopemux_core DESTINATION ${CMAKE_INSTALL_PREFIX})

# Print configuration summary
message(STATUS "Configuration summary:")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "  Python include dirs: ${PYTHON_INCLUDE_DIRS}")
message(STATUS "  Installation prefix: ${CMAKE_INSTALL_PREFIX}")
