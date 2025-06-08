/**
 * @file memory_management.c
 * @brief Implementation of memory management utilities for ScopeMux
 *
 * This module provides utilities for memory management, including
 * allocation, deallocation, and memory pool management.
 */

#include <stdlib.h>
#include <string.h>

/**
 * @brief Allocate memory with error checking
 *
 * @param size Size of memory to allocate
 * @return void* Pointer to allocated memory or NULL on failure
 */
void *safe_malloc(size_t size) {
  void *ptr = malloc(size);
  if (ptr == NULL) {
    // Handle out of memory error
    // In a real implementation, we might log this or set an error
  }
  return ptr;
}

/**
 * @brief Reallocate memory with error checking
 *
 * @param ptr Pointer to memory to reallocate
 * @param size New size
 * @return void* Pointer to reallocated memory or NULL on failure
 */
void *safe_realloc(void *ptr, size_t size) {
  void *new_ptr = realloc(ptr, size);
  if (new_ptr == NULL && size > 0) {
    // Handle out of memory error
    // In a real implementation, we might log this or set an error

    // Note: We don't free ptr here, as realloc keeps the original
    // allocation intact if it fails
  }
  return new_ptr;
}

/**
 * @brief Free memory safely (handles NULL)
 *
 * @param ptr Pointer to memory to free
 */
void safe_free(void *ptr) {
  if (ptr != NULL) {
    free(ptr);
  }
}

/**
 * @brief Duplicate a string with error checking
 *
 * @param str String to duplicate
 * @return char* Pointer to duplicated string or NULL on failure
 */
char *safe_strdup(const char *str) {
  if (str == NULL) {
    return NULL;
  }

  size_t len = strlen(str) + 1;
  char *dup = (char *)safe_malloc(len);
  if (dup != NULL) {
    memcpy(dup, str, len);
  }
  return dup;
}

/**
 * @brief Initialize a memory pool for temporary allocations
 *
 * @param size Size of the memory pool
 * @return void* Pointer to the memory pool or NULL on failure
 */
void *memory_pool_init(size_t size) {
  // TODO: Implement memory pool initialization
  // This is a placeholder for a more advanced memory pool implementation
  return safe_malloc(size);
}

/**
 * @brief Allocate from a memory pool
 *
 * @param pool Memory pool
 * @param size Size to allocate
 * @return void* Pointer to allocated memory or NULL on failure
 */
void *memory_pool_alloc(void *pool, size_t size) {
  // TODO: Implement memory pool allocation
  // This is a placeholder for a more advanced memory pool implementation
  return safe_malloc(size);
}

/**
 * @brief Free a memory pool
 *
 * @param pool Memory pool to free
 */
void memory_pool_free(void *pool) {
  // TODO: Implement memory pool cleanup
  // This is a placeholder for a more advanced memory pool implementation
  safe_free(pool);
}
