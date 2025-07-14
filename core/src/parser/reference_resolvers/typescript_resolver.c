#include "../../../include/scopemux/logging.h"
#include "../../../include/scopemux/parser.h"
#include "../../../include/scopemux/reference_resolver.h"
#include "../../../include/scopemux/symbol_table.h"
#include "../ast_node.h"
#include "js_ts_resolver_shared_utils.h"
#include "language_resolvers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Public API implementation
ResolutionStatus reference_resolver_typescript(ASTNode *node, ReferenceType ref_type,
                                               const char *name, GlobalSymbolTable *symbol_table,
                                               void *resolver_data) {
  return reference_resolver_js_ts_resolve(node, ref_type, name, symbol_table, resolver_data, true);
}

/**
 * Get TypeScript resolver statistics
 */
void typescript_resolver_get_stats(size_t *total, size_t *resolved, size_t *import_resolved,
                                   size_t *type_resolved, size_t *interface_resolved,
                                   size_t *property_resolved, size_t *generic_resolved) {
  reference_resolver_js_ts_get_stats(total, resolved, import_resolved, type_resolved,
                                     interface_resolved, property_resolved, generic_resolved, NULL);
}

/**
 * Reset TypeScript resolver statistics
 */
void typescript_resolver_reset_stats() { reference_resolver_js_ts_reset_stats(); }
