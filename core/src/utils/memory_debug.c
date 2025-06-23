/**
 * @file memory_debug.c
 * @brief Implementation of memory debugging and validation utilities
 *
 * This module provides runtime tools for tracking memory allocations,
 * detecting memory corruption, validating pointers, and identifying memory leaks.
 * It can be used alongside Valgrind for comprehensive memory safety analysis.
 */

#include "../../core/include/scopemux/memory_debug.h"
#include "../../core/include/scopemux/logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>

// Define the canary pattern for detecting buffer overflows
#define CANARY_PATTERN 0xCACA5E5E
#define CANARY_SIZE 8
#define MAX_TRACKED_ALLOCATIONS 10000
#define MAX_TAG_LENGTH 32

// Configuration flags
static bool tracking_enabled = false;
static bool bounds_check_enabled = false;
static bool leak_detection_enabled = false;
static bool initialized = false;

// Mutex for thread safety
static pthread_mutex_t memory_debug_mutex = PTHREAD_MUTEX_INITIALIZER;

// Allocation tracking structure
typedef struct {
    void *ptr;
    size_t size;
    const char *file;
    int line;
    char tag[MAX_TAG_LENGTH];
    bool freed;
} AllocationInfo;

// Allocation tracking state
static AllocationInfo allocations[MAX_TRACKED_ALLOCATIONS];
static size_t allocation_count = 0;
static size_t total_allocated = 0;
static size_t peak_allocated = 0;
static size_t allocation_count_total = 0;

void memory_debug_configure(bool enable_tracking, bool enable_bounds_check, bool enable_leak_detection) {
    pthread_mutex_lock(&memory_debug_mutex);
    tracking_enabled = enable_tracking;
    bounds_check_enabled = enable_bounds_check;
    leak_detection_enabled = enable_leak_detection;
    pthread_mutex_unlock(&memory_debug_mutex);
}

void memory_debug_init(void) {
    pthread_mutex_lock(&memory_debug_mutex);
    if (!initialized) {
        memset(allocations, 0, sizeof(allocations));
        allocation_count = 0;
        total_allocated = 0;
        peak_allocated = 0;
        allocation_count_total = 0;
        initialized = true;
    }
    pthread_mutex_unlock(&memory_debug_mutex);
    
    log_info("Memory debugger initialized (tracking=%d, bounds_check=%d, leak_detection=%d)",
             tracking_enabled, bounds_check_enabled, leak_detection_enabled);
}

void memory_debug_cleanup(void) {
    pthread_mutex_lock(&memory_debug_mutex);
    
    if (leak_detection_enabled) {
        size_t leaks = 0;
        size_t leak_bytes = 0;
        
        for (size_t i = 0; i < allocation_count; i++) {
            if (!allocations[i].freed) {
                leaks++;
                leak_bytes += allocations[i].size;
                log_error("MEMORY LEAK: %zu bytes at %p allocated in %s:%d [%s]",
                         allocations[i].size, allocations[i].ptr,
                         allocations[i].file, allocations[i].line,
                         allocations[i].tag);
            }
        }
        
        if (leaks > 0) {
            log_error("Memory leak summary: %zu leaks, %zu bytes total", leaks, leak_bytes);
        } else {
            log_info("No memory leaks detected");
        }
    }
    
    if (tracking_enabled) {
        log_info("Memory tracking summary:");
        log_info("  Peak memory usage: %zu bytes", peak_allocated);
        log_info("  Total allocations: %zu", allocation_count_total);
    }
    
    initialized = false;
    pthread_mutex_unlock(&memory_debug_mutex);
}

