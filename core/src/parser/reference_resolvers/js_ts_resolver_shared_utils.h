#pragma once

#include "../ast_node.h"
#include "../../../include/scopemux/language.h"
#include "../../../include/scopemux/reference_resolver.h"
#include "../../../include/scopemux/symbol_table.h"

/**
 * Shared statistics for JavaScript/TypeScript language resolution
 */
typedef struct {
  size_t num_total_lookups;
  size_t num_resolved;
  size_t num_import_resolved;
  size_t num_module_resolved;
  size_t num_class_resolved;
  size_t num_type_resolved;      // TypeScript only
  size_t num_interface_resolved; // TypeScript only
  size_t num_generic_resolved;   // TypeScript only
} ReferenceResolverJsTsStats;

/**
 * Shared implementation for JavaScript/TypeScript language resolution.
 * Handles common functionality like module imports, class resolution,
 * and prototype chain lookups.
 */
ResolutionStatus reference_resolver_js_ts_resolve(ASTNode *node, ReferenceType ref_type,
                                                  const char *name, GlobalSymbolTable *symbol_table,
                                                  void *resolver_data, bool typescript_mode);

/**
 * Get JavaScript/TypeScript resolver statistics
 */
void reference_resolver_js_ts_get_stats(size_t *total, size_t *resolved, size_t *import_resolved,
                                        size_t *module_resolved, size_t *class_resolved,
                                        size_t *type_resolved, size_t *interface_resolved,
                                        size_t *generic_resolved);

/**
 * Reset JavaScript/TypeScript resolver statistics
 */
void reference_resolver_js_ts_reset_stats(void);
