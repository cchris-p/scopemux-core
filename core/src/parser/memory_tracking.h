/**
 * @file memory_tracking.h
 * @brief Memory tracking and debugging utilities for parser components
 *
 * Contains functions for tracking memory allocations, detecting leaks,
 * and providing detailed diagnostics about memory usage.
 */

#ifndef SCOPEMUX_MEMORY_TRACKING_H
#define SCOPEMUX_MEMORY_TRACKING_H

#include "../../core/include/scopemux/parser.h"
#include <setjmp.h>
#include <stdbool.h>

// Registry size for tracking CST nodes
#define MAX_CST_NODES 1000

/**
 * @brief Print summary of cst_node allocations and frees at program exit
 */
void print_cst_free_summary(void);

/**
 * @brief Register the summary function to run at program exit
 */
void register_cst_free_summary(void);

/**
 * @brief Register a node in the tracking registry
 *
 * @param node Node to register
 * @param type Type of the node
 */
void register_cst_node(CSTNode *node, const char *type);

/**
 * @brief Mark a node as freed in the registry
 *
 * @param node Node to mark as freed
 */
void mark_cst_node_freed(CSTNode *node);

/**
 * @brief Helper function to safely free a memory field with tracking
 *
 * @param ptr_field Pointer to the field pointer to free
 * @param field_name Name of the field for logging
 * @param encountered_error Pointer to error flag to set if issues occur
 */
void safe_free_field(void **ptr_field, const char *field_name, int *encountered_error);

/**
 * @brief Signal handler for crash recovery during parsing
 *
 * @param sig Signal number
 */
void segfault_handler(int sig);

// Shared data for signal handling
extern jmp_buf parse_crash_recovery;
extern volatile int crash_occurred;

#endif /* SCOPEMUX_MEMORY_TRACKING_H */
