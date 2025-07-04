/**
 * @file test_compiler_flags.h
 * @brief Compiler-specific pragmas to suppress warnings in test code
 *
 * This header provides macros to disable specific compiler warnings that
 * are common in test code but don't affect functionality.
 */

#ifndef SCOPEMUX_TEST_COMPILER_FLAGS_H
#define SCOPEMUX_TEST_COMPILER_FLAGS_H

#ifdef __GNUC__
  /* Begin suppressing warnings */
  #define SCOPEMUX_SUPPRESS_TEST_WARNINGS_BEGIN \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wstrict-prototypes\"") \
    _Pragma("GCC diagnostic ignored \"-Wmissing-prototypes\"") \
    _Pragma("GCC diagnostic ignored \"-Wpedantic\"") \
    _Pragma("GCC diagnostic ignored \"-Wunused-parameter\"") \
    _Pragma("GCC diagnostic ignored \"-Wunused-function\"") \
    _Pragma("GCC diagnostic ignored \"-Wunused-variable\"") \
    _Pragma("GCC diagnostic ignored \"-Wincompatible-pointer-types\"")

  /* Restore warnings */
  #define SCOPEMUX_SUPPRESS_TEST_WARNINGS_END \
    _Pragma("GCC diagnostic pop")
#else
  /* For non-GCC compilers, define empty macros */
  #define SCOPEMUX_SUPPRESS_TEST_WARNINGS_BEGIN
  #define SCOPEMUX_SUPPRESS_TEST_WARNINGS_END
#endif

#endif /* SCOPEMUX_TEST_COMPILER_FLAGS_H */
