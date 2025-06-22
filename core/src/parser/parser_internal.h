/**
 * @file parser_internal.h
 * @brief Internal declarations for the parser module
 *
 * Contains shared definitions and declarations for the parser module
 * components that are not meant to be exposed publicly.
 */

#ifndef SCOPEMUX_PARSER_INTERNAL_H
#define SCOPEMUX_PARSER_INTERNAL_H

#include "../../include/scopemux/config/node_type_mapping_loader.h"
#include "../../include/scopemux/logging.h"
#include "../../include/scopemux/memory_debug.h"
#include "../../include/scopemux/parser.h"
#include "../../include/scopemux/query_manager.h"
#include "../../include/scopemux/tree_sitter_integration.h"
#include <setjmp.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Magic number for AST nodes to detect corruption/double-free
#define ASTNODE_MAGIC 0xABCD1234

// --- Memory Debugging Functions ---

/**
 * @brief Print summary of CST node allocation/free operations
 */
void print_cst_free_summary(void);

/**
 * @brief Register a CST node in the tracking registry
 *
 * @param node Node to register
 * @param type Type name for the node
 */
void register_cst_node(CSTNode *node, const char *type);

/**
 * @brief Mark a CST node as freed in the registry
 *
 * @param node Node that was freed
 */
void mark_cst_node_freed(CSTNode *node);

/**
 * @brief Register the summary function to run at program exit
 */
void register_cst_free_summary(void);

/**
 * @brief Helper function to safely free a field with memory tracking
 *
 * @param ptr_field Pointer to the field pointer to free
 * @param field_name Name of the field for logging
 * @param encountered_error Pointer to error flag to set if issues occur
 */
void safe_free_field(void **ptr_field, const char *field_name, int *encountered_error);

/**
 * @brief Signal handler for crash recovery
 *
 * @param sig Signal number
 */
void segfault_handler(int sig);

// Shared data for signal handling
extern jmp_buf parse_crash_recovery;
extern volatile int crash_occurred;

#endif /* SCOPEMUX_PARSER_INTERNAL_H */
