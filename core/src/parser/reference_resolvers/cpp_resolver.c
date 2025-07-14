#include "../../../include/scopemux/logging.h"
#include "../../../include/scopemux/parser.h"
#include "../../../include/scopemux/reference_resolver.h"
#include "../../../include/scopemux/symbol_table.h"
#include "../ast_node.h"
#include "c_cpp_resolver_shared_utils.h"
#include "language_resolvers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Public API implementation
ResolutionStatus reference_resolver_cpp(ASTNode *node, ReferenceType ref_type, const char *name,
                                        GlobalSymbolTable *symbol_table, void *resolver_data) {
  return reference_resolver_c_cpp_resolve(node, ref_type, name, symbol_table, resolver_data, true);
}

/**
 * Get C++ resolver statistics
 */
void cpp_resolver_get_stats(size_t *total, size_t *resolved, size_t *namespace_resolved,
                            size_t *template_resolved, size_t *method_resolved,
                            size_t *class_resolved) {
  reference_resolver_c_cpp_get_stats(total, resolved, NULL, NULL, NULL, class_resolved,
                                     template_resolved, namespace_resolved);
}

/**
 * Reset C++ resolver statistics
 */
void cpp_resolver_reset_stats() { reference_resolver_c_cpp_reset_stats(); }
