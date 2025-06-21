/**
 * @file parser.c
 * @brief Implementation of the parser module for ScopeMux
 *
 * This file implements the main parser functionality, responsible for
 * parsing source code and managing the resulting AST and CST nodes.
 */

// Define _POSIX_C_SOURCE to make strdup available
#define _POSIX_C_SOURCE 200809L

#include "../../include/scopemux/parser.h"
#include "../../include/scopemux/query_manager.h"
#include "../../include/scopemux/tree_sitter_integration.h" // For ts_parser_delete
#include "../../include/scopemux/config/node_type_mapping_loader.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Forward Declarations for internal functions ---
static void ast_node_free_internal(ASTNode *node);
// Note: cst_node_free_internal was removed as it's not needed

// --- AST Node Management ---

/**
 * Recursively free an AST node and its resources, but not its children
 * This is different from ast_node_free which also frees children
 */
static void ast_node_free_internal(ASTNode *node) __attribute__((unused));
static void ast_node_free_internal(ASTNode *node) {
#ifdef DEBUG_PARSER
  fprintf(stderr, "ast_node_free_internal: node=%p\n", (void*)node);
#endif
  if (!node)
    return;
#ifdef DEBUG_PARSER
  if (node->magic != ASTNODE_MAGIC) {
    fprintf(stderr, "ERROR: ast_node_free_internal: node=%p has invalid or corrupted magic!\n", (void*)node);
    abort();
  }
#endif
  if (!node)
    return;

  // Free all resources owned by this node
  free(node->name);
  free(node->qualified_name);
  free(node->signature);
  free(node->docstring);
  free(node->raw_content);
  free(node->children);
  free(node->references);
  free(node);
}

/**
 * Add an AST node to the parser context's tracking array
 * This ensures all allocated nodes are properly tracked and can be freed later
 */
bool parser_add_ast_node(ParserContext *ctx, ASTNode *node) {
#ifdef DEBUG_PARSER
  fprintf(stderr, "parser_add_ast_node: ctx=%p, node=%p, num_ast_nodes=%zu, capacity=%zu\n", (void*)ctx, (void*)node, ctx ? ctx->num_ast_nodes : 0, ctx ? ctx->ast_nodes_capacity : 0);
#endif
  if (!ctx || !node) {
    return false;
  }

  // Defensive check: prevent adding the same node pointer more than once.
  // If a node is added multiple times, it will be freed multiple times, causing double free or memory corruption.
  for (size_t i = 0; i < ctx->num_ast_nodes; ++i) {
    if (ctx->all_ast_nodes[i] == node) {
      // Optionally log this event for debugging purposes.
      // log_debug("ASTNode pointer %p already registered, skipping duplicate add", (void*)node);
      return true;
    }
  }

  // Check if we need to grow the array
  if (ctx->num_ast_nodes >= ctx->ast_nodes_capacity) {
#ifdef DEBUG_PARSER
    fprintf(stderr, "parser_add_ast_node: realloc all_ast_nodes from %zu to %zu\n", ctx->ast_nodes_capacity, ctx->ast_nodes_capacity == 0 ? 16 : ctx->ast_nodes_capacity * 2);
#endif
    size_t new_capacity = ctx->ast_nodes_capacity == 0 ? 16 : ctx->ast_nodes_capacity * 2;
    ASTNode **new_nodes = realloc(ctx->all_ast_nodes, new_capacity * sizeof(ASTNode *));
    if (!new_nodes) {
      return false;
    }
    ctx->all_ast_nodes = new_nodes;
    ctx->ast_nodes_capacity = new_capacity;
#ifdef DEBUG_PARSER
    fprintf(stderr, "parser_add_ast_node: realloc success, new array=%p, new capacity=%zu\n", (void*)ctx->all_ast_nodes, ctx->ast_nodes_capacity);
#endif
  }

  // Add the node to the tracking array
  ctx->all_ast_nodes[ctx->num_ast_nodes++] = node;
#ifdef DEBUG_PARSER
  fprintf(stderr, "parser_add_ast_node: added node=%p, num_ast_nodes now=%zu\n", (void*)node, ctx->num_ast_nodes);
  assert(ctx->num_ast_nodes <= ctx->ast_nodes_capacity);
#endif
  return true;
}


