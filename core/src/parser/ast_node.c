/**
 * @file ast_node.c
 * @brief Implementation of AST node lifecycle and management functions
 *
 * This file contains implementation for AST node creation, manipulation,
 * and memory management to ensure proper handling of abstract syntax trees.
 */

#include "ast_node.h"
#include "../../core/include/scopemux/ast.h"
#include "../../core/include/scopemux/logging.h"
#include "../../core/include/scopemux/memory_debug.h"
#include "parser_internal.h"
#include <stdlib.h>
#include <string.h>

/**
 * Convert an ASTNodeType enum value to its canonical string representation
 *
 * This function maps internal enum values to their canonical schema string
 * representations as defined in the AST/CST schema documentation.
 *
 * @param type The ASTNodeType enum value
 * @return The canonical string representation (e.g., "ROOT", "FUNCTION", etc.)
 */
const char *ast_node_type_to_string(ASTNodeType type) {
  switch (type) {
  case NODE_ROOT:
    return "ROOT";
  case NODE_FUNCTION:
    return "FUNCTION";
  case NODE_CLASS:
    return "CLASS";
  case NODE_METHOD:
    return "METHOD";
  case NODE_VARIABLE:
    return "VARIABLE";
  case NODE_PARAMETER:
    return "PARAMETER";
  case NODE_IDENTIFIER:
    return "IDENTIFIER";
  case NODE_IMPORT:
    return "IMPORT";
  case NODE_INCLUDE:
    return "INCLUDE";
  case NODE_MODULE:
    return "MODULE";
  case NODE_VARIABLE_DECLARATION:
    return "VARIABLE_DECLARATION";
  case NODE_FOR_STATEMENT:
    return "FOR_STATEMENT";
  case NODE_WHILE_STATEMENT:
    return "WHILE_STATEMENT";
  case NODE_DO_WHILE_STATEMENT:
    return "DO_WHILE_STATEMENT";
  case NODE_IF_STATEMENT:
    return "IF_STATEMENT";
  case NODE_IF_ELSE_IF_STATEMENT:
    return "IF_ELSE_IF_STATEMENT";
  case NODE_SWITCH_STATEMENT:
    return "SWITCH_STATEMENT";
  case NODE_COMMENT:
    return "COMMENT";
  case NODE_DOCSTRING:
    return "DOCSTRING";
  case NODE_NAMESPACE:
    return "NAMESPACE";
  case NODE_STRUCT:
    return "STRUCT";
  case NODE_ENUM:
    return "ENUM";
  case NODE_INTERFACE:
    return "INTERFACE";
  case NODE_UNION:
    return "UNION";
  case NODE_TYPEDEF:
    return "TYPEDEF";
  case NODE_MACRO:
    return "MACRO";
  case NODE_CONTROL_FLOW:
    return "CONTROL_FLOW";
  case NODE_TEMPLATE_SPECIALIZATION:
    return "TEMPLATE_SPECIALIZATION";
  case NODE_LAMBDA:
    return "LAMBDA";
  case NODE_USING:
    return "USING";
  case NODE_FRIEND:
    return "FRIEND";
  case NODE_OPERATOR:
    return "OPERATOR";
  case NODE_UNKNOWN:
  default:
    return "UNKNOWN";
  }
}

/**
 * Recursively free an AST node's resources, but not its children
 * This is different from ast_node_free which also frees children
 */
