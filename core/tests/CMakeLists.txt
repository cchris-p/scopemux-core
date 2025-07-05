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
)

add_library(test_parser STATIC
    src/init_parser_tests.c
    src/edge_case_tests.c
)

# Link test libraries with core libraries
target_link_libraries(test_parser PRIVATE parser_core)
target_link_libraries(test_utilities PRIVATE parser_core)

# Set include directories for test targets
set_common_test_includes(test_parser)
set_common_test_includes(test_utilities)

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

set_common_test_includes(run_tests)

# Installation rules
install(TARGETS run_tests DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

message(STATUS "Configured Scopemux Bindings C Tests with Criterion.")
message(STATUS "Copied example files and expected JSON to build directory for validation.") 