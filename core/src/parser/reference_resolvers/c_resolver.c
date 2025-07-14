#include "../../../include/scopemux/logging.h"
#include "../../../include/scopemux/parser.h"
#include "../../../include/scopemux/project_context.h"
#include "../../../include/scopemux/reference_resolver.h"
#include "../../../include/scopemux/symbol_table.h"
#include "../ast_node.h"
#include "c_cpp_resolver_shared_utils.h"
#include "language_resolvers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "language_resolvers.h"

// Public API implementation
ResolutionStatus reference_resolver_c(ASTNode *node, ReferenceType ref_type, const char *name,
                                      GlobalSymbolTable *symbol_table, void *resolver_data) {
  return reference_resolver_c_cpp_resolve(node, ref_type, name, symbol_table, resolver_data, false);
}

/**
 * Get C resolver statistics
 */
void reference_resolver_c_get_stats(size_t *total, size_t *resolved, size_t *macro_resolved,
                                    size_t *header_resolved, size_t *struct_fields_resolved) {
  reference_resolver_c_cpp_get_stats(total, resolved, header_resolved, macro_resolved,
                                     struct_fields_resolved, NULL, NULL, NULL);
}

/**
 * Reset C resolver statistics
 */
void reference_resolver_c_reset_stats() { reference_resolver_c_cpp_reset_stats(); }