void ast_node_free_internal(ASTNode *node) {
    fprintf(stderr, "[TEST] ast_node_free_internal reached for node at %p\n", (void*)node);

    log_debug("[TEST] ast_node_free_internal reached for node at %p", (void*)node);

    log_debug("[ast_node_free_internal] Freeing ASTNode at %p (magic=0x%X owned_fields=0x%X)", (void*)node, node ? node->magic : 0, node ? node->owned_fields : 0);

  if (!node) {
    log_debug("Skipping free for NULL ASTNode");
    return;
  }

  // First check memory canary to detect buffer overflows
  if (!memory_debug_check_canary(node, sizeof(ASTNode))) {
    log_error("Memory corruption detected in ASTNode at %p (buffer overflow)", (void *)node);
  }

  // Validate magic number to detect double-frees or invalid pointers
  if (node->magic != ASTNODE_MAGIC) {
    if (node->magic == 0) {
      log_error("Double-free detected: attempt to free already freed ASTNode at %p", (void *)node);
    } else {
      log_error("Invalid magic number in ASTNode at %p: expected 0x%X, found 0x%X", (void *)node,
                ASTNODE_MAGIC, node->magic);
    }
    // NOTE: Defensive: Do not proceed with freeing if magic is invalid
    return;
  }

  // Set the magic number to a known bad value after freeing
  node->magic = 0xDEADBEEF;

  // Free string fields based on their source
  if (node->name && node->name_source == AST_SOURCE_DEBUG_ALLOC) {
    memory_debug_free(node->name, __FILE__, __LINE__);
  }
  if (node->qualified_name && node->qualified_name_source == AST_SOURCE_DEBUG_ALLOC) {
    memory_debug_free(node->qualified_name, __FILE__, __LINE__);
  }
  if (node->signature && node->signature_source == AST_SOURCE_DEBUG_ALLOC) {
    memory_debug_free(node->signature, __FILE__, __LINE__);
  }
  if (node->docstring && node->docstring_source == AST_SOURCE_DEBUG_ALLOC) {
    memory_debug_free(node->docstring, __FILE__, __LINE__);
  }
  if (node->raw_content && (node->owned_fields & FIELD_RAW_CONTENT)) {
    log_debug("Freeing raw_content for ASTNode at %p (owned_fields=0x%X)", (void*)node, node->owned_fields);
    memory_debug_free(node->raw_content, __FILE__, __LINE__);
    node->raw_content = NULL;
    node->owned_fields &= ~FIELD_RAW_CONTENT;
  }
  if (node->file_path && node->file_path_source == AST_SOURCE_DEBUG_ALLOC) {
    memory_debug_free(node->file_path, __FILE__, __LINE__);
  }

  // Free additional data if it exists
  if (node->additional_data) { // Assuming always owned
    memory_debug_free(node->additional_data, __FILE__, __LINE__);
  }
}

/**
 * Set the file path of an AST node
 * @param node The node to modify
 * @param file_path The file path to set (will be copied)
 * @return true on success, false on allocation failure
 */
bool ast_node_set_file_path(ASTNode *node, char *file_path, ASTStringSource source) {
  if (!node || !file_path) {
    log_error("Cannot set file path: invalid parameters");
    return false;
  }

  // Free existing file path if it's owned
  if (node->file_path && node->file_path_source == AST_SOURCE_DEBUG_ALLOC) {
    memory_debug_free(node->file_path, __FILE__, __LINE__);
  }

  node->file_path = file_path;
  node->file_path_source = source;
  return true;
}

/**
 * Get the file path of an AST node
 * @param node The node to query
 * @return The file path, or NULL if not set
 */
const char *ast_node_get_file_path(const ASTNode *node) {
  if (!node)
    return NULL;
  return node->file_path;
}

/**
 * Creates a new AST node with the specified type and name.
 */
ASTNode *ast_node_new(ASTNodeType type, char *name, ASTStringSource name_source) {
  // Allocate memory for the node with memory tracking
  ASTNode *node = (ASTNode *)memory_debug_malloc(sizeof(ASTNode), __FILE__, __LINE__, "ast_node");
  if (!node) {
    log_error("Failed to allocate memory for ASTNode");
    return NULL;
  }

  // Initialize all fields to safe defaults
  memset(node, 0, sizeof(ASTNode));

  // Set the magic number for validation
  node->magic = ASTNODE_MAGIC;
  node->type = type;

  // Set name and its source
  node->name = name;
  node->name_source = name_source;

  // Initialize other sources
  node->qualified_name_source = AST_SOURCE_NONE;
  node->signature_source = AST_SOURCE_NONE;
  node->docstring_source = AST_SOURCE_NONE;
  node->file_path_source = AST_SOURCE_NONE;

  // Initialize memory canary at the end of the struct
  memory_debug_set_canary(node, sizeof(ASTNode));

  return node;
}

/**
 * Frees an AST node and all its resources recursively.
 */