ASTNode *ast_node_new(ASTNodeType type, const char *name) {
#define ASTNODE_MAGIC 0xA57A57AA

  ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
  if (!node)
    return NULL;

  node->type = type;
  node->magic = ASTNODE_MAGIC;
  if (name) {
    node->name = strdup(name);
    if (!node->name) {
      free(node);
      return NULL;
    }
  }
  return node;
}

void ast_node_free(ASTNode *node) {
#ifdef DEBUG_PARSER
  fprintf(stderr, "ast_node_free: node=%p\n", (void*)node);
#endif
  if (!node)
    return;
#ifdef DEBUG_PARSER
  if (node->magic != ASTNODE_MAGIC) {
    fprintf(stderr, "ERROR: ast_node_free: node=%p has invalid or corrupted magic!\n", (void*)node);
    abort();
  }
#endif
  if (!node)
    return;

  // In our design, most AST nodes are part of the `all_ast_nodes` flat array
  // in the context and are freed in `parser_clear`. However, this function can be
  // used to free detached nodes that aren't tracked by a ParserContext.

  // Free all resources owned by this node
#ifdef DEBUG_PARSER
  fprintf(stderr, "ast_node_free: node=%p, name=%s, qualified_name=%s, signature=%p, docstring=%p, raw_content=%p, num_children=%zu\n", (void*)node, node->name ? node->name : "(null)", node->qualified_name ? node->qualified_name : "(null)", (void*)node->signature, (void*)node->docstring, (void*)node->raw_content, node->num_children);
#endif
  free(node->name);
  free(node->qualified_name);
  if (node->signature) {
#ifdef DEBUG_PARSER
    fprintf(stderr, "Freeing signature: node=%p, ptr=%p, first 16 bytes='%.*s'\n", (void*)node, (void*)node->signature, 16, node->signature);
#endif
    free(node->signature);
  }
  if (node->docstring) {
#ifdef DEBUG_PARSER
    fprintf(stderr, "Freeing docstring: node=%p, ptr=%p, first 16 bytes='%.*s'\n", (void*)node, (void*)node->docstring, 16, node->docstring);
#endif
    free(node->docstring);
  }
  if (node->raw_content) {
#ifdef DEBUG_PARSER
    fprintf(stderr, "Freeing raw_content: node=%p, ptr=%p, first 16 bytes='%.*s'\n", (void*)node, (void*)node->raw_content, 16, node->raw_content);
#endif
    free(node->raw_content);
  }

  // Free all children recursively
  for (size_t i = 0; i < node->num_children; i++) {
#ifdef DEBUG_PARSER
    fprintf(stderr, "ast_node_free: node=%p, freeing child[%zu]=%p\n", (void*)node, i, (void*)node->children[i]);
#endif
    if (node->children[i]) {
      ast_node_free(node->children[i]);
    }
  }

  // Free array pointers
  free(node->children);
  free(node->references);

#ifdef DEBUG_PARSER
  node->magic = 0; // Clear magic to catch use-after-free
  fprintf(stderr, "ast_node_free: node=%p freed (magic cleared)\n", (void*)node);
#endif
  // Free the node itself
  free(node);
}

// --- CST Node Management ---

CSTNode *cst_node_new(const char *type, char *content) {
  CSTNode *node = (CSTNode *)calloc(1, sizeof(CSTNode));
  if (!node)
    return NULL;

  node->type = type;       // This is a pointer to a static string from Tree-sitter, no copy needed.
  node->content = content; // Ownership is transferred to the node.
  return node;
}

void cst_node_free(CSTNode *node) {
  if (!node)
    return;

  for (unsigned int i = 0; i < node->children_count; i++) {
    cst_node_free(node->children[i]);
  }

  if (node->children) {
    free(node->children);
  }
  if (node->content) {
    free(node->content);
  }
  free(node);
}

// --- Parser Lifecycle ---

ParserContext *parser_init(void) {
  ParserContext *ctx = (ParserContext *)calloc(1, sizeof(ParserContext));
  if (!ctx)
    return NULL;

  // Use absolute path to the queries directory in the project root
  // This ensures queries can be found regardless of the current working directory
  ctx->q_manager = query_manager_init("/home/matrillo/apps/scopemux/queries");
  if (!ctx->q_manager) {
    free(ctx);
    return NULL;
  }

  // Load config-driven node type mapping
  load_node_type_mapping("/home/matrillo/apps/scopemux/core/config/node_type_mapping.json");

  ctx->mode = PARSE_AST; // Default mode
  return ctx;
}

