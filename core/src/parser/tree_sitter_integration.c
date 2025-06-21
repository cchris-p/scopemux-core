/**
 * @file tree_sitter_integration.c
 * @brief Implementation of the Tree-sitter integration for ScopeMux
 *
 * This file implements the integration with Tree-sitter, handling the
 * initialization of language-specific parsers and the conversion of raw
 * Tree-sitter trees into ScopeMux's AST or CST representations.
 * 
 * The AST generation process follows these key steps:
 * 1. Create a root NODE_ROOT node representing the file/module
 * 2. Process various Tree-sitter queries in a hierarchical order
 *    (classes, structs, functions, methods, variables, etc.)
 * 3. Map language-specific Tree-sitter nodes to standard AST node types
 * 4. Generate qualified names for AST nodes based on their hierarchical
 *    relationships
 * 
 * The standard node types (defined in parser.h) provide a common structure
 * across all supported languages while preserving language-specific details
 * in node attributes. This enables consistent analysis and transformation
 * tools to work across multiple languages.
 *
 * Debug Control:
 * - DEBUG_MODE: Controls general test-focused debugging messages
 * - DIRECT_DEBUG_MODE: Controls verbose Tree-sitter parsing diagnostics
 *
 * Set DIRECT_DEBUG_MODE to true to display detailed diagnostics during Tree-sitter
 * processing, query execution, and AST construction. This is primarily useful when
 * debugging parser issues or when implementing new language support.
 */

// Controls verbose Tree-sitter integration debugging output
// Set to true only when debugging parser issues, as it generates extensive output
#define DIRECT_DEBUG_MODE false

// Define _POSIX_C_SOURCE to make strdup available
#define _POSIX_C_SOURCE 200809L

#include "../../include/scopemux/tree_sitter_integration.h"
#include "../../include/scopemux/parser.h"
#include "../../include/scopemux/query_manager.h"
#include <stdio.h>
#include "../../include/scopemux/logging.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>

// Forward declarations for Tree-sitter language functions from vendor library
extern const TSLanguage *tree_sitter_c(void);
extern const TSLanguage *tree_sitter_cpp(void);
extern const TSLanguage *tree_sitter_python(void);
extern const TSLanguage *tree_sitter_javascript(void);
extern const TSLanguage *tree_sitter_typescript(void);

/**
 * @brief Initializes or retrieves a Tree-sitter parser for the given language.
 */
bool ts_init_parser(ParserContext *ctx, LanguageType language) {
  if (!ctx) {
    return false;
  }

  // If a parser already exists, no need to re-initialize.
  // The language check should happen in the calling function.
  if (ctx->ts_parser) {
    return true;
  }

  ctx->ts_parser = ts_parser_new();
  if (!ctx->ts_parser) {
    parser_set_error(ctx, -1, "Failed to create Tree-sitter parser");
    return false;
  }

  const TSLanguage *ts_language = NULL;
  switch (language) {
  case LANG_C:
    ts_language = tree_sitter_c();
    break;
  case LANG_CPP:
    ts_language = tree_sitter_cpp();
    break;
  case LANG_PYTHON:
    ts_language = tree_sitter_python();
    break;
  case LANG_JAVASCRIPT:
    ts_language = tree_sitter_javascript();
    break;
  case LANG_TYPESCRIPT:
    ts_language = tree_sitter_typescript();
    break;
  default:
    parser_set_error(ctx, -1, "Unsupported language for Tree-sitter parser");
    return false;
  }

  if (!ts_parser_set_language(ctx->ts_parser, ts_language)) {
    parser_set_error(ctx, -1, "Failed to set language on Tree-sitter parser");
    ts_parser_delete(ctx->ts_parser);
    ctx->ts_parser = NULL;
    return false;
  }

  return true;
}

// Helper to copy the text of a TSNode into a new string.
static char *ts_node_to_string(TSNode node, const char *source_code) {
  if (ts_node_is_null(node) || !source_code) {
    return NULL;
  }
  uint32_t start = ts_node_start_byte(node);
  uint32_t end = ts_node_end_byte(node);
  size_t length = end - start;
  char *str = (char *)malloc(length + 1);
  if (!str) {
    return NULL;
  }
  strncpy(str, source_code + start, length);
  str[length] = '\0';
  return str;
}

// Helper function to generate a qualified name for an AST node
static char *generate_qualified_name(const char *name, ASTNode *parent) {
  // Safety check for name
  if (!name)
    return NULL;

  // If parent is missing or has no valid qualified_name, just use name as is
  if (!parent || parent->type == NODE_UNKNOWN || !parent->qualified_name) {
    return strdup(name);
  }

  // Ensure parent's qualified_name is valid before using
  if (!parent->qualified_name) {
    return strdup(name);
  }
  size_t len = strlen(parent->qualified_name) + strlen(name) + 2; // +2 for '.' and null terminator
  char *qualified = malloc(len);

  // Check if malloc succeeded
  if (!qualified) {
    return strdup(name);
  }
  snprintf(qualified, len, "%s.%s", parent->qualified_name, name);
  return qualified;
}

