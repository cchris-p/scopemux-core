#include "../../../include/scopemux/ast.h"
#include "../../../include/scopemux/logging.h"
#include "../../../include/scopemux/parser.h"
#include "../../../include/scopemux/project_context.h"
#include "../../../include/scopemux/reference_resolver.h"
#include "../../../include/scopemux/symbol_table.h"
#include "../ast_node.h"
#include "js_ts_resolver_shared_utils.h"
#include "language_resolvers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Public API implementation
ResolutionStatus reference_resolver_javascript(ASTNode *node, ReferenceType ref_type,
                                               const char *name, GlobalSymbolTable *symbol_table,
                                               void *resolver_data) {
  return reference_resolver_js_ts_resolve(node, ref_type, name, symbol_table, resolver_data, false);
}

/**
 * Get JavaScript resolver statistics
 */
void javascript_resolver_get_stats(size_t *total, size_t *resolved, size_t *import_resolved,
                                   size_t *property_resolved, size_t *prototype_resolved) {
  reference_resolver_js_ts_get_stats(total, resolved, import_resolved, NULL, NULL, NULL, NULL,
                                     NULL);
}

/**
 * Reset JavaScript resolver statistics
 */
void javascript_resolver_reset_stats() { reference_resolver_js_ts_reset_stats(); }
