# Set compiler flags for this directory
# Note: C/C++ standards are already set in the root CMakeLists.txt

# Set compiler flags
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic")
endif()

# Find Python packages needed for the project
find_package(Python 3.10 REQUIRED COMPONENTS Interpreter Development)

# Fetch pybind11 first, before we use any of its variables
include(FetchContent)
FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11
    GIT_TAG v2.11.1  # Updated to latest version to fix CMake compatibility issues
)
FetchContent_MakeAvailable(pybind11)

# Include directories - make sure all targets can find the headers
include_directories(
    # Tree-sitter headers (defined in the root CMakeLists.txt)
    ${TS_CORE_INCLUDE_DIR}
    ${TS_C_INCLUDE_DIR}
    ${TS_CPP_INCLUDE_DIR}
    ${TS_PYTHON_INCLUDE_DIR}
    # JavaScript and TypeScript support removed temporarily
    # ${TS_JS_INCLUDE_DIR}
    # ${TS_TS_INCLUDE_DIR}
    # Project headers
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/scopemux
    # Python headers
    ${Python_INCLUDE_DIRS}
    ${pybind11_INCLUDE_DIRS}
)

# Add preprocessor defines for proper compilation
add_compile_definitions(
    SCOPEMUX_BUILDING=1
    Py_LIMITED_API=0x03070000
    PY_SSIZE_T_CLEAN
)

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

# Define source files
set(PARSER_SOURCES
    src/parser/parser.c
    src/parser/tree_sitter_integration.c
    # src/parser/ir_generator.c  # Commented out missing file
)

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

# Add library targets
add_library(parser_core STATIC ${PARSER_SOURCES} ${COMMON_SOURCES})
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
    # JavaScript and TypeScript support removed temporarily
    # ts_javascript
    # ts_typescript
)

# Python bindings module
pybind11_add_module(scopemux_core
    src/bindings/module.c
    src/bindings/parser_bindings.c
    src/bindings/context_engine_bindings.c
)

# Link libraries to the Python module
target_link_libraries(scopemux_core PRIVATE parser_core context_engine)

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

# Add subdirectories for tests and examples if they exist
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    add_subdirectory(tests)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/examples)
    add_subdirectory(examples)
endif()

# Print configuration summary
message(STATUS "Configuration summary:")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "  Python: ${Python_VERSION}")
message(STATUS "  Installation prefix: ${CMAKE_INSTALL_PREFIX}")