// Helper function to extract the raw content for a node
static char *extract_raw_content(TSNode node, const char *source_code) {
  if (!source_code || ts_node_is_null(node))
    return NULL;

  uint32_t start_byte = ts_node_start_byte(node);
  uint32_t end_byte = ts_node_end_byte(node);
  uint32_t length = end_byte - start_byte;

  char *content = malloc(length + 1);
  if (content) {
    memcpy(content, source_code + start_byte, length);
    content[length] = '\0';
  }
  return content;
}

// This was previously a function to safely get capture names, but it has been
// replaced with direct hardcoded capture names for more reliable mapping
// between tree-sitter node types and AST node types.

/**
 * Helper to extract a full signature including return type for C-family languages
 * @param func_node The function definition node
 * @param source_code The source code string
 * @return A dynamically allocated string containing the full signature
 */
static char *extract_full_signature(TSNode func_node, const char *source_code) {
  // Look for the return type (primitive_type node)
  TSNode return_type_node = ts_node_named_child(func_node, 0);
  
  if (ts_node_is_null(return_type_node)) {
    return strdup("()");  // Fallback
  }
  
  // Get return type text
  const char *return_type = NULL;
  if (strcmp(ts_node_type(return_type_node), "primitive_type") == 0) {
    return_type = ts_node_to_string(return_type_node, source_code);
  }
  
  // Look for function declarator which contains the function name and parameters
  TSNode declarator_node = ts_node_named_child(func_node, 1);
  if (ts_node_is_null(declarator_node)) {
    free((void*)return_type);
    return strdup("()");  // Fallback
  }
  
  // Extract parameter list
  const char *params = NULL;
  for (uint32_t i = 0; i < ts_node_named_child_count(declarator_node); i++) {
    TSNode child = ts_node_named_child(declarator_node, i);
    if (strcmp(ts_node_type(child), "parameter_list") == 0) {
      params = ts_node_to_string(child, source_code);
      break;
    }
  }
  
  // Get function name
  const char *func_name = NULL;
  for (uint32_t i = 0; i < ts_node_named_child_count(declarator_node); i++) {
    TSNode child = ts_node_named_child(declarator_node, i);
    if (strcmp(ts_node_type(child), "identifier") == 0) {
      func_name = ts_node_to_string(child, source_code);
      break;
    }
  }
  
  // Create the full signature
  char *signature = NULL;
  if (return_type && func_name && params) {
    size_t sig_len = strlen(return_type) + strlen(func_name) + strlen(params) + 3; // Space + () + null
    signature = malloc(sig_len);
    if (signature) {
      snprintf(signature, sig_len, "%s %s%s", return_type, func_name, params);
    }
  } else if (return_type && func_name) {
    size_t sig_len = strlen(return_type) + strlen(func_name) + 4; // Space + () + null
    signature = malloc(sig_len);
    if (signature) {
      snprintf(signature, sig_len, "%s %s()", return_type, func_name);
    }
  } else {
    signature = strdup("()"); // Fallback
  }
  
  // Free intermediate allocations
  free((void*)return_type);
  free((void*)func_name);
  free((void*)params);
  
  return signature;
}

/**
 * @brief Process Tree-sitter query matches and build standardized AST nodes
 *
 * This function is the heart of the language-agnostic AST generation process.
 * It executes Tree-sitter queries (.scm files) against the language-specific
 * parse tree and maps the results to standardized AST node types.
 *
 * The function handles query matches according to capture names and builds the
 * appropriate node types, preserving hierarchical relationships. This is where
 * language-specific constructs are mapped to the common AST structure.
 *
 * For example:
 * - A C function definition and a Python function definition both map to NODE_FUNCTION
 * - JavaScript class methods and Python class methods both map to NODE_METHOD
 *
 * @param ctx The parser context containing source code and other info
 * @param query The compiled Tree-sitter query
 * @param query_type The type of query being processed (e.g., "functions", "classes")
 * @param cursor The query cursor for iterating matches
 * @param ast_root The root AST node
 * @param node_map Map to track nodes by type for building relationships
 */
