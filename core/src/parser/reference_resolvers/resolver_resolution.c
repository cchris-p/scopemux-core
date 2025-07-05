/**
 * @file resolver_resolution.c
 * @brief Implementation of resolution operations
 *
 * This file handles:
 * - Generic resolution strategies
 * - Node-specific resolution
 * - File-level resolution
 * - Project-level resolution
 */

#include "scopemux/logging.h"
#include "scopemux/reference_resolver.h"
#include "scopemux/reference_resolver_internal.h"
#include "scopemux/symbol_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Find the appropriate resolver for a language
 */
extern LanguageResolver *find_language_resolver_impl(ReferenceResolver *resolver,
                                                     Language language);

/**
 * Resolve a reference in a specific node
 * @note Implemented in resolver_implementation.c
 */
extern ResolutionStatus reference_resolver_resolve_node_impl(ReferenceResolver *resolver,
                                                             ASTNode *node, ReferenceType ref_type,
                                                             const char *qualified_name,
                                                             Language language);

/**
 * Resolve all references in a file
 * @note Implemented in resolver_implementation.c
 */
extern ResolutionStatus reference_resolver_resolve_file_impl(ReferenceResolver *resolver,
                                                             ParserContext *file_context);

/**
 * Resolve all references in a project
 * @note Implemented in resolver_implementation.c
 */
extern ResolutionStatus reference_resolver_resolve_all_impl(ReferenceResolver *resolver,
                                                            ProjectContext *project_context);

/**
 * Generic reference resolution algorithm
 * @note Implemented in resolver_implementation.c
 */
extern ResolutionStatus reference_resolver_generic_resolve_impl(ASTNode *node,
                                                                ReferenceType ref_type,
                                                                const char *name,
                                                                GlobalSymbolTable *symbol_table);
