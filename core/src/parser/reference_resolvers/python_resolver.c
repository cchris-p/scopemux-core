#include "../../../include/scopemux/logging.h"
#include "../../../include/scopemux/parser.h"
#include "../../../include/scopemux/reference_resolver.h"
#include "../../../include/scopemux/symbol_table.h"
#include "../ast_node.h"
#include "language_resolvers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Python resolver stats
static struct {
  size_t total_lookups;
  size_t resolved_lookups;
  size_t import_lookups;
  size_t class_lookups;
  size_t method_lookups;
  size_t function_lookups;
  size_t variable_lookups;
  size_t decorator_lookups;
} python_resolver_stats = {0};

// Public API implementation
ResolutionStatus reference_resolver_python(ASTNode *node, ReferenceType ref_type, const char *name,
                                           GlobalSymbolTable *symbol_table, void *resolver_data) {
  (void)resolver_data;

  // Track statistics
  python_resolver_stats.total_lookups++;

  // For now, just use the generic resolver
  ResolutionStatus status = reference_resolver_generic_resolve(node, ref_type, name, symbol_table);
  if (status == RESOLUTION_SUCCESS) {
    python_resolver_stats.resolved_lookups++;
  }
  return status;
}

/**
 * Get Python resolver stats
 */
void python_resolver_get_stats(size_t *total, size_t *resolved, size_t *import_resolved,
                                size_t *class_resolved, size_t *method_resolved,
                                size_t *function_resolved, size_t *variable_resolved,
                                size_t *decorator_resolved) {
  if (total) *total = python_resolver_stats.total_lookups;
  if (resolved) *resolved = python_resolver_stats.resolved_lookups;
  if (import_resolved) *import_resolved = python_resolver_stats.import_lookups;
  if (class_resolved) *class_resolved = python_resolver_stats.class_lookups;
  if (method_resolved) *method_resolved = python_resolver_stats.method_lookups;
  if (function_resolved) *function_resolved = python_resolver_stats.function_lookups;
  if (variable_resolved) *variable_resolved = python_resolver_stats.variable_lookups;
  if (decorator_resolved) *decorator_resolved = python_resolver_stats.decorator_lookups;
}

/**
 * Reset Python resolver stats
 */
void python_resolver_reset_stats(void) {
  memset(&python_resolver_stats, 0, sizeof(python_resolver_stats));
}