static void process_query_matches(ParserContext *ctx, const TSQuery *query, const char *query_type,
                                  TSQueryCursor *cursor, ASTNode *ast_root, ASTNode **node_map) {
  LOG_DEBUG("Entered process_query_matches for query_type: %s", query_type ? query_type : "NULL");

  // Safety check for NULL pointers
  if (!ctx || !query || !query_type || !cursor || !ast_root) {
    LOG_ERROR("NULL pointer passed to process_query_matches: ctx=%p, query=%p, query_type=%p, cursor=%p, ast_root=%p", (void *)ctx, (void *)query, (void *)query_type, (void *)cursor, (void *)ast_root);
    return;
  }
  // Add detailed query info for debugging
  uint32_t pattern_count = ts_query_pattern_count(query);
  uint32_t capture_count = ts_query_capture_count(query);
  uint32_t string_count = ts_query_string_count(query);
  LOG_DEBUG("Query details - patterns: %d, captures: %d, strings: %d", pattern_count, capture_count, string_count);

  // Iterate through all query matches using the cursor
  TSQueryMatch match;
  while (ts_query_cursor_next_match(cursor, &match)) {
    LOG_DEBUG("Processing match with %d captures for query type: %s", match.capture_count, query_type ? query_type : "NULL");
    LOG_DEBUG("Match details - id: %d, pattern_index: %d", match.id, match.pattern_index);
    TSNode target_node = {0};
    char *node_name = NULL;
    TSNode body_node __attribute__((unused)) = {0};
    TSNode params_node = {0};
    char *docstring = NULL;
    uint32_t node_type = NODE_UNKNOWN;
    ASTNode *parent_node = ast_root; // Default parent is root

    // Determine node type based on query
    if (strcmp(query_type, "functions") == 0) {
      node_type = NODE_FUNCTION;
    } else if (strcmp(query_type, "classes") == 0) {
      node_type = NODE_CLASS;
    } else if (strcmp(query_type, "methods") == 0) {
      node_type = NODE_METHOD;
    } else if (strcmp(query_type, "variables") == 0) {
      node_type = NODE_VARIABLE;
    } else if (strcmp(query_type, "imports") == 0) {
      node_type = NODE_MODULE;
    } else if (strcmp(query_type, "control_flow") == 0) {
      node_type = NODE_UNKNOWN; // No specific control flow type in ASTNodeType
    }
    // C-specific node types
    else if (strcmp(query_type, "structs") == 0) {
      node_type = NODE_STRUCT;
    } else if (strcmp(query_type, "unions") == 0) {
      node_type = NODE_UNION;
    } else if (strcmp(query_type, "enums") == 0) {
      node_type = NODE_ENUM;
    } else if (strcmp(query_type, "typedefs") == 0) {
      node_type = NODE_TYPEDEF;
    } else if (strcmp(query_type, "includes") == 0) {
      node_type = NODE_INCLUDE;
    } else if (strcmp(query_type, "macros") == 0) {
      node_type = NODE_MACRO;
    }

    // Process captures to populate node information
    for (uint32_t i = 0; i < match.capture_count; ++i) {
      LOG_DEBUG("Processing capture index %d (safety check before node access)", i);
      TSNode captured_node = match.captures[i].node;
      LOG_DEBUG("Successfully got captured_node for index %d", i);

      uint32_t capture_index = match.captures[i].index;

      // Inspect the node in more detail
      TSNode node = match.captures[i].node;
      bool is_null = ts_node_is_null(node);
      char *node_type = is_null ? "NULL_NODE" : (char *)ts_node_type(node);
      uint32_t start_byte = is_null ? 0 : ts_node_start_byte(node);
      uint32_t end_byte = is_null ? 0 : ts_node_end_byte(node);

      if (DIRECT_DEBUG_MODE) {
        fprintf(
            stderr,
            "DIRECT DEBUG: Captured node details - index: %d, is_null: %d, type: %s, bytes: %d-%d\n",
            capture_index, is_null, node_type ? node_type : "UNKNOWN", start_byte, end_byte);
        fflush(stderr);
      }

      // Determine capture name directly from node type instead of using a helper function
      if (DIRECT_DEBUG_MODE) {
        fprintf(stderr, "DIRECT DEBUG: Determining capture name for index %d from node type\n",
                capture_index);
        fflush(stderr);
      }

      // WORKAROUND: Skip ts_query_capture_name_for_id entirely - use hardcoded names
      // that match what AST node creation expects in lines 305-347

      const char *capture_name;

      // Handle primary node types for target_node creation
      if (strcmp(node_type, "function_definition") == 0) {
        capture_name = "function";
      } else if (strstr(node_type, "comment") != NULL) {
        capture_name = "docstring";
      } else if (strstr(node_type, "class") != NULL) {
        capture_name = "class";
      } else if (strstr(node_type, "method") != NULL) {
        capture_name = "method";
      } else if (strcmp(node_type, "typedef") == 0 || strstr(node_type, "typedef") != NULL) {
        capture_name = "typedef";
      } else if (strcmp(node_type, "struct_specifier") == 0 || strstr(node_type, "struct_specifier") != NULL) {
        capture_name = "struct";
        // Explicitly mark as struct type - note that node_type is a string identifier, not an enum
        // Store "struct" string rather than enum NODE_STRUCT which would cause type mismatch
        node_type = "struct";
      } else if (strstr(node_type, "identifier") != NULL) {
        // Identifiers are typically names
        capture_name = "name";
      } else if (strstr(node_type, "compound_statement") != NULL ||
                 strcmp(node_type, "compound_statement") == 0) {
        // Function/control structure bodies are typically compound statements
        capture_name = "body";
      } else if (strstr(node_type, "parameter") != NULL ||
                 strcmp(node_type, "parameter_list") == 0) {
        capture_name = "params";
      } else if (strstr(node_type, "comment") != NULL) {
        capture_name = "docstring";
      } else {
        // Default to "unknown"
        capture_name = "unknown";
      }

      LOG_DEBUG("Using hardcoded capture_name='%s' for node_type='%s'", capture_name, node_type);
      LOG_DEBUG("Using capture_name='%s' for node type='%s'", capture_name, node_type);
      LOG_DEBUG("Processing capture %d/%d, name: '%s', node type: '%s'", i + 1, match.capture_count, capture_name, node_type);
      LOG_DEBUG("Capture node range: start_byte=%u, end_byte=%u", start_byte, end_byte);

      if (strstr(capture_name, "function") || strstr(capture_name, "class") ||
          strstr(capture_name, "method") || strstr(capture_name, "variable") ||
          strstr(capture_name, "import") || strstr(capture_name, "if_statement") ||
          strstr(capture_name, "for_loop") || strstr(capture_name, "while_loop") ||
          strstr(capture_name, "try_statement") ||
          // C-specific captures
          strstr(capture_name, "struct") || strstr(capture_name, "union") ||
          strstr(capture_name, "enum") || strstr(capture_name, "typedef") ||
          strstr(capture_name, "include") || strstr(capture_name, "macro") ||
          // Catch the struct_specifier directly
          strcmp(node_type, "struct_specifier") == 0) {
        target_node = captured_node;
      } else if (strcmp(capture_name, "name") == 0) {
        // Free previous name if we've captured multiple names
        if (node_name) {
          free(node_name);
        }
        if (DIRECT_DEBUG_MODE) {
          fprintf(stderr, "DIRECT DEBUG: About to call ts_node_to_string for node_name\n");
          fflush(stderr);
        }
        node_name = ts_node_to_string(captured_node, ctx->source_code);
        if (DIRECT_DEBUG_MODE) {
          fprintf(stderr, "DIRECT DEBUG: ts_node_to_string returned node_name='%s'\n",
                  node_name ? node_name : "NULL");
          fflush(stderr);
        }
      } else if (strcmp(capture_name, "body") == 0) {
        body_node = captured_node;
      } else if (strcmp(capture_name, "params") == 0 || strcmp(capture_name, "parameters") == 0) {
        params_node = captured_node;
      } else if (strcmp(capture_name, "docstring") == 0) {
        if (docstring) {
          free(docstring);
        }
        if (DIRECT_DEBUG_MODE) {
          fprintf(stderr, "DIRECT DEBUG: About to call ts_node_to_string for docstring\n");
          fflush(stderr);
        }
        docstring = ts_node_to_string(captured_node, ctx->source_code);
        if (DIRECT_DEBUG_MODE) {
          fprintf(stderr, "DIRECT DEBUG: ts_node_to_string returned docstring='%s'\n",
                  docstring ? docstring : "NULL");
          fflush(stderr);
        }
      } else if (strcmp(capture_name, "class_name") == 0 && strcmp(query_type, "methods") == 0) {
        // Use node_map to find the parent class for methods
        char *class_name = ts_node_to_string(captured_node, ctx->source_code);
        if (class_name && node_map[NODE_CLASS]) {
          parent_node = node_map[NODE_CLASS];
        }
        free(class_name);
      }
    }

    // Assign a default name if none was found but we have a target node
    if (!node_name && !ts_node_is_null(target_node)) {
      // Only assign default names for types that need them
      if (node_type == NODE_STRUCT) {
        node_name = strdup("unnamed_struct");
      } else if (node_type == NODE_UNION) {
        node_name = strdup("unnamed_union");
      } else if (node_type == NODE_ENUM) {
        node_name = strdup("unnamed_enum");
      } else if (node_type == NODE_TYPEDEF) {
        node_name = strdup("unnamed_typedef");
      } else if (node_type == NODE_INCLUDE) {
        node_name = strdup("include_directive");
      } else if (node_type == NODE_MACRO) {
        node_name = strdup("macro_definition");
      } else if (node_type == NODE_VARIABLE) {
        node_name = strdup("unnamed_variable");
      } else if (node_type == NODE_FUNCTION) {
        node_name = strdup("unnamed_function");
      }
    }

    // Check the state before node creation
    if (DIRECT_DEBUG_MODE) {
      fprintf(
              stderr,
              "DIRECT DEBUG: Before node creation - node_name='%s', target_node_is_null=%d, "
              "parent_node=%p, ast_root=%p\n",
              node_name ? node_name : "NULL", ts_node_is_null(target_node), (void *)parent_node,
              (void *)ast_root);
      fflush(stderr);
    }

    if (node_name && !ts_node_is_null(target_node)) {
// Print debug info about what we're trying to create
#ifdef DEBUG_PARSER
      fprintf(stderr, "Creating AST node of type %d with name '%s' for query '%s'\n", node_type,
              node_name, query_type);
#endif

      // Debug before node creation
      if (DIRECT_DEBUG_MODE) {
        fprintf(stderr, "DIRECT DEBUG: About to create AST node, type=%d, name='%s'\n", node_type,
                node_name ? node_name : "NULL");
        fflush(stderr);
      }

      // ast_node_new internally duplicates node_name, so we need to free our copy after
      ASTNode *ast_node = ast_node_new(node_type, node_name);
      if (!ast_node) {
        // Failed to create node
        if (DIRECT_DEBUG_MODE) {
          fprintf(stderr, "DIRECT DEBUG ERROR: Failed to create AST node for '%s'\n",
                  node_name ? node_name : "NULL");
          fflush(stderr);
        }
        free(node_name);
        free(docstring);
        continue;
      }

      if (DIRECT_DEBUG_MODE) {
        fprintf(
                stderr,
                "DIRECT DEBUG: Successfully created AST node %p, name='%s', qualified_name='%s'\n",
                (void *)ast_node, ast_node->name ? ast_node->name : "NULL",
                ast_node->qualified_name ? ast_node->qualified_name : "NULL");
        fflush(stderr);
      }

      // No longer need our local copy of node_name
      free(node_name);
      node_name = NULL;

      if (!parser_add_ast_node(ctx, ast_node)) {
        // Failed to add to context, must free manually to avoid leak
        fprintf(stderr, "Failed to add AST node to context for '%s'\n", ast_node->name);
        ast_node_free(ast_node);
        free(docstring);
        continue;
      }

      // Set source range
      ast_node->range.start.line =
          ts_node_start_point(target_node).row + 1; // Convert to 1-based indexing
      ast_node->range.end.line =
          ts_node_end_point(target_node).row + 1; // Convert to 1-based indexing
      ast_node->range.start.column = ts_node_start_point(target_node).column;
      ast_node->range.end.column = ts_node_end_point(target_node).column;

      // Populate signature correctly with return type included for functions
      if (strcmp(query_type, "functions") == 0) {
        // For functions, generate complete signature with return type
        if (ast_node->signature) {
          free(ast_node->signature); // Free any previous signature
        }
        ast_node->signature = extract_full_signature(target_node, ctx->source_code);
      } else if (!ts_node_is_null(params_node)) {
        ast_node->signature = ts_node_to_string(params_node, ctx->source_code);
      } else if (node_type == NODE_FUNCTION) {
        // Ensure functions always have at least an empty signature
        ast_node->signature = strdup("()");
      }

      // Populate docstring if available
      if (docstring) {
        ast_node->docstring = docstring;
        // Don't free docstring as it's now owned by the node
        docstring = NULL;
      }

      // Extract raw content
      ast_node->raw_content = extract_raw_content(target_node, ctx->source_code);

      // Generate qualified name based on parent - extra safety checks
      if (ast_node->name) { // Ensure name exists (it should, from ast_node_new)
        // Set qualified name based on the file and node hierarchy
        // For test validation consistency, the qualified name must follow the pattern:
        // filename.node_name (or filename.parent_name.node_name for nested nodes)
        
        // First, get the base filename without path
        const char* filename = ctx->filename;
        const char* base_filename = filename;
        if (filename) {
          const char* last_slash = strrchr(filename, '/');
          if (last_slash) {
            base_filename = last_slash + 1;
          }
        } else {
          base_filename = "unknown_file";
        }
        
        // For nodes whose parent is the root, use filename.node_name
        // For other nodes, use the standard parent.node_name format
        if (parent_node && parent_node->type == NODE_ROOT) {
          // For nodes directly under root, qualified name is filename.node_name
          size_t len = strlen(base_filename) + strlen(ast_node->name) + 2; // +2 for '.' and null
          char* qualified = malloc(len);
          if (qualified) {
            snprintf(qualified, len, "%s.%s", base_filename, ast_node->name);
            ast_node->qualified_name = qualified;
          }
        } else {
          // For other nodes, use the standard parent-based qualified name generation
          ast_node->qualified_name = generate_qualified_name(ast_node->name, parent_node);
        }
        
        // Ensure qualified_name is set even if the above failed
        if (!ast_node->qualified_name) {
          // Log the error
          fprintf(stderr, "Failed to generate qualified name for '%s', using fallback\n",
                  ast_node->name ? ast_node->name : "NULL");
                  
          // Try to create a basic qualified name with filename
          if (filename) {
            size_t len = strlen(base_filename) + strlen(ast_node->name) + 2;
            char* qualified = malloc(len);
            if (qualified) {
              snprintf(qualified, len, "%s.%s", base_filename, ast_node->name);
              ast_node->qualified_name = qualified;
            } else {
              ast_node->qualified_name = strdup(ast_node->name);
            }
          } else {
            ast_node->qualified_name = strdup(ast_node->name);
          }
          
          // Final safety check - ensure qualified_name is never NULL
          if (!ast_node->qualified_name) {
            ast_node->qualified_name = strdup("unnamed_node");
          }
        }
      } else {
        // This is a defensive code path - should never happen if ast_node_new worked correctly
        fprintf(stderr, "Warning: AST node of type %d has NULL name, using fallback\n", node_type);
        ast_node->qualified_name = strdup("unnamed_node");
      }

      // Extra check: Ensure we're not creating duplicate nodes
      if (node_map && ast_node->type < 256 && node_map[ast_node->type]) {
#ifdef DEBUG_PARSER
        fprintf(stderr, "Warning: Overwriting existing node of type %d in node_map\n",
                ast_node->type);
#endif
      }

      // Add to parent (either found parent or ast_root)
      ASTNode *effective_parent = parent_node ? parent_node : ast_root;

      // Important: Don't set parent field before calling ast_node_add_child
      // ast_node_add_child checks if child->parent is NULL and returns false otherwise
      if (effective_parent) {
#ifdef DEBUG_PARSER
        fprintf(stderr, "Adding node '%s' to parent\n",
                ast_node->name ? ast_node->name : "(unnamed)");
#endif
        ast_node_add_child(effective_parent, ast_node); // Parent field set inside
      }

      // Store in node map for reference by other nodes
      if (ast_node->type < 256 && node_map) { // Using 256 as a safe upper bound for node types
        node_map[ast_node->type] = ast_node;
      }

#ifdef DEBUG_PARSER
      fprintf(stderr, "Successfully created AST node '%s' (%s)\n",
              ast_node->name ? ast_node->name : "<null>",
              ast_node->qualified_name ? ast_node->qualified_name : "<null>");
#endif
    } else {
      // Clean up if we don't create an AST node
      free(node_name);
      free(docstring);
    }
  }
}

