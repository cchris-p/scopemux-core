/**
 * @file memory_debug.h
 * @brief Memory debugging and validation utilities for ScopeMux
 *
 * This module provides tools for debugging memory issues, tracking allocations,
 * detecting memory corruption, and validating pointers.
 */

#ifndef SCOPEMUX_MEMORY_DEBUG_H
#define SCOPEMUX_MEMORY_DEBUG_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Configure the memory debugger
 * 
 * @param enable_tracking Enable allocation tracking
 * @param enable_bounds_check Enable bounds checking for allocations
 * @param enable_leak_detection Enable leak detection at shutdown
 */
void memory_debug_configure(bool enable_tracking, bool enable_bounds_check, bool enable_leak_detection);

/**
 * @brief Initialize the memory debugging subsystem
 * 
 * Call this early in the program to enable tracking
 */
void memory_debug_init(void);

/**
 * @brief Clean up the memory debugging subsystem
 * 
 * Call this at program exit to check for leaks
 */
void memory_debug_cleanup(void);

/**
 * @brief Track a memory allocation
 * 
 * @param ptr Pointer to the allocated memory
 * @param size Size of the allocation
 * @param file Source file where allocation occurred
 * @param line Line number where allocation occurred
 * @param tag Optional tag to identify the allocation type (e.g., "ASTNode")
 */
void memory_debug_track(void *ptr, size_t size, const char *file, int line, const char *tag);

/**
 * @brief Untrack a memory allocation before freeing
 * 
 * @param ptr Pointer to the allocated memory
 * @param file Source file where deallocation occurred
 * @param line Line number where deallocation occurred
 */
void memory_debug_untrack(void *ptr, const char *file, int line);

/**
 * @brief Verify that a pointer is valid
 * 
 * @param ptr Pointer to check
 * @return true if pointer is valid, false otherwise
 */
bool memory_debug_is_valid_ptr(const void *ptr);

/**
 * @brief Check if a pointer is within a specified memory range
 *
 * @param ptr Pointer to check
 * @param start Start of the memory range
 * @param size Size of the memory range
 * @return true if pointer is within range, false otherwise
 */
bool memory_debug_ptr_in_range(const void *ptr, const void *start, size_t size);

/**
 * @brief Print current memory allocation statistics
 */
void memory_debug_print_stats(void);

/**
 * @brief Print details of all currently tracked allocations
 */
void memory_debug_dump_allocations(void);

/**
 * @brief Check a specific memory block for corruption
 * 
 * @param ptr Pointer to the memory block
 * @return true if memory is intact, false if corrupted
 */
bool memory_debug_check_corruption(void *ptr);

/**
 * @brief Set a memory canary pattern at the end of the allocation
 * 
 * This adds a pattern beyond the requested size that can be checked
 * to detect buffer overflows.
 * 
 * @param ptr Pointer to the memory block
 * @param size Size of the actual allocation (not including canary)
 */
void memory_debug_set_canary(void *ptr, size_t size);

/**
 * @brief Check if the memory canary is intact
 * 
 * @param ptr Pointer to the memory block
 * @param size Size of the actual allocation (not including canary)
 * @return true if canary is intact, false if overwritten
 */
bool memory_debug_check_canary(void *ptr, size_t size);

/**
 * @brief Wrapper macros for memory management with automatic tracking
 */
#ifdef SCOPEMUX_DEBUG_MEMORY

#define MALLOC(size, tag) memory_debug_malloc(size, __FILE__, __LINE__, tag)
#define CALLOC(nmemb, size, tag) memory_debug_calloc(nmemb, size, __FILE__, __LINE__, tag)
#define REALLOC(ptr, size, tag) memory_debug_realloc(ptr, size, __FILE__, __LINE__, tag)
#define FREE(ptr) memory_debug_free(ptr, __FILE__, __LINE__)
#define STRDUP(s, tag) memory_debug_strdup(s, __FILE__, __LINE__, tag)

#else

#define MALLOC(size, tag) malloc(size)
#define CALLOC(nmemb, size, tag) calloc(nmemb, size)
#define REALLOC(ptr, size, tag) realloc(ptr, size)
#define FREE(ptr) free(ptr)
#define STRDUP(s, tag) strdup(s)

#endif

/**
 * @brief Tracked allocation functions
 */
void *memory_debug_malloc(size_t size, const char *file, int line, const char *tag);
void *memory_debug_calloc(size_t nmemb, size_t size, const char *file, int line, const char *tag);
void *memory_debug_realloc(void *ptr, size_t size, const char *file, int line, const char *tag);
void memory_debug_free(void *ptr, const char *file, int line);
char *memory_debug_strdup(const char *s, const char *file, int line, const char *tag);
char *memory_debug_strndup(const char *s, size_t n, const char *file, int line, const char *tag);

#endif /* SCOPEMUX_MEMORY_DEBUG_H */