void parser_free(ParserContext *ctx) {
  if (!ctx)
    return;
  parser_clear(ctx);

  // Free config-driven node type mapping
  free_node_type_mapping();

  if (ctx->ts_parser) {
    ts_parser_delete(ctx->ts_parser);
  }
  if (ctx->q_manager) {
    query_manager_free(ctx->q_manager);
  }
  free(ctx);
}

void parser_clear(ParserContext *ctx) {
#ifdef DEBUG_PARSER
  fprintf(stderr, "Entered parser_clear: ctx=%p\n", (void*)ctx);
#endif
  if (!ctx)
    return;

#ifdef DEBUG_PARSER
  fprintf(stderr, "parser_clear: ctx=%p, source_code=%p, source_code_length=%zu\n", (void*)ctx, (void*)ctx->source_code, ctx ? ctx->source_code_length : 0);
  if (ctx && ctx->source_code) {
    fprintf(stderr, "parser_clear: source_code first 16 bytes: '%.*s'\n", 16, ctx->source_code);
  }
#endif

#ifdef DEBUG_PARSER
  if (ctx->filename) {
    fprintf(stderr, "Freeing ctx->filename: ptr=%p, first 16 bytes='%.*s'\n", (void*)ctx->filename, 16, ctx->filename);
  }
#endif
  free(ctx->filename);
  ctx->filename = NULL;

#ifdef DEBUG_PARSER
  if (ctx->source_code) {
    fprintf(stderr, "Freeing ctx->source_code: ptr=%p, first 16 bytes='%.*s'\n", (void*)ctx->source_code, 16, ctx->source_code);
  }
#endif
  free(ctx->source_code);
  ctx->source_code = NULL;

  free(ctx->last_error);
  ctx->last_error = NULL;

  if (ctx->ast_root) {
    // The root itself is just one of the nodes in all_ast_nodes,
    // so we don't need to free it separately.
    ctx->ast_root = NULL;
  }

  if (ctx->all_ast_nodes) {
    for (size_t i = 0; i < ctx->num_ast_nodes; i++) {
      ASTNode *node = ctx->all_ast_nodes[i];
      if (node) {
        free(node->name);
        free(node->qualified_name);
        if (node->signature) {
#ifdef DEBUG_PARSER
          fprintf(stderr, "Freeing signature: node=%p, ptr=%p, first 16 bytes='%.*s'\n", (void*)node, (void*)node->signature, 16, node->signature);
          if (ctx && ctx->source_code && node->signature >= ctx->source_code && node->signature < ctx->source_code + ctx->source_code_length) {
            fprintf(stderr, "WARNING: signature field points inside source_code buffer!\n");
          }
#endif
          free(node->signature);
        }
        if (node->docstring) {
#ifdef DEBUG_PARSER
          fprintf(stderr, "Freeing docstring: node=%p, ptr=%p, first 16 bytes='%.*s'\n", (void*)node, (void*)node->docstring, 16, node->docstring);
          if (ctx && ctx->source_code && node->docstring >= ctx->source_code && node->docstring < ctx->source_code + ctx->source_code_length) {
            fprintf(stderr, "WARNING: docstring field points inside source_code buffer!\n");
          }
#endif
          free(node->docstring);
        }
        if (node->raw_content) {
#ifdef DEBUG_PARSER
          fprintf(stderr, "Freeing raw_content: node=%p, ptr=%p, first 16 bytes='%.*s'\n", (void*)node, (void*)node->raw_content, 16, node->raw_content);
          if (ctx && ctx->source_code && node->raw_content >= ctx->source_code && node->raw_content < ctx->source_code + ctx->source_code_length) {
            fprintf(stderr, "WARNING: raw_content field points inside source_code buffer!\n");
          }
#endif
          free(node->raw_content);
        }
        free(node->children);
        free(node->references);
        free(node);
      }
    }
    free(ctx->all_ast_nodes);
    ctx->all_ast_nodes = NULL;
  }

  if (ctx->cst_root) {
    cst_node_free(ctx->cst_root);
    ctx->cst_root = NULL;
  }

  ctx->source_code_length = 0;
  ctx->language = LANG_UNKNOWN;
  ctx->num_ast_nodes = 0;
  ctx->ast_nodes_capacity = 0;
  ctx->error_code = 0;
}