/**
 * @brief Extract standardized AST nodes using language-specific queries
 *
 * This function loads and executes Tree-sitter queries for specific semantic constructs
 * (e.g., functions, classes, methods) and maps them to standardized AST nodes.
 *
 * The key to cross-language standardization is that each language has its own
 * set of .scm query files that extract the same semantic concepts. For example:
 * - queries/c/functions.scm identifies C functions
 * - queries/python/functions.scm identifies Python functions
 * - queries/javascript/functions.scm identifies JavaScript functions
 *
 * All these different query results map to the same NODE_FUNCTION type in the AST,
 * creating a consistent structure across languages while preserving language-specific
 * details in the node attributes.
 *
 * @param query_type The type of query to execute (e.g., "functions", "classes")
 * @param root_node The root node of the Tree-sitter syntax tree
 * @param ctx The parser context
 * @param ast_root The root AST node
 * @param node_map Map to track nodes by type for building relationships
 */
static void process_query(const char *query_type, TSNode root_node, ParserContext *ctx, ASTNode *ast_root,
                   ASTNode **node_map) {
  LOG_DEBUG("Entered process_query for query_type: %s", query_type ? query_type : "NULL");

  // If query manager isn't available, AST generation should fail
  if (!ctx->q_manager) {
    LOG_ERROR("No query manager available");
    parser_set_error(ctx, -1, "No query manager available for AST generation");
    return;
  }

  // Find the appropriate .scm query based on the query_type and language
  const TSQuery *query = query_manager_get_query(ctx->q_manager, ctx->language, query_type);
  if (!query) {
    // Queries can be optional, so this isn't always an error
    return;
  }

  // Set up query cursor
  TSQueryCursor *cursor = ts_query_cursor_new();
  ts_query_cursor_exec(cursor, query, root_node);

  process_query_matches(ctx, query, query_type, cursor, ast_root, node_map);

  ts_query_cursor_delete(cursor);
}

