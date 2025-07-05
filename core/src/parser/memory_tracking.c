/**
 * @file memory_tracking.c
 * @brief Implementation of memory tracking and debugging utilities
 *
 * This file provides implementation for memory tracking functions,
 * including CST node registry and safe memory deallocation.
 */

#include "memory_tracking.h"
#include "parser_internal.h"

// Global counter for tracking cst_node_free calls
static size_t cst_node_free_count = 0;
// Global counter for tracking cst_node_new calls
static size_t cst_node_new_count = 0;

// Registry to track all allocated CST nodes (up to 1000)
static struct {
  CSTNode *node;
  const char *type;
  int freed;
} cst_registry[MAX_CST_NODES];
static size_t cst_registry_count = 0;

// Signal handling for crash protection - explicitly export these symbols
#define SCOPEMUX_API __attribute__((visibility("default")))

// These globals are referenced from other modules
SCOPEMUX_API jmp_buf parse_crash_recovery;
SCOPEMUX_API volatile int crash_occurred = 0;

// Function to print summary of cst_node calls at program exit
void print_cst_free_summary(void) {
  printf("[CST SUMMARY] Created: %zu, Freed: %zu\n", cst_node_new_count, cst_node_free_count);

  // Print info about nodes that were created but not freed
  printf("[CST REGISTRY] Total tracked: %zu\n", cst_registry_count);
  size_t not_freed = 0;
  for (size_t i = 0; i < cst_registry_count && i < MAX_CST_NODES; i++) {
    if (!cst_registry[i].freed) {
      printf("[CST LEAK] Node at %p (type=%s) was never freed\n", (void *)cst_registry[i].node,
             cst_registry[i].type);
      not_freed++;
    }
  }
  printf("[CST LEAK SUMMARY] Nodes not freed: %zu\n", not_freed);
  fflush(stdout);
  fflush(stderr);
}

// Register the summary function to run at program exit
void register_cst_free_summary(void) { atexit(print_cst_free_summary); }

// Register a node in the tracking registry
void register_cst_node(CSTNode *node, const char *type) {
  cst_node_new_count++;
  if (cst_registry_count < MAX_CST_NODES) {
    cst_registry[cst_registry_count].node = node;
    cst_registry[cst_registry_count].type = type;
    cst_registry[cst_registry_count].freed = 0;
    cst_registry_count++;
  }
}

// Mark a node as freed in the registry
void mark_cst_node_freed(CSTNode *node) {
  cst_node_free_count++;
  for (size_t i = 0; i < cst_registry_count && i < MAX_CST_NODES; i++) {
    if (cst_registry[i].node == node) {
      cst_registry[i].freed = 1;
      break;
    }
  }
}

// Signal handler for segmentation faults during parsing
SCOPEMUX_API void segfault_handler(int sig) {
  fprintf(stderr, "\n*** SEGMENTATION FAULT DETECTED (signal %d) ***\n", sig);
  fprintf(stderr, "Dumping memory allocation information for diagnostics...\n");

  // Dump memory allocation information
  memory_debug_print_stats();
  memory_debug_dump_allocations();

  // Print stack trace if possible
  fprintf(stderr, "\nAttempting to recover from crash...\n");

  // Set the crash flag and jump back to recovery point
  crash_occurred = 1;
  longjmp(parse_crash_recovery, 1);
}

// Helper function to safely free a node field with memory tracking
void safe_free_field(void **ptr_field, const char *field_name, int *encountered_error) {
  if (!ptr_field || !*ptr_field) {
    return;
  }

  if (!memory_debug_is_valid_ptr(*ptr_field)) {
    log_error("Invalid pointer detected for field %s: %p", field_name, *ptr_field);
    if (encountered_error)
      *encountered_error = 1;
    *ptr_field = NULL;
    return;
  }

  memory_debug_free(*ptr_field, __FILE__, __LINE__);
  *ptr_field = NULL;
}
