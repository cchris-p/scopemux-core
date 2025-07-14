#pragma once

#include "../ast_node.h"
#include "../../../include/scopemux/language.h"
#include "../../../include/scopemux/reference_resolver.h"
#include "../../../include/scopemux/symbol_table.h"

/**
 * Shared statistics for C/C++ language resolution
 */
typedef struct {
  size_t num_total_lookups;
  size_t num_resolved;
  size_t num_header_resolved;
  size_t num_macro_resolved;
  size_t num_struct_fields_resolved;
  size_t num_class_resolved;
  size_t num_template_resolved;
  size_t num_namespace_resolved;
} ReferenceResolverCCppStats;

/**
 * Shared implementation for C/C++ language resolution.
 * Handles common functionality like struct/class resolution,
 * header inclusion, and namespace lookup.
 */
ResolutionStatus reference_resolver_c_cpp_resolve(ASTNode *node, ReferenceType ref_type,
                                                  const char *name, GlobalSymbolTable *symbol_table,
                                                  void *resolver_data, bool cpp_mode);

/**
 * Get C/C++ resolver statistics
 */
void reference_resolver_c_cpp_get_stats(size_t *total, size_t *resolved, size_t *header_resolved,
                                        size_t *macro_resolved, size_t *struct_fields_resolved,
                                        size_t *class_resolved, size_t *template_resolved,
                                        size_t *namespace_resolved);

/**
 * Reset C/C++ resolver statistics
 */
void reference_resolver_c_cpp_reset_stats(void);