// --- Parser Configuration and Execution ---

void parser_set_mode(ParserContext *ctx, ParseMode mode) {
  if (ctx) {
    ctx->mode = mode;
  }
}

void parser_set_error(ParserContext *ctx, int code, const char *message) {
  if (!ctx)
    return;

  free(ctx->last_error);
  ctx->last_error = strdup(message);
  ctx->error_code = code;
}

/**
 * @brief Appends a child node to a parent's list of children.
 *
 * @param parent The parent CSTNode.
 * @param child The child CSTNode to add.
 * @return True on success, false on failure (e.g., memory allocation error).
 */
bool cst_node_add_child(CSTNode *parent, CSTNode *child) {
  if (!parent || !child) {
    return false;
  }

  CSTNode **new_children =
      (CSTNode **)realloc(parent->children, (parent->children_count + 1) * sizeof(CSTNode *));
  if (!new_children) {
    return false;
  }

  parent->children = new_children;
  parent->children[parent->children_count] = child;
  parent->children_count++;

  return true;
}

/* Language detection */
LanguageType parser_detect_language(const char *filename, const char *content,
                                    size_t content_length) {
  if (!filename) {
    return LANG_UNKNOWN;
  }

  // Get file extension
  const char *ext = strrchr(filename, '.');
  if (!ext) {
    return LANG_UNKNOWN;
  }

  // Convert to lowercase for case-insensitive comparison
  char ext_lower[10] = {0};
  size_t ext_len = strlen(ext);
  if (ext_len > 9) {
    ext_len = 9; // Prevent buffer overflow
  }

  for (size_t i = 0; i < ext_len; i++) {
    ext_lower[i] = (ext[i] >= 'A' && ext[i] <= 'Z') ? ext[i] + 32 : ext[i];
  }

  // Check file extensions
  if (strcmp(ext_lower, ".c") == 0 || strcmp(ext_lower, ".h") == 0) {
    return LANG_C;
  } else if (strcmp(ext_lower, ".cpp") == 0 || strcmp(ext_lower, ".cc") == 0 ||
             strcmp(ext_lower, ".cxx") == 0 || strcmp(ext_lower, ".hpp") == 0 ||
             strcmp(ext_lower, ".hxx") == 0) {
    return LANG_CPP;
  } else if (strcmp(ext_lower, ".py") == 0) {
    return LANG_PYTHON;
  }

  // If extension doesn't match, try to detect from content
  if (content && content_length > 0) {
    // Simple heuristics for language detection based on content
    // Check for Python shebang
    if (content_length > 2 && content[0] == '#' && content[1] == '!') {
      const char *python_str = strstr(content, "python");
      if (python_str && (python_str - content) < 100) { // Check in first 100 chars
        return LANG_PYTHON;
      }
    }

    // More sophisticated content-based detection could be added here
  }

  return LANG_UNKNOWN;
}

/* File parsing */
bool parser_parse_file(ParserContext *ctx, const char *filename, LanguageType language) {
  if (!ctx || !filename) {
    return false;
  }

  // Open the file
  FILE *file = fopen(filename, "rb");
  if (!file) {
    if (ctx->last_error) {
      free(ctx->last_error);
    }
    ctx->last_error = strdup("Failed to open file");
    return false;
  }

  // Get file size
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  if (file_size <= 0) {
    fclose(file);
    if (ctx->last_error) {
      free(ctx->last_error);
    }
    ctx->last_error = strdup("Empty or invalid file");
    return false;
  }

  // Allocate memory for file content
  char *content = (char *)malloc(file_size + 1);
  if (!content) {
    fclose(file);
    if (ctx->last_error) {
      free(ctx->last_error);
    }
    ctx->last_error = strdup("Memory allocation failed for file content");
    return false;
  }

  // Read file content
  size_t bytes_read = fread(content, 1, file_size, file);
  fclose(file);

  if (bytes_read != (size_t)file_size) {
    free(content);
    if (ctx->last_error) {
      free(ctx->last_error);
    }
    ctx->last_error = strdup("Failed to read entire file");
    return false;
  }

  // Null-terminate the content
  content[file_size] = '\0';

  // Parse the file content
  bool result = parser_parse_string(ctx, content, file_size, filename, language);

  // Free the temporary content buffer
  free(content);

  return result;
}

