/**
 * @file symbol_management.c
 * @brief Symbol registration and reference resolution functionality for ProjectContext
 *
 * Handles the registration of symbols from parsed files into the global symbol table,
 * and the resolution of references between symbols across different files.
 */

#include "scopemux/symbol_management.h"
#include "scopemux/ast.h"
#include "scopemux/logging.h"
#include "scopemux/project_context.h"
// #include "scopemux/re"
#include "scopemux/reference_resolver.h"
#include "scopemux/symbol_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations
SymbolEntry *symbol_entry_create(const char *qualified_name, ASTNode *node, const char *file_path,
                                 SymbolScope scope, Language language);
bool reference_resolver_init_builtin(ReferenceResolver *resolver);
void reference_resolver_free(ReferenceResolver *resolver);
ResolutionStatus reference_resolver_resolve_node(ReferenceResolver *resolver, ASTNode *node,
                                                 ReferenceType type, const char *name,
                                                 Language language);
void reference_resolver_get_stats(const ReferenceResolver *resolver, size_t *out_total_references,
                                  size_t *out_resolved_references);

/**
 * @brief Helper function to recursively register symbols from an AST node
 *
 * @param symbol_table The global symbol table
 * @param node The ASTNode to register
 * @param filepath The file path (used for symbol qualification)
 */
static void register_node_symbols(GlobalSymbolTable *symbol_table, ASTNode *node,
                                  const char *filepath) {
  if (!symbol_table || !node) {
    return;
  }

  // Register this node if it has a name and is a significant symbol type
  if (node->name && node->qualified_name) {
    switch (node->type) {
    // Register all significant, top-level symbol types
    case NODE_FUNCTION:
    case NODE_CLASS:
    case NODE_STRUCT:
    case NODE_ENUM:
    case NODE_VARIABLE:
    case NODE_TYPEDEF:
    case NODE_NAMESPACE:
    case NODE_INTERFACE:
    case NODE_MODULE:
    case NODE_MACRO:
    case NODE_USING:
    case NODE_FRIEND:
    case NODE_OPERATOR:
      // Add to symbol table with filepath for context
      SymbolEntry *entry =
          symbol_entry_create(node->qualified_name, node, filepath, SCOPE_FILE, node->lang);
      if (entry) {
        symbol_table_add(symbol_table, entry);
      }
      break;

    default:
      // Other node types might not need to be in the global symbol table
      break;
    }
  }

  // Recursively process children
  for (size_t i = 0; i < node->num_children; i++) {
    register_node_symbols(symbol_table, node->children[i], filepath);
  }
}

/**
 * @brief Extract symbols from a parser context and add them to the global symbol table
 *
 * This function extracts symbols from a parser context and adds them to the
 * global symbol table for cross-file reference resolution.
 *
 * @param project The ProjectContext
 * @param parser The ParserContext to extract symbols from
 * @param symbol_table The global symbol table to add symbols to
 * @return true if successful, false otherwise
 */
bool project_extract_symbols_impl(ProjectContext *project, ParserContext *parser,
                                  GlobalSymbolTable *symbol_table) {
  if (!project || !parser || !symbol_table) {
    log_error("Invalid parameters for project_extract_symbols_impl");
    return false;
  }

  log_debug("Extracting symbols from parser context: %s", SAFE_STR(parser->filename));

  // Process each AST node in the file
  for (size_t i = 0; i < parser->num_ast_nodes; i++) {
    ASTNode *root = parser->all_ast_nodes[i];
    if (!root) {
      continue;
    }

    // Register symbols from this AST
    register_node_symbols(symbol_table, root, parser->filename);
  }

  return true;
}

/**
 * @brief Get a symbol by its qualified name from anywhere in the project (Implementation)
 *
 * @param project The ProjectContext
 * @param qualified_name The fully qualified name of the symbol
 * @return The ASTNode for the symbol, or NULL if not found
 */
