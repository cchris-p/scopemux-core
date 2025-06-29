cmake_minimum_required(VERSION 3.16)
project(scopemux_project VERSION 0.1.0 LANGUAGES C CXX)

# Set C and C++ standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include the ExternalProject module
include(ExternalProject)

# Define source directories
set(VENDOR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor)
set(TS_DIR ${VENDOR_DIR}/tree-sitter)
set(TS_C_DIR ${VENDOR_DIR}/tree-sitter-c)
set(TS_CPP_DIR ${VENDOR_DIR}/tree-sitter-cpp)
set(TS_PYTHON_DIR ${VENDOR_DIR}/tree-sitter-python)
set(TS_JS_DIR ${VENDOR_DIR}/tree-sitter-javascript)
set(TS_TS_DIR ${VENDOR_DIR}/tree-sitter-typescript/typescript)
set(TS_LIB_DIR ${CMAKE_CURRENT_BINARY_DIR}/tree-sitter-libs)

# Create directory for Tree-sitter libraries
file(MAKE_DIRECTORY ${TS_LIB_DIR})

# Define paths for the built Tree-sitter libraries
# Static libraries
set(TS_CORE_LIB ${TS_LIB_DIR}/libtree-sitter.a)
set(TS_C_LIB ${TS_LIB_DIR}/libtree-sitter-c.a)
set(TS_CPP_LIB ${TS_LIB_DIR}/libtree-sitter-cpp.a)
set(TS_PYTHON_LIB ${TS_LIB_DIR}/libtree-sitter-python.a)
set(TS_JS_LIB ${TS_LIB_DIR}/libtree-sitter-javascript.a)
set(TS_TS_LIB ${TS_LIB_DIR}/libtree-sitter-typescript.a)

# Shared libraries
set(TS_CORE_SO ${TS_LIB_DIR}/libtree-sitter.so)
set(TS_C_SO ${TS_LIB_DIR}/libtree-sitter-c.so)
set(TS_CPP_SO ${TS_LIB_DIR}/libtree-sitter-cpp.so)
set(TS_PYTHON_SO ${TS_LIB_DIR}/libtree-sitter-python.so)
set(TS_JS_SO ${TS_LIB_DIR}/libtree-sitter-javascript.so)
set(TS_TS_SO ${TS_LIB_DIR}/libtree-sitter-typescript.so)

# Build the main Tree-sitter library
ExternalProject_Add(tree_sitter_core
    SOURCE_DIR ${TS_DIR}
    CONFIGURE_COMMAND ""  # No configure step required
    BUILD_COMMAND make -C ${TS_DIR} CFLAGS="-fPIC"
    BUILD_IN_SOURCE 1
    INSTALL_COMMAND ${CMAKE_COMMAND} -E copy
        ${TS_DIR}/libtree-sitter.a
        ${TS_CORE_LIB}
    BUILD_BYPRODUCTS ${TS_CORE_LIB}
)

# Build the Tree-sitter C grammar
ExternalProject_Add(tree_sitter_c
    SOURCE_DIR ${TS_C_DIR}
    CONFIGURE_COMMAND ""  # No configure step required
    BUILD_COMMAND make -C ${TS_C_DIR} CFLAGS="-fPIC"
    BUILD_IN_SOURCE 1
    INSTALL_COMMAND ${CMAKE_COMMAND} -E copy
        ${TS_C_DIR}/libtree-sitter-c.a
        ${TS_C_LIB}
    BUILD_BYPRODUCTS ${TS_C_LIB}
    DEPENDS tree_sitter_core
)

# Build the Tree-sitter C++ grammar
ExternalProject_Add(tree_sitter_cpp
    SOURCE_DIR ${TS_CPP_DIR}
    CONFIGURE_COMMAND ""  # No configure step required
    BUILD_COMMAND make -C ${TS_CPP_DIR} CFLAGS="-fPIC"
    BUILD_IN_SOURCE 1
    INSTALL_COMMAND ${CMAKE_COMMAND} -E copy
        ${TS_CPP_DIR}/libtree-sitter-cpp.a
        ${TS_CPP_LIB}
    BUILD_BYPRODUCTS ${TS_CPP_LIB}
    DEPENDS tree_sitter_core
)

