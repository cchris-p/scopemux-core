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

# Define source files
set(SCOPEMUX_SOURCES
    bindings/src/bindings/module.c
    bindings/src/bindings/parser_bindings.c
)

# Python bindings module
pybind11_add_module(scopemux_core ${SCOPEMUX_SOURCES})

# Set include directories for the Python module
target_include_directories(scopemux_core PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/include
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/include/scopemux
    ${PYTHON_INCLUDE_DIRS}
)

# Installation rules
install(TARGETS scopemux_core DESTINATION ${CMAKE_INSTALL_PREFIX})

# Print configuration summary
message(STATUS "Configuration summary:")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "  Python include dirs: ${PYTHON_INCLUDE_DIRS}")
message(STATUS "  Installation prefix: ${CMAKE_INSTALL_PREFIX}")