const ASTNode *project_get_symbol_impl(const ProjectContext *project, const char *qualified_name) {
  if (!project || !qualified_name || !project->symbol_table) {
    return NULL;
  }

  SymbolEntry *entry = symbol_table_lookup(project->symbol_table, qualified_name);
  return entry ? entry->node : NULL;
}

/**
 * @brief Helper function to recursively collect nodes of a specific type
 *
 * @param node The ASTNode to check
 * @param type The type to match
 * @param out_nodes Array to store the found symbols
 * @param max_nodes Maximum number of nodes to store
 * @param count Pointer to the current count of found nodes
 */
static void collect_nodes_by_type(const ASTNode *node, ASTNodeType type, const ASTNode **out_nodes,
                                  size_t max_nodes, size_t *count) {
  if (!node || !out_nodes || !count) {
    return;
  }

  // Check if this node matches the requested type
  if (node->type == type) {
    if (*count < max_nodes) {
      out_nodes[*count] = node;
    }
    (*count)++;
  }

  // Recursively check children
  for (size_t i = 0; i < node->num_children; i++) {
    collect_nodes_by_type(node->children[i], type, out_nodes, max_nodes, count);
  }
}

/**
 * @brief Get all symbols of a specific type across the entire project (Implementation)
 *
 * @param project The ProjectContext
 * @param type The type of symbols to find
 * @param out_nodes Array to store the found symbols
 * @param max_nodes Maximum number of nodes to store
 * @return The number of symbols found (may be greater than max_nodes if buffer is too small)
 */
size_t project_get_symbols_by_type_impl(const ProjectContext *project, ASTNodeType type,
                                        const ASTNode **out_nodes, size_t max_nodes) {
  if (!project || !out_nodes || max_nodes == 0) {
    return 0;
  }

  size_t count = 0;

  // Iterate over all files in the project
  for (size_t i = 0; i < project->num_files; i++) {
    ParserContext *ctx = project->file_contexts[i];
    if (!ctx)
      continue;

    // Iterate over all nodes in the file
    for (size_t j = 0; j < ctx->num_ast_nodes; j++) {
      collect_nodes_by_type(ctx->all_ast_nodes[j], type, out_nodes, max_nodes, &count);
    }
  }

  return count;
}

/**
 * @brief Helper function to recursively resolve references in an AST node
 *
 * @param project The ProjectContext
 * @param node The ASTNode to resolve references for
 */
static void resolve_node_references(ProjectContext *project, ASTNode *node,
                                    ReferenceResolver *resolver) {
  if (!project || !node) {
    return;
  }

  // Resolve references for this node based on its type
  switch (node->type) {
  case NODE_FUNCTION:
    if (node->name) {
      reference_resolver_resolve_node(resolver, node, REF_CALL, node->name,
                                      LANG_UNKNOWN); // TODO: Thread language from context
    }
    break;
  case NODE_VARIABLE:
    if (node->name) {
      reference_resolver_resolve_node(resolver, node, REF_USE, node->name,
                                      LANG_UNKNOWN); // TODO: Thread language from context
    }
    break;
  case NODE_CLASS:
  case NODE_STRUCT:
  case NODE_ENUM:
  case NODE_INTERFACE:
    if (node->name) {
      reference_resolver_resolve_node(resolver, node, REF_TYPE, node->name,
                                      LANG_UNKNOWN); // TODO: Thread language from context
    }
    break;
  case NODE_IMPORT:
    if (node->name) {
      reference_resolver_resolve_node(resolver, node, REF_IMPORT, node->name,
                                      LANG_UNKNOWN); // TODO: Thread language from context
    }
    break;
  case NODE_INCLUDE:
    if (node->raw_content) {
      char *include_path = NULL;
      char *start = strchr(node->raw_content, '"');
      if (start) {
        start++;
        char *end = strchr(start, '"');
        if (end) {
          size_t len = end - start;
          include_path = malloc(len + 1);
          if (include_path) {
            strncpy(include_path, start, len);
            include_path[len] = '\0';
          }
        }
      }
      if (include_path) {
        reference_resolver_resolve_node(resolver, node, REF_INCLUDE, include_path,
                                        LANG_UNKNOWN); // TODO: Thread language from context
        free(include_path);
      }
    }
    break;
  default:
    // Other node types might not need reference resolution at this level
    break;
  }

  // Recursively resolve references for children
  for (size_t i = 0; i < node->num_children; i++) {
    resolve_node_references(project, node->children[i], resolver);
  }
}

