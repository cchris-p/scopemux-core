/**
 * @file memory_management.h
 * @brief Safe memory management utilities for ScopeMux
 *
 * Provides wrappers for memory allocation, reallocation, freeing, and string duplication,
 * as well as memory pool management. All functions are designed to improve safety and
 * debugging in C code.
 */

#ifndef SCOPEMUX_MEMORY_MANAGEMENT_H
#define SCOPEMUX_MEMORY_MANAGEMENT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Allocate memory safely.
 *
 * @param size Number of bytes to allocate.
 * @return Pointer to allocated memory, or NULL on failure.
 */
void *safe_malloc(size_t size);

/**
 * @brief Reallocate memory safely.
 *
 * @param ptr Pointer to previously allocated memory.
 * @param size New size in bytes.
 * @return Pointer to reallocated memory, or NULL on failure.
 */
void *safe_realloc(void *ptr, size_t size);

/**
 * @brief Free memory safely.
 *
 * @param ptr Pointer to memory to free.
 */
void safe_free(void *ptr);

/**
 * @brief Duplicate a string safely.
 *
 * @param str String to duplicate.
 * @return Pointer to duplicated string, or NULL on failure.
 */
char *safe_strdup(const char *str);

/**
 * @brief Initialize a memory pool.
 *
 * @param size Size of the pool in bytes.
 * @return Pointer to the memory pool, or NULL on failure.
 */
void *memory_pool_init(size_t size);

/**
 * @brief Allocate memory from a memory pool.
 *
 * @param pool Pointer to the memory pool.
 * @param size Number of bytes to allocate.
 * @return Pointer to allocated memory, or NULL on failure.
 */
void *memory_pool_alloc(void *pool, size_t size);

/**
 * @brief Free a memory pool.
 *
 * @param pool Pointer to the memory pool to free.
 */
void memory_pool_free(void *pool);

#ifdef __cplusplus
}
#endif

#endif /* SCOPEMUX_MEMORY_MANAGEMENT_H */