void memory_debug_track(void *ptr, size_t size, const char *file, int line, const char *tag) {
    if (!tracking_enabled || !initialized || !ptr) {
        return;
    }
    
    pthread_mutex_lock(&memory_debug_mutex);
    
    // Find an empty slot or grow the array if needed
    size_t index = allocation_count;
    if (index >= MAX_TRACKED_ALLOCATIONS) {
        log_error("Maximum tracked allocations (%d) exceeded; memory tracking will be incomplete",
                 MAX_TRACKED_ALLOCATIONS);
        pthread_mutex_unlock(&memory_debug_mutex);
        return;
    }
    
    allocations[index].ptr = ptr;
    allocations[index].size = size;
    allocations[index].file = file;
    allocations[index].line = line;
    allocations[index].freed = false;
    
    if (tag) {
        strncpy(allocations[index].tag, tag, MAX_TAG_LENGTH - 1);
        allocations[index].tag[MAX_TAG_LENGTH - 1] = '\0';
    } else {
        strcpy(allocations[index].tag, "unknown");
    }
    
    allocation_count++;
    allocation_count_total++;
    total_allocated += size;
    
    if (total_allocated > peak_allocated) {
        peak_allocated = total_allocated;
    }
    
    pthread_mutex_unlock(&memory_debug_mutex);
}

void memory_debug_untrack(void *ptr, const char *file, int line) {
    if (!tracking_enabled || !initialized || !ptr) {
        return;
    }
    
    pthread_mutex_lock(&memory_debug_mutex);
    
    bool found = false;
    for (size_t i = 0; i < allocation_count; i++) {
        if (allocations[i].ptr == ptr && !allocations[i].freed) {
            allocations[i].freed = true;
            total_allocated -= allocations[i].size;
            found = true;
            break;
        }
    }
    
    if (!found) {
        log_error("Attempt to free untracked memory at %p in %s:%d",
                 ptr, file, line);
    }
    
    pthread_mutex_unlock(&memory_debug_mutex);
}

bool memory_debug_is_valid_ptr(const void *ptr) {
    if (!tracking_enabled || !initialized || !ptr) {
        // Cannot validate when tracking is disabled
        return true;
    }
    
    pthread_mutex_lock(&memory_debug_mutex);
    
    bool valid = false;
    for (size_t i = 0; i < allocation_count; i++) {
        if (allocations[i].ptr == ptr && !allocations[i].freed) {
            valid = true;
            break;
        }
        
        // Check if pointer is within a valid allocation's range
        if (bounds_check_enabled && 
            !allocations[i].freed &&
            memory_debug_ptr_in_range(ptr, allocations[i].ptr, allocations[i].size)) {
            valid = true;
            break;
        }
    }
    
    pthread_mutex_unlock(&memory_debug_mutex);
    return valid;
}

bool memory_debug_ptr_in_range(const void *ptr, const void *start, size_t size) {
    const char *p = (const char *)ptr;
    const char *s = (const char *)start;
    return (p >= s && p < (s + size));
}

void memory_debug_print_stats(void) {
    if (!tracking_enabled || !initialized) {
        log_info("Memory tracking not enabled");
        return;
    }
    
    pthread_mutex_lock(&memory_debug_mutex);
    
    size_t active_allocations = 0;
    for (size_t i = 0; i < allocation_count; i++) {
        if (!allocations[i].freed) {
            active_allocations++;
        }
    }
    
    log_info("Memory tracking statistics:");
    log_info("  Current allocations: %zu (%zu bytes)", active_allocations, total_allocated);
    log_info("  Peak memory usage: %zu bytes", peak_allocated);
    log_info("  Total allocations: %zu", allocation_count_total);
    
    pthread_mutex_unlock(&memory_debug_mutex);
}

void memory_debug_dump_allocations(void) {
    if (!tracking_enabled || !initialized) {
        log_info("Memory tracking not enabled");
        return;
    }
    
    pthread_mutex_lock(&memory_debug_mutex);
    
    log_info("Current active allocations:");
    for (size_t i = 0; i < allocation_count; i++) {
        if (!allocations[i].freed) {
            log_info("  %p: %zu bytes [%s] at %s:%d",
                    allocations[i].ptr,
                    allocations[i].size,
                    allocations[i].tag,
                    allocations[i].file,
                    allocations[i].line);
        }
    }
    
    pthread_mutex_unlock(&memory_debug_mutex);
}

void memory_debug_set_canary(void *ptr, size_t size) {
    if (!bounds_check_enabled || !initialized || !ptr) {
        return;
    }
    
    // Set canary pattern at end of allocation
    uint32_t *canary_ptr = (uint32_t *)((char *)ptr + size);
    canary_ptr[0] = CANARY_PATTERN;
    canary_ptr[1] = CANARY_PATTERN;
}

