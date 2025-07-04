/**
 * @file ast_node.c
 * @brief Implementation of AST node lifecycle and management functions
 *
 * This file contains implementation for AST node creation, manipulation,
 * and memory management to ensure proper handling of abstract syntax trees.
 */

#include "ast_node.h"
#include "parser_internal.h"

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
  }

  // Reset the magic number to prevent double-free
  node->magic = 0;

  // Free the name if it exists
  if (node->name) {
    memory_debug_free(node->name, __FILE__, __LINE__);
    node->name = NULL;
  }

  // Free the signature if it exists
  if (node->signature) {
    memory_debug_free(node->signature, __FILE__, __LINE__);
    node->signature = NULL;
  }

  // Free the qualified name if it exists
  if (node->qualified_name) {
    memory_debug_free(node->qualified_name, __FILE__, __LINE__);
    node->qualified_name = NULL;
  }

  // Free the docstring if it exists
  if (node->docstring) {
    memory_debug_free(node->docstring, __FILE__, __LINE__);
    node->docstring = NULL;
  }

  // Free the file_path if it exists
  if (node->file_path) {
    memory_debug_free(node->file_path, __FILE__, __LINE__);
    node->file_path = NULL;
  }

  // Free any additional data if it exists
  if (node->additional_data) {
    memory_debug_free(node->additional_data, __FILE__, __LINE__);
    node->additional_data = NULL;
  }

  // Properties handling is now done through the additional_data field
  // No specific properties dictionary to free

  // Note: children and references arrays are not freed here
  // They are handled by ast_node_free to ensure proper recursion
}

/**
 * Set the file path of an AST node
 * @param node The node to modify
 * @param file_path The file path to set (will be copied)
 * @return true on success, false on allocation failure
 */
bool ast_node_set_file_path(ASTNode *node, const char *file_path) {
  if (!node)
    return false;
  if (node->file_path) {
    memory_debug_free(node->file_path, __FILE__, __LINE__);
    node->file_path = NULL;
  }
  if (file_path) {
    node->file_path = strdup(file_path);
    if (!node->file_path)
      return false;
  }
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
ASTNode *ast_node_new(ASTNodeType type, const char *name) {
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

  // Copy the name if provided
  if (name) {
    node->name = strdup(name);
    if (!node->name) {
      log_error("Failed to duplicate AST node name");
      memory_debug_free(node, __FILE__, __LINE__);
      return NULL;
    }
  }

  return node;
}

/**
 * Frees an AST node and all its resources recursively.
 */
void ast_node_free(ASTNode *node) {
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
      return; // Skip further processing to avoid crashes
    } else {
      log_error("Invalid magic number in ASTNode at %p: expected 0x%X, found 0x%X", (void *)node,
                ASTNODE_MAGIC, node->magic);
      // Continue with caution
    }
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
    node->num_children = 0;
    node->children_capacity = 0;
  }

  // Free the references array (but not the referenced nodes)
  if (node->references) {
    memory_debug_free(node->references, __FILE__, __LINE__);
    node->references = NULL;
    node->num_references = 0;
    node->references_capacity = 0;
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
ASTNode *ast_node_create(ASTNodeType type, const char *name, const char *qualified_name,
                         SourceRange range) {
  ASTNode *node = ast_node_new(type, name);
  if (!node) {
    return NULL;
  }

  // Set the source range
  node->range = range;

  // Copy the qualified name if provided
  if (qualified_name) {
    node->qualified_name = strdup(qualified_name);
    if (!node->qualified_name) {
      log_error("Failed to duplicate qualified name");
      ast_node_free(node);
      return NULL;
    }
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
      log_error("Failed to allocate memory for property placeholder");
      return false;
    }
  }

  return true;
}

/**
 * Set an attribute on an AST node (alias for ast_node_set_property for test compatibility).
 */
bool ast_node_set_attribute(ASTNode *node, const char *key, const char *value) {
  return ast_node_set_property(node, key, value);
}