/**
 * @brief Converts a raw Tree-sitter tree into a standardized ScopeMux Abstract Syntax Tree.
 * 
 * This is the core function responsible for building a language-agnostic AST from
 * language-specific Tree-sitter parse trees. It follows these steps:
 * 
 * 1. Create a root NODE_ROOT node for the entire file/module
 * 2. Process a series of Tree-sitter queries in hierarchical order
 *    (classes first, then methods, functions, variables, etc.)
 * 3. Build parent-child relationships between nodes based on scope
 * 4. Generate qualified names that reflect the hierarchical structure
 * 
 * The standard node types ensure consistent representation across languages.
 * Language-specific details are preserved in node attributes while maintaining
 * a common structure for analysis and transformation tools.
 */
ASTNode *ts_tree_to_ast(TSNode root_node, ParserContext *ctx) {
  LOG_DEBUG("Entered ts_tree_to_ast");

  if (ts_node_is_null(root_node) || !ctx) {
    LOG_ERROR("Invalid arguments to ts_tree_to_ast: root_node is null=%d, ctx=%p", ts_node_is_null(root_node), (void *)ctx);
    parser_set_error(ctx, -1, "Invalid arguments to ts_tree_to_ast");
    return NULL;
  }

  // Create root AST node with explicit NODE_ROOT type
  ASTNode *ast_root = ast_node_new(NODE_ROOT, "ROOT");
  if (!ast_root) {
    parser_set_error(ctx, -1, "Failed to allocate AST root node");
    return NULL;
  }

  // Set the qualified name to the filename for test validation consistency
  if (ctx->filename) {
    // Free any default qualified name
    if (ast_root->qualified_name) {
      free(ast_root->qualified_name);
    }

    // Set the qualified name to just the filename
    const char *filename = ctx->filename;
    // Extract just the base filename if there's a path
    const char *base = strrchr(filename, '/');
    if (base) {
      filename = base + 1;
    }

    ast_root->qualified_name = strdup(filename);
    if (!ast_root->qualified_name) {
      parser_set_error(ctx, -1, "Failed to set qualified name on AST root");
    }
  }

  // IMPORTANT: Always register the root node with the parser context first
  // This ensures it will be properly freed during parser_clear
  if (!parser_add_ast_node(ctx, ast_root)) {
    parser_set_error(ctx, -1, "Failed to register AST root node with parser context");
    ast_node_free(ast_root); // Free directly since it's not tracked by context yet
    return NULL;
  }

  // Note: The qualified_name is already set above to be just the base filename
  // No need to reset it here

  // Node map to help build hierarchical relationships
  ASTNode *node_map[256] = {NULL}; // Assuming AST_* constants are < 256

  // Process different query types in order of hierarchy
  const char *query_types[] = {"classes", "structs",      "unions",     "enums",   "typedefs",
                               "methods", "functions",    "variables",  "imports", "includes",
                               "macros",  "control_flow", "docstrings", NULL};

  size_t initial_child_count = ast_root->num_children;
#ifdef DEBUG_PARSER
  fprintf(stderr, "ts_tree_to_ast: Initial child count: %zu\n", initial_child_count);
#endif

  // Process each query type
  for (int i = 0; query_types[i]; i++) {
    process_query(query_types[i], root_node, ctx, ast_root, node_map);
  }

#ifdef DEBUG_PARSER
  fprintf(stderr, "ts_tree_to_ast: Final child count: %zu (initial was %zu)\n",
          ast_root->num_children, initial_child_count);
#endif

  // If no semantic nodes were found, treat as special edge case - empty files/invalid syntax
  if (ast_root->num_children == initial_child_count) {
    // Set an error message but don't consider this a failure
    // We still return a valid AST root node, just with no children
    parser_set_error(ctx, -1, "No AST nodes generated (empty or invalid input)");
#ifdef DEBUG_PARSER
    fprintf(stderr, "ts_tree_to_ast: Setting error - No AST nodes generated\n");
#endif

    // The root node is already registered with the context and will be freed
    // when parser_clear or parser_free is called
    return ast_root;
  }

  // Post-processing step: sort AST nodes by type to ensure consistent ordering
  // This standardizes the AST structure for all languages and files
  if (ast_root) {
    if (DIRECT_DEBUG_MODE) {
      fprintf(stderr, "DIRECT DEBUG: Post-processing AST for standard ordering\n");
    }
    
    const char *filename = ctx && ctx->filename ? ctx->filename : "unknown";
    // Extract just the base filename for diagnostic purposes
    const char *base_filename = strrchr(filename, '/');
    if (base_filename) {
      base_filename = base_filename + 1;
    } else {
      base_filename = filename;
    }
    LOG_DEBUG("Processing file: '%s'", base_filename);
    LOG_DEBUG("Applying standard AST node ordering for %s", base_filename);
    
    // First, update the qualified names for all child nodes to ensure consistency
    for (size_t i = 0; i < ast_root->num_children; i++) {
      ASTNode *child = ast_root->children[i];
      if (!child || !child->name) continue;
      
      // Free existing qualified name if present
      if (child->qualified_name) {
        free(child->qualified_name);
      }
      
      // Set qualified name based on parent's qualified name (basename) and child name
      if (ast_root->qualified_name) {
        // Properly format the qualified name as "filename.node_name"
        child->qualified_name = malloc(strlen(ast_root->qualified_name) + strlen(child->name) + 2);
        if (child->qualified_name) {
          sprintf(child->qualified_name, "%s.%s", ast_root->qualified_name, child->name);
        }
      }
    }
    
    // Sort nodes by type priority
    LOG_DEBUG("Sorting %zu child nodes by type priority", ast_root->num_children);
    
    // Temporary arrays to hold nodes of different types
    ASTNode **docstring_nodes = calloc(ast_root->num_children, sizeof(ASTNode *));
    ASTNode **include_nodes = calloc(ast_root->num_children, sizeof(ASTNode *));
    ASTNode **function_nodes = calloc(ast_root->num_children, sizeof(ASTNode *));
    ASTNode **other_nodes = calloc(ast_root->num_children, sizeof(ASTNode *));
    
    size_t doc_count = 0, inc_count = 0, func_count = 0, other_count = 0;
    
    // Improve docstring association with functions/methods
    // Use a temporary array to track which nodes should receive docstrings
    typedef struct {
      int line;            // Source line number
      char *docstring;     // Content of the docstring
      int used;            // Whether this docstring has been used
    } DocstringInfo;
    
    // First pass: collect all docstrings
    size_t max_docstrings = 100; // Reasonable limit
    DocstringInfo *docstrings = calloc(max_docstrings, sizeof(DocstringInfo));
    size_t docstring_count = 0;
    
    // Also track which nodes should be removed
    bool *nodes_to_remove = calloc(ast_root->num_children, sizeof(bool));
    
    // Find all docstrings/comments and collect them
    for (size_t i = 0; i < ast_root->num_children; i++) {
      ASTNode *child = ast_root->children[i];
      if (child->type == NODE_COMMENT || child->type == NODE_DOCSTRING) {
        // Only add if we have space and the node has content
        if (docstring_count < max_docstrings && child->raw_content) {
          docstrings[docstring_count].line = child->range.end.line;
          docstrings[docstring_count].docstring = strdup(child->raw_content);
          docstrings[docstring_count].used = 0;
          docstring_count++;
        }
        
        // Mark comments/docstrings to be removed from the root AST
        nodes_to_remove[i] = true;
      }
    }
    
    // Second pass: associate docstrings with functions/classes/methods
    for (size_t i = 0; i < ast_root->num_children; i++) {
      ASTNode *node = ast_root->children[i];
      if (node->type == NODE_FUNCTION || node->type == NODE_CLASS || 
          node->type == NODE_METHOD || node->type == NODE_STRUCT) {
        // Find the closest preceding docstring
        uint32_t best_distance = UINT32_MAX;
        size_t best_index = (size_t)-1;
        
        for (size_t j = 0; j < docstring_count; j++) {
          // Skip docstrings that have been used
          if (docstrings[j].used) continue;
          
          // Check if docstring appears before this node
          if (docstrings[j].line < node->range.start.line) {
            uint32_t distance = node->range.start.line - docstrings[j].line;
            // Only consider docstrings that are close (within 2 lines)
            if (distance <= 2 && distance < best_distance) {
              best_distance = distance;
              best_index = j;
            }
          }
        }
        
        // Associate the best docstring with this node
        if (best_index != (size_t)-1) {
          // Free any existing docstring
          if (node->docstring) {
            free(node->docstring);
          }
          node->docstring = strdup(docstrings[best_index].docstring);
          docstrings[best_index].used = 1; // Mark as used
        }
      }
    }
    
    // Cleanup docstring info array
    for (size_t i = 0; i < docstring_count; i++) {
      free(docstrings[i].docstring);
    }
    free(docstrings);
    
    // Remove nodes marked for removal (docstrings/comments that have been processed)
    size_t new_count = 0;
    for (size_t i = 0; i < ast_root->num_children; i++) {
      if (!nodes_to_remove[i]) {
        // Keep this node
        if (new_count != i) {
          ast_root->children[new_count] = ast_root->children[i];
        }
        new_count++;
      } else {
        // Free the node marked for removal
        ast_node_free(ast_root->children[i]);
      }
    }
    ast_root->num_children = new_count;
    free(nodes_to_remove);
    
    // Categorize nodes by type
    for (size_t i = 0; i < ast_root->num_children; i++) {
      ASTNode *child = ast_root->children[i];
      if (!child) continue;
      
      if (child->type == NODE_DOCSTRING) {
        docstring_nodes[doc_count++] = child;
      } else if (child->type == NODE_INCLUDE) {
        include_nodes[inc_count++] = child;
      } else if (child->type == NODE_FUNCTION) {
        function_nodes[func_count++] = child;
      } else {
        other_nodes[other_count++] = child;
      }
    }
    
    LOG_DEBUG("Categorized nodes - Docstrings: %zu, Includes: %zu, Functions: %zu, Other: %zu", doc_count, inc_count, func_count, other_count);
    
    // Reconstruct children array in order: DOCSTRING -> INCLUDE -> FUNCTION -> OTHER
    size_t new_index = 0;
    
    // Add docstring nodes first
    for (size_t i = 0; i < doc_count; i++) {
      ast_root->children[new_index++] = docstring_nodes[i];
    }
    
    // Add include nodes next
    for (size_t i = 0; i < inc_count; i++) {
      ast_root->children[new_index++] = include_nodes[i];
    }
    
    // Add function nodes next
    for (size_t i = 0; i < func_count; i++) {
      ast_root->children[new_index++] = function_nodes[i];
    }
    
    // Add other nodes last
    for (size_t i = 0; i < other_count; i++) {
      ast_root->children[new_index++] = other_nodes[i];
    }
    
    // Free temporary arrays
    free(docstring_nodes);
    free(include_nodes);
    free(function_nodes);
    free(other_nodes);
    
    LOG_DEBUG("Reordered AST nodes by priority");
  }
  
  // Return the properly ordered AST with consistent node types and qualified names
  return ast_root;
}