bool memory_debug_check_canary(void *ptr, size_t size) {
    if (!bounds_check_enabled || !initialized || !ptr) {
        return true;
    }
    
    // Check canary pattern at end of allocation
    uint32_t *canary_ptr = (uint32_t *)((char *)ptr + size);
    return (canary_ptr[0] == CANARY_PATTERN && canary_ptr[1] == CANARY_PATTERN);
}

bool memory_debug_check_corruption(void *ptr) {
    if (!tracking_enabled || !bounds_check_enabled || !initialized || !ptr) {
        return true;
    }
    
    pthread_mutex_lock(&memory_debug_mutex);
    
    bool valid = true;
    size_t size = 0;
    bool found = false;
    
    // Find the allocation info for this pointer
    for (size_t i = 0; i < allocation_count; i++) {
        if (allocations[i].ptr == ptr && !allocations[i].freed) {
            size = allocations[i].size;
            found = true;
            break;
        }
    }
    
    if (found) {
        // Check the canary pattern
        valid = memory_debug_check_canary(ptr, size);
        if (!valid) {
            log_error("Memory corruption detected: buffer overflow at %p", ptr);
        }
    } else {
        valid = false;
        log_error("Memory corruption check failed: %p is not a tracked allocation", ptr);
    }
    
    pthread_mutex_unlock(&memory_debug_mutex);
    return valid;
}

// Memory allocation wrappers

void *memory_debug_malloc(size_t size, const char *file, int line, const char *tag) {
    size_t alloc_size = size;
    if (bounds_check_enabled) {
        alloc_size += CANARY_SIZE;  // Add space for canary
    }
    
    void *ptr = malloc(alloc_size);
    if (!ptr) {
        log_error("malloc failed for %zu bytes at %s:%d", size, file, line);
        return NULL;
    }
    
    if (bounds_check_enabled) {
        memory_debug_set_canary(ptr, size);
    }
    
    memory_debug_track(ptr, size, file, line, tag);
    return ptr;
}

void *memory_debug_calloc(size_t nmemb, size_t size, const char *file, int line, const char *tag) {
    size_t total_size = nmemb * size;
    size_t alloc_size = total_size;
    if (bounds_check_enabled) {
        alloc_size += CANARY_SIZE;  // Add space for canary
    }
    
    void *ptr = calloc(1, alloc_size);  // Use calloc(1, size) to handle overflow checking
    if (!ptr) {
        log_error("calloc failed for %zu elements of %zu bytes at %s:%d", nmemb, size, file, line);
        return NULL;
    }
    
    if (bounds_check_enabled) {
        memory_debug_set_canary(ptr, total_size);
    }
    
    memory_debug_track(ptr, total_size, file, line, tag);
    return ptr;
}

void *memory_debug_realloc(void *ptr, size_t size, const char *file, int line, const char *tag) {
    if (!ptr) {
        // If ptr is NULL, realloc behaves like malloc
        return memory_debug_malloc(size, file, line, tag);
    }
    
    // Untrack the old allocation
    memory_debug_untrack(ptr, file, line);
    
    size_t alloc_size = size;
    if (bounds_check_enabled) {
        alloc_size += CANARY_SIZE;
    }
    
    void *new_ptr = realloc(ptr, alloc_size);
    if (!new_ptr) {
        log_error("realloc failed for %zu bytes at %s:%d", size, file, line);
        return NULL;
    }
    
    if (bounds_check_enabled) {
        memory_debug_set_canary(new_ptr, size);
    }
    
    memory_debug_track(new_ptr, size, file, line, tag);
    return new_ptr;
}

void memory_debug_free(void *ptr, const char *file, int line) {
    if (!ptr) {
        return;  // Free of NULL is a no-op
    }
    
    memory_debug_untrack(ptr, file, line);
    free(ptr);
}

char *memory_debug_strdup(const char *s, const char *file, int line, const char *tag) {
    if (!s) {
        return NULL;
    }
    
    size_t len = strlen(s) + 1;
    char *dup = memory_debug_malloc(len, file, line, tag);
    if (dup) {
        memcpy(dup, s, len);
    }
    
    return dup;
}