void ast_node_free(ASTNode *node) {
    fprintf(stderr, "[TEST] ast_node_free reached for node at %p\n", (void*)node);

    log_debug("[ast_node_free] Called for node at %p (magic=0x%X)", (void*)node, node ? node->magic : 0);

  if (!node) {
    log_debug("Skipping free for NULL ASTNode");
    return;
  }

  // First check memory canary to detect buffer overflows
  if (!memory_debug_check_canary(node, sizeof(ASTNode))) {
    log_error("Memory corruption detected in ASTNode at %p (buffer overflow)", (void *)node);
  }

  // Validate magic number to detect double-frees
  if (node->magic != ASTNODE_MAGIC) {
    if (node->magic == 0) {
      log_error("Double-free detected: attempt to free already freed ASTNode at %p", (void *)node);
    } else {
      log_error("Invalid magic number in ASTNode at %p: expected 0x%X, found 0x%X", (void *)node,
                ASTNODE_MAGIC, node->magic);
    }
    // NOTE: Defensive: Do not proceed with freeing if magic is invalid
    return;
  }

  // Free internal resources (name, signature, etc.)
  ast_node_free_internal(node);

  // Free all children recursively
  if (node->children && node->num_children > 0) {
    for (size_t i = 0; i < node->num_children; i++) {
      if (node->children[i]) {
        ast_node_free(node->children[i]);
        node->children[i] = NULL; // Prevent double-free
      }
    }
    memory_debug_free(node->children, __FILE__, __LINE__);
    node->children = NULL;
  }
  if (node->references) {
    node->references = NULL;
  }
  // Finally free the node itself
  memory_debug_free(node, __FILE__, __LINE__);
}

/**
 * Add a child AST node to a parent node.
 */
bool ast_node_add_child(ASTNode *parent, ASTNode *child) {
  if (!parent || !child) {
    log_error("Cannot add child: %s", !parent ? "parent is NULL" : "child is NULL");
    return false;
  }

  // Check if we need to allocate or resize the children array
  if (!parent->children) {
    // Initial allocation
    parent->children_capacity = 4; // Start with space for 4 children
    parent->children = (ASTNode **)memory_debug_malloc(
        parent->children_capacity * sizeof(ASTNode *), __FILE__, __LINE__, "ast_node_children");
    if (!parent->children) {
      log_error("Memory allocation failed for ASTNode children array");
      parent->children_capacity = 0;
      return false;
    }
  } else if (parent->num_children >= parent->children_capacity) {
    // Need to resize
    size_t new_capacity = parent->children_capacity * 2;
    ASTNode **new_children =
        (ASTNode **)memory_debug_realloc(parent->children, new_capacity * sizeof(ASTNode *),
                                         __FILE__, __LINE__, "ast_node_children");
    if (!new_children) {
      log_error("Memory reallocation failed for ASTNode children array");
      return false;
    }
    parent->children = new_children;
    parent->children_capacity = new_capacity;
  }

  // Set the parent-child relationship
  parent->children[parent->num_children++] = child;
  child->parent = parent;

  return true;
}

/**
 * Add a reference from one AST node to another.
 */
bool ast_node_add_reference(ASTNode *from, ASTNode *to) {
  if (!from || !to) {
    log_error("Cannot add reference: %s", !from ? "source node is NULL" : "target node is NULL");
    return false;
  }

  // Check if we need to allocate or resize the references array
  if (!from->references) {
    // Initial allocation
    from->references_capacity = 4; // Start with space for 4 references
    from->references = (ASTNode **)memory_debug_malloc(
        from->references_capacity * sizeof(ASTNode *), __FILE__, __LINE__, "ast_node_references");
    if (!from->references) {
      log_error("Memory allocation failed for ASTNode references array");
      return false;
    }
  } else if (from->num_references >= from->references_capacity) {
    // Need to resize
    size_t new_capacity = from->references_capacity * 2;
    ASTNode **new_references =
        (ASTNode **)memory_debug_realloc(from->references, new_capacity * sizeof(ASTNode *),
                                         __FILE__, __LINE__, "ast_node_references");
    if (!new_references) {
      log_error("Memory reallocation failed for ASTNode references array");
      return false;
    }
    from->references = new_references;
    from->references_capacity = new_capacity;
  }

  // Add the reference
  from->references[from->num_references++] = to;
  return true;
}

/**
 * Create a new AST node with full attributes.
 */
ASTNode *ast_node_create(ASTNodeType type, char *name, ASTStringSource name_source, char *qualified_name,
                          ASTStringSource qualified_name_source, SourceRange range) {
  // Create node with name - this will set FIELD_NAME ownership if needed
  ASTNode *node = ast_node_new(type, name, name_source);
  if (!node) {
    return NULL;
  }

  // Set the source range
  node->range = range;

  // Take ownership of the qualified name if provided
  if (qualified_name) {
    node->qualified_name = qualified_name;
    node->qualified_name_source = qualified_name_source;
  }

  return node;
}