/**
 * @brief Resolve references across all files in the project (Implementation)
 *
 * @param project The ProjectContext
 * @return true if successful, false otherwise
 */
bool project_resolve_references_impl(ProjectContext *project) {
  if (!project) {
    return false;
  }

  log_info("Resolving references across %zu files", project->num_files);

  // Create a reference resolver
  ReferenceResolver *resolver = reference_resolver_create(project->symbol_table);
  if (!resolver) {
    log_error("Failed to create reference resolver");
    return false;
  }

  // Initialize the resolver with built-in resolvers
  if (!reference_resolver_init_builtin(resolver)) {
    log_error("Failed to initialize reference resolver with built-in resolvers");
    reference_resolver_free(resolver);
    return false;
  }

  // Iterate through all files and resolve references
  for (size_t i = 0; i < project->num_files; i++) {
    ParserContext *ctx = project->file_contexts[i];
    if (!ctx)
      continue;

    log_debug("Resolving references in file: %s", SAFE_STR(ctx->filename));

    // Start with the root nodes
    for (size_t j = 0; j < ctx->num_ast_nodes; j++) {
      resolve_node_references(project, ctx->all_ast_nodes[j], resolver);
    }
  }

  // Log resolution statistics
  size_t total_lookups = 0, resolved_count = 0;
  reference_resolver_get_stats(resolver, &total_lookups, &resolved_count);

  log_info("Reference resolution complete: %zu lookups, %zu resolved", total_lookups,
           resolved_count);

  return true;
}

/**
 * @brief Helper function to recursively find references to a symbol in an AST node
 *
 * @param current The current node to check
 * @param target The target node to find references to
 * @param out_references Array to store the found references
 * @param max_references Maximum number of references to store
 * @param count Pointer to the current count of found references
 */
static void find_references_in_node(const ASTNode *current, const ASTNode *target,
                                    const ASTNode **out_references, size_t max_references,
                                    size_t *count) {
  if (!current || !target || !out_references || !count) {
    return;
  }

  // Check if this node references the target
  for (size_t i = 0; i < current->num_references; i++) {
    if (current->references[i] == target) {
      if (*count < max_references) {
        out_references[*count] = current;
      }
      (*count)++;
      break; // Found a reference, no need to check other references of this node
    }
  }

  // Recursively check children
  for (size_t i = 0; i < current->num_children; i++) {
    find_references_in_node(current->children[i], target, out_references, max_references, count);
  }
}

/**
 * @brief Find all references to a symbol across the project (Implementation)
 *
 * @param project The ProjectContext
 * @param node The symbol ASTNode to find references for
 * @param out_references Array to store the found references
 * @param max_references Maximum number of references to store
 * @return The number of references found
 */
size_t project_find_references_impl(const ProjectContext *project, const ASTNode *node,
                                    const ASTNode **out_references, size_t max_references) {
  if (!project || !node || !out_references || max_references == 0) {
    return 0;
  }

  size_t count = 0;

  // Need the qualified name to search for references
  if (!node->qualified_name) {
    return 0;
  }

  // Iterate over all files in the project
  for (size_t i = 0; i < project->num_files; i++) {
    ParserContext *ctx = project->file_contexts[i];
    if (!ctx)
      continue;

    // Iterate over all nodes in the file
    for (size_t j = 0; j < ctx->num_ast_nodes; j++) {
      find_references_in_node(ctx->all_ast_nodes[j], node, out_references, max_references, &count);
    }
  }

  return count;
}