# Build the Tree-sitter Python grammar
ExternalProject_Add(tree_sitter_python
    SOURCE_DIR ${TS_PYTHON_DIR}
    CONFIGURE_COMMAND ""  # No configure step required
    BUILD_COMMAND make -C ${TS_PYTHON_DIR} CFLAGS="-fPIC"
    BUILD_IN_SOURCE 1
    INSTALL_COMMAND ${CMAKE_COMMAND} -E copy
        ${TS_PYTHON_DIR}/libtree-sitter-python.a
        ${TS_PYTHON_LIB}
    BUILD_BYPRODUCTS ${TS_PYTHON_LIB}
    DEPENDS tree_sitter_core
)

# Build the Tree-sitter JavaScript grammar
ExternalProject_Add(tree_sitter_javascript
    SOURCE_DIR ${TS_JS_DIR}
    CONFIGURE_COMMAND ""  # No configure step required
    BUILD_COMMAND make -C ${TS_JS_DIR} CFLAGS="-fPIC"
    BUILD_IN_SOURCE 1
    INSTALL_COMMAND ${CMAKE_COMMAND} -E copy
        ${TS_JS_DIR}/libtree-sitter-javascript.a
        ${TS_JS_LIB}
    BUILD_BYPRODUCTS ${TS_JS_LIB}
    DEPENDS tree_sitter_core
)

# Build the Tree-sitter TypeScript grammar
ExternalProject_Add(tree_sitter_typescript
    SOURCE_DIR ${TS_TS_DIR}
    CONFIGURE_COMMAND ""  # No configure step required
    BUILD_COMMAND make -C ${TS_TS_DIR} CFLAGS="-fPIC"
    BUILD_IN_SOURCE 1
    INSTALL_COMMAND ${CMAKE_COMMAND} -E copy
        ${TS_TS_DIR}/libtree-sitter-typescript.a
        ${TS_TS_LIB}
    BUILD_BYPRODUCTS ${TS_TS_LIB}
    DEPENDS tree_sitter_core
)

# Create imported library targets for Tree-sitter libraries
add_library(ts_core STATIC IMPORTED)
set_target_properties(ts_core PROPERTIES
    IMPORTED_LOCATION ${TS_CORE_LIB}
)
add_dependencies(ts_core tree_sitter_core)

add_library(ts_c STATIC IMPORTED)
set_target_properties(ts_c PROPERTIES
    IMPORTED_LOCATION ${TS_C_LIB}
)
add_dependencies(ts_c tree_sitter_c)

add_library(ts_cpp STATIC IMPORTED)
set_target_properties(ts_cpp PROPERTIES
    IMPORTED_LOCATION ${TS_CPP_LIB}
)
add_dependencies(ts_cpp tree_sitter_cpp)

add_library(ts_python STATIC IMPORTED)
set_target_properties(ts_python PROPERTIES
    IMPORTED_LOCATION ${TS_PYTHON_LIB}
)
add_dependencies(ts_python tree_sitter_python)

add_library(ts_javascript STATIC IMPORTED)
set_target_properties(ts_javascript PROPERTIES
    IMPORTED_LOCATION ${TS_JS_LIB}
)
add_dependencies(ts_javascript tree_sitter_javascript)

add_library(ts_typescript STATIC IMPORTED)
set_target_properties(ts_typescript PROPERTIES
    IMPORTED_LOCATION ${TS_TS_LIB}
)
add_dependencies(ts_typescript tree_sitter_typescript)

# Export variables for use by subdirectories
set(TS_CORE_INCLUDE_DIR ${TS_DIR}/lib/include CACHE FILEPATH "Tree-sitter core include directory")
set(TS_C_INCLUDE_DIR ${TS_C_DIR}/src CACHE FILEPATH "Tree-sitter C include directory")
set(TS_CPP_INCLUDE_DIR ${TS_CPP_DIR}/src CACHE FILEPATH "Tree-sitter C++ include directory")
set(TS_PYTHON_INCLUDE_DIR ${TS_PYTHON_DIR}/src CACHE FILEPATH "Tree-sitter Python include directory")
set(TS_JS_INCLUDE_DIR ${TS_JS_DIR}/src CACHE FILEPATH "Tree-sitter JavaScript include directory")
set(TS_TS_INCLUDE_DIR ${TS_TS_DIR}/src CACHE FILEPATH "Tree-sitter TypeScript include directory")

# Finally add the main project subdirectory
add_subdirectory(core)

# Print configuration summary
message(STATUS "Configuration summary:")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "  C++ Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Tree-sitter core libraries built to: ${TS_LIB_DIR}")