/* String parsing */
bool parser_parse_string(ParserContext *ctx, const char *const content, size_t content_length,
                         const char *filename, LanguageType language) {
  if (!ctx || !content || content_length == 0) {
    parser_set_error(ctx, -1, "Invalid arguments to parser_parse_string");
    return false;
  }

  // Clear any previous parse results before starting a new run
  parser_clear(ctx);

  // Store source code and metadata in the context
  ctx->source_code = strdup(content);
  if (!ctx->source_code) {
    parser_set_error(ctx, -1, "Failed to allocate memory for source code");
    return false;
  }
  ctx->source_code_length = content_length;

  if (filename) {
    ctx->filename = strdup(filename);
    if (!ctx->filename) {
      parser_set_error(ctx, -1, "Failed to allocate memory for filename");
      return false; // source_code will be freed by parser_clear/free
    }
  }
  ctx->language = language;

  // Initialize the Tree-sitter parser for the given language if not already done
  if (!ts_init_parser(ctx, language)) {
    // ts_init_parser is responsible for setting a detailed error message
    return false;
  }

  // Parse the source code to get a raw Tree-sitter tree
  TSTree *tree = ts_parser_parse_string(ctx->ts_parser, NULL, content, content_length);
  if (!tree) {
    parser_set_error(ctx, -1, "Tree-sitter failed to parse the source code");
    return false;
  }

  // Get the root node of the syntax tree
  TSNode root_node = ts_tree_root_node(tree);
  if (ts_node_is_null(root_node)) {
    parser_set_error(ctx, -1, "Tree-sitter returned a null root node");
    ts_tree_delete(tree);
    return false;
  }

  bool success = false;
  // Based on the selected mode, generate either an AST or a CST
  if (ctx->mode == PARSE_AST) {
    ctx->ast_root = ts_tree_to_ast(root_node, ctx);
    if (ctx->ast_root) {
      success = true;
    }
    // ts_tree_to_ast will set the error on failure
  } else if (ctx->mode == PARSE_CST) {
    ctx->cst_root = ts_tree_to_cst(root_node, ctx);
    if (ctx->cst_root) {
      success = true;
    }
    // ts_tree_to_cst will set the error on failure
  } else {
    parser_set_error(ctx, -1, "Unknown or unsupported parse mode selected");
  }

  // Clean up the raw Tree-sitter tree, as it's no longer needed
  ts_tree_delete(tree);

  return success;
}

/* Error handling */
const char *parser_get_last_error(const ParserContext *ctx) {
  if (!ctx) {
    return "Invalid parser context";
  }

  // Return NULL when there's no error, not a string "No error"
  return ctx->last_error;
}

/* Node retrieval by qualified name */
const ASTNode *parser_get_ast_node(const ParserContext *ctx, const char *qualified_name) {
  if (!ctx || !qualified_name) {
    return NULL;
  }

  // Linear search through all nodes
  for (size_t i = 0; i < ctx->num_ast_nodes; i++) {
    if (ctx->all_ast_nodes[i] &&
        strcmp(ctx->all_ast_nodes[i]->qualified_name, qualified_name) == 0) {
      return ctx->all_ast_nodes[i];
    }
  }

  return NULL; // Not found
}

/* Node retrieval by type */
size_t parser_get_ast_nodes_by_type(const ParserContext *ctx, ASTNodeType type,
                                    const ASTNode **out_nodes, size_t max_nodes) {
  if (!ctx || !out_nodes || max_nodes == 0) {
    return 0;
  }

  size_t count = 0;
  for (size_t i = 0; i < ctx->num_ast_nodes; i++) {
    if (ctx->all_ast_nodes[i] && ctx->all_ast_nodes[i]->type == type) {
      if (count < max_nodes) {
        out_nodes[count] = ctx->all_ast_nodes[i];
      }
      count++;
    }
  }

  return count;
}

/* AST node creation */
ASTNode *ast_node_create(ASTNodeType type, const char *name, const char *qualified_name,
                         SourceRange range) {
  if (!name || !qualified_name) {
    return NULL;
  }

  ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
  if (!node) {
    return NULL; // Memory allocation failed
  }

  // Initialize basic properties
  node->type = type;
  node->name = strdup(name);
  node->qualified_name = strdup(qualified_name);
  node->range = range;
  node->signature = NULL;
  node->docstring = NULL;
  node->raw_content = NULL;

  // Initialize relationships
  node->parent = NULL;
  node->children = NULL;
  node->num_children = 0;
  node->children_capacity = 0;
  node->references = NULL;
  node->num_references = 0;
  node->references_capacity = 0;
  node->additional_data = NULL;

  // Check if memory allocation for strings succeeded
  if (!node->name || !node->qualified_name) {
    ast_node_free(node);
    return NULL;
  }

  return node;
}