/**
 * Set a property on an AST node.
 *
 * Note: This is a simplified implementation. A full implementation would use
 * a proper dictionary data structure for the properties.
 */
bool ast_node_set_property(ASTNode *node, const char *key, const char *value) {
  if (!node || !key || !value) {
    log_error("Cannot set property: invalid parameters");
    return false;
  }

  // TODO: Implement a proper dictionary for properties
  // For now, just create a placeholder in additional_data if it doesn't exist
  if (!node->additional_data) {
    node->additional_data = memory_debug_malloc(1, __FILE__, __LINE__, "ast_property_placeholder");
    if (!node->additional_data) {
      log_error("Failed to allocate property placeholder");
      return false;
    }
    node->owned_fields |= FIELD_ADDITIONAL_DATA;
  }

  return true;
}

/**
 * Set an attribute on an AST node (alias for ast_node_set_property for test compatibility).
 */
bool ast_node_set_attribute(ASTNode *node, const char *key, const char *value) {
  return ast_node_set_property(node, key, value);
}

/**
 * Set the signature of an AST node
 * @param node The node to modify
 * @param signature The signature to set (will be copied)
 * @return true on success, false on allocation failure
 */
bool ast_node_set_signature(ASTNode *node, char *signature, ASTStringSource source) {
  if (!node) {
    log_error("Cannot set signature: null node");
    return false;
  }

  // Free existing signature if it's owned
  if (node->signature && node->signature_source == AST_SOURCE_DEBUG_ALLOC) {
    memory_debug_free(node->signature, __FILE__, __LINE__);
  }

  node->signature = signature;
  node->signature_source = source;
  return true;
}

/**
 * Set the docstring of an AST node
 * @param node The node to modify
 * @param docstring The docstring to set (will be copied)
 * @return true on success, false on allocation failure
 */
bool ast_node_set_docstring(ASTNode *node, char *docstring, ASTStringSource source) {
  if (!node) {
    log_error("Cannot set docstring: null node");
    return false;
  }

  // Free existing docstring if it's owned
  if (node->docstring && node->docstring_source == AST_SOURCE_DEBUG_ALLOC) {
    memory_debug_free(node->docstring, __FILE__, __LINE__);
  }

  node->docstring = docstring;
  node->docstring_source = source;
  return true;
}

static void ast_node_free_data(ASTNode *node) {
  if (!node) {
    return;
  }

  // Only free if source is AST_SOURCE_DEBUG_ALLOC; do NOT free if AST_SOURCE_ALIAS or AST_SOURCE_STATIC.
  if (node->name && node->name_source == AST_SOURCE_DEBUG_ALLOC) {
    FREE(node->name);
    node->name = NULL;
  }

  if (node->qualified_name && node->qualified_name_source == AST_SOURCE_DEBUG_ALLOC) {
    FREE(node->qualified_name);
    node->qualified_name = NULL;
  }

  if (node->signature && node->signature_source == AST_SOURCE_DEBUG_ALLOC) {
    FREE(node->signature);
    node->signature = NULL;
  }

  if (node->docstring && node->docstring_source == AST_SOURCE_DEBUG_ALLOC) {
    FREE(node->docstring);
    node->docstring = NULL;
  }

  if (node->raw_content && (node->owned_fields & FIELD_RAW_CONTENT)) {
    log_debug("[ast_node_free_data] Freeing raw_content for ASTNode at %p (owned_fields=0x%X)", (void*)node, node->owned_fields);
    FREE(node->raw_content);
    node->raw_content = NULL;
    node->owned_fields &= ~FIELD_RAW_CONTENT;
  }

  if (node->file_path && node->file_path_source == AST_SOURCE_DEBUG_ALLOC) {
    FREE(node->file_path);
    node->file_path = NULL;
  }

  if (node->additional_data) { // Assuming always owned
    FREE(node->additional_data);
    node->additional_data = NULL;
  }
}

static bool ast_node_init_additional_data(ASTNode *node) {
  if (!node) {
    return false;
  }

  if (!node->additional_data) {
    node->additional_data = MALLOC(1, "ast_property_placeholder");
    if (!node->additional_data) {
      return false;
    }
  }

  return true;
}