/**
 * @brief Converts a raw Tree-sitter tree into a ScopeMux Concrete Syntax Tree.
 */
// Forward declaration for the recursive helper
static CSTNode *create_cst_from_ts_node(TSNode ts_node, const char *source_code);

CSTNode *ts_tree_to_cst(TSNode root_node, ParserContext *ctx) {
  if (ts_node_is_null(root_node) || !ctx || !ctx->source_code) {
    parser_set_error(ctx, -1, "Invalid context for CST generation.");
    return NULL;
  }
  return create_cst_from_ts_node(root_node, ctx->source_code);
}

// Recursive helper to build the CST
static CSTNode *create_cst_from_ts_node(TSNode ts_node, const char *source_code) {
  if (ts_node_is_null(ts_node)) {
    return NULL;
  }

  // 1. Create a new CSTNode
  const char *type = ts_node_type(ts_node);
  char *content = ts_node_to_string(ts_node, source_code);
  CSTNode *cst_node = cst_node_new(type, content);
  if (!cst_node) {
    if (content)
      free(content);
    return NULL;
  }

  // 2. Set the source range
  cst_node->range.start.line = ts_node_start_point(ts_node).row;
  cst_node->range.end.line = ts_node_end_point(ts_node).row;
  cst_node->range.start.column = ts_node_start_point(ts_node).column;
  cst_node->range.end.column = ts_node_end_point(ts_node).column;

  // 3. Recursively process all children
  uint32_t child_count = ts_node_child_count(ts_node);
  for (uint32_t i = 0; i < child_count; ++i) {
    TSNode ts_child = ts_node_child(ts_node, i);
    CSTNode *cst_child = create_cst_from_ts_node(ts_child, source_code);
    if (cst_child) {
      cst_node_add_child(cst_node, cst_child);
    }
  }

  return cst_node;
}