/* AST node cleanup - Implementation of the function declared in parser.h */
/* This implementation is already provided earlier in this file */

/* AST node relationship management */
bool ast_node_add_child(ASTNode *parent, ASTNode *child) {
#ifdef DEBUG_PARSER
  fprintf(stderr, "ast_node_add_child: parent=%p, child=%p, num_children=%zu, capacity=%zu\n", (void*)parent, (void*)child, parent ? parent->num_children : 0, parent ? parent->children_capacity : 0);
#endif
  if (!parent || !child) {
    return false;
  }

  // Check if the child already has a parent
  if (child->parent) {
    // Child already has a parent, can't add it to another parent
    return false;
  }

  // Check if we need to allocate or resize the children array
  if (!parent->children) {
    // Initial allocation
    parent->children_capacity = 4; // Start with space for 4 children
    parent->children = (ASTNode **)malloc(parent->children_capacity * sizeof(ASTNode *));
#ifdef DEBUG_PARSER
    fprintf(stderr, "ast_node_add_child: malloc children array, ptr=%p, capacity=%zu\n", (void*)parent->children, parent->children_capacity);
#endif
    if (!parent->children) {
      return false; // Memory allocation failed
    }
  } else if (parent->num_children >= parent->children_capacity) {
    // Need to resize
    size_t new_capacity = parent->children_capacity * 2;
    ASTNode **new_children =
        (ASTNode **)realloc(parent->children, new_capacity * sizeof(ASTNode *));
    if (!new_children) {
      return false; // Memory allocation failed
    }
    parent->children = new_children;
    parent->children_capacity = new_capacity;
#ifdef DEBUG_PARSER
    fprintf(stderr, "ast_node_add_child: realloc success, new array=%p, new capacity=%zu\n", (void*)parent->children, parent->children_capacity);
#endif
  }

  // Add the child
  parent->children[parent->num_children++] = child;
  child->parent = parent;
#ifdef DEBUG_PARSER
  fprintf(stderr, "ast_node_add_child: added child=%p, num_children now=%zu\n", (void*)child, parent->num_children);
  assert(parent->num_children <= parent->children_capacity);
#endif
  return true;
}

bool ast_node_add_reference(ASTNode *from, ASTNode *to) {
#ifdef DEBUG_PARSER
  fprintf(stderr, "ast_node_add_reference: from=%p, to=%p, num_references=%zu, capacity=%zu\n", (void*)from, (void*)to, from ? from->num_references : 0, from ? from->references_capacity : 0);
#endif
  if (!from || !to) {
    return false;
  }

  // Check if we need to allocate or resize the references array
  if (!from->references) {
    // Initial allocation
    from->references_capacity = 4; // Start with space for 4 references
    from->references = (ASTNode **)malloc(from->references_capacity * sizeof(ASTNode *));
#ifdef DEBUG_PARSER
    fprintf(stderr, "ast_node_add_reference: malloc references array, ptr=%p, capacity=%zu\n", (void*)from->references, from->references_capacity);
#endif
    if (!from->references) {
      return false; // Memory allocation failed
    }
  } else if (from->num_references >= from->references_capacity) {
    // Need to resize
    size_t new_capacity = from->references_capacity * 2;
    ASTNode **new_references =
        (ASTNode **)realloc(from->references, new_capacity * sizeof(ASTNode *));
    if (!new_references) {
      return false; // Memory allocation failed
    }
    from->references = new_references;
    from->references_capacity = new_capacity;
#ifdef DEBUG_PARSER
    fprintf(stderr, "ast_node_add_reference: realloc success, new array=%p, new capacity=%zu\n", (void*)from->references, from->references_capacity);
#endif
  }

  // Add the reference
  from->references[from->num_references++] = to;
#ifdef DEBUG_PARSER
  fprintf(stderr, "ast_node_add_reference: added to=%p, num_references now=%zu\n", (void*)to, from->num_references);
  assert(from->num_references <= from->references_capacity);
#endif
  return true;
}
