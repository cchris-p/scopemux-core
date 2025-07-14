#include "language.h"
#include "source_range.h"
/**
 * @file ast.h
 * @brief Abstract Syntax Tree (AST) definitions for ScopeMux
 *
 * This header defines the common AST structures used throughout ScopeMux,
 * including node types, creation/destruction functions, and traversal utilities.
 */

#ifndef SCOPEMUX_AST_H
#define SCOPEMUX_AST_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * AST node types used for language-agnostic representation
 */
typedef enum {
  NODE_UNKNOWN = 0,
  NODE_ROOT = 1,
  NODE_FUNCTION = 2,
  NODE_CLASS = 3,
  NODE_METHOD = 4,
  NODE_VARIABLE = 5,
  NODE_PARAMETER = 6,
  NODE_IDENTIFIER = 7,
  NODE_IMPORT = 8,
  NODE_INCLUDE = 9,
  NODE_MODULE = 10,
  NODE_VARIABLE_DECLARATION = 11,
  NODE_FOR_STATEMENT = 12,
  NODE_WHILE_STATEMENT = 13,
  NODE_DO_WHILE_STATEMENT = 14,
  NODE_IF_STATEMENT = 15,
  NODE_IF_ELSE_IF_STATEMENT = 16,
  NODE_SWITCH_STATEMENT = 17,
  NODE_COMMENT = 18,
  NODE_DOCSTRING = 19,
  NODE_NAMESPACE = 20,
  NODE_STRUCT = 21,
  NODE_ENUM = 22,
  NODE_INTERFACE = 23,
  NODE_UNION = 24,   // C/C++ union
  NODE_TYPEDEF = 25, // typedef declaration
  NODE_MACRO = 26,   // macro definition
  NODE_CONTROL_FLOW = 27,
  NODE_TEMPLATE_SPECIALIZATION = 28,
  NODE_LAMBDA = 29,
  NODE_USING = 30,
  NODE_FRIEND = 31,
  NODE_OPERATOR = 32,
  NODE_TYPE = 33,     // Type node (used in type annotations or declarations)
  NODE_PROPERTY = 34, // Property or field inside a class or struct
  /* Add more node types as needed */
} ASTNodeType;

/**
 * @enum ASTStringSource
 * @brief Describes the allocation source of a string field in an ASTNode.
 *
 * AST_SOURCE_ALIAS is used if a string field points to the same buffer as another field (e.g., node_name aliases docstring).
 * In this case, only the original field (with AST_SOURCE_DEBUG_ALLOC) will be freed, and the alias will not be freed.
 */
typedef enum {
    AST_SOURCE_NONE,         /**< Source is unknown or not set */
    AST_SOURCE_DEBUG_ALLOC,  /**< String was allocated by memory_debug_malloc and must be freed */
    AST_SOURCE_STATIC,       /**< String is a static literal or library-managed, and must not be freed */
    AST_SOURCE_ALIAS,        /**< String is an alias of another field (do not free; freeing handled by original owner) */
} ASTStringSource;

/**
 * Abstract Syntax Tree node structure
 */
typedef struct ASTNode {
  uint32_t magic;   /**< Canary for heap corruption/use-after-free detection */
  ASTNodeType type; /**< Type of the node */

  // Memory ownership flags for string fields
  uint32_t owned_fields; /**< Bitfield tracking which string fields we own and must free */
#define FIELD_NAME (1 << 0)
#define FIELD_QUALIFIED_NAME (1 << 1)
#define FIELD_SIGNATURE (1 << 2)
#define FIELD_DOCSTRING (1 << 3)
#define FIELD_RAW_CONTENT (1 << 4)
#define FIELD_FILE_PATH (1 << 5)
#define FIELD_ADDITIONAL_DATA (1 << 6)

  char *name;           /**< Name of the entity */
  ASTStringSource name_source;

  char *qualified_name; /**< Fully qualified name (e.g., namespace::class::method) */
  ASTStringSource qualified_name_source;

  SourceRange range;    /**< Source code range */
  char *signature;      /**< Function/method signature if applicable */
  ASTStringSource signature_source;

  char *docstring;      /**< Associated documentation */
  ASTStringSource docstring_source;

  char *raw_content;    /**< Raw source code content */
  char *file_path;      /**< Source file path (new field) */
  ASTStringSource file_path_source;

  struct ASTNode *parent;    /**< Parent node (e.g., class for a method) */
  struct ASTNode **children; /**< Child nodes (e.g., methods for a class) */
  size_t num_children;       /**< Number of children */
  size_t children_capacity;  /**< Capacity of children array */

  struct ASTNode **references; /**< Nodes referenced by this node */
  size_t num_references;       /**< Number of references */
  size_t references_capacity;  /**< Capacity of references array */

  void *additional_data;        /**< Language-specific or analysis data */
  Language lang;                /**< Language type */
  size_t ast_node_set_property; /**< Number of properties set on this node */
  char **property_names;        /**< Names of properties */
  char **property_values;       /**< Values of properties */
  size_t properties_capacity;   /**< Capacity of property arrays */
  size_t num_properties;        /**< Number of properties set on this node */
} ASTNode;

/**
 * Create a new AST node with the given type
 * @param type The type of node to create
 * @return A new AST node with the given type, or NULL on allocation failure
 */
ASTNode *ast_node_new(ASTNodeType type, char *name, ASTStringSource name_source);
ASTNode *ast_node_create(ASTNodeType type, char *name, ASTStringSource name_source, char *qualified_name, ASTStringSource qualified_name_source, SourceRange range);

/**
 * Convert an ASTNodeType enum value to its canonical string representation
 * @param type The ASTNodeType enum value
 * @return The canonical string representation (e.g., "ROOT", "FUNCTION", etc.)
 */
const char *ast_node_type_to_string(ASTNodeType type);

/**
 * Free an AST node and all its children recursively
 * @param node The node to free
 */
void ast_node_free(ASTNode *node);

/**
 * Add a child node to a parent node
 * @param parent The parent node
 * @param child The child node to add
 * @return true on success, false on allocation failure
 */
bool ast_node_add_child(ASTNode *parent, ASTNode *child);

/**
 * Set the name of an AST node
 * @param node The node to modify
 * @param name The name to set (will be copied)
 * @return true on success, false on allocation failure
 */
bool ast_node_set_name(ASTNode *node, char *name, ASTStringSource source);

/**
 * Set the file path of an AST node
 * @param node The node to modify
 * @param file_path The file path to set (will be copied)
 * @return true on success, false on allocation failure
 */
bool ast_node_set_file_path(ASTNode *node, char *file_path, ASTStringSource source);

/**
 * Get the file path of an AST node
 * @param node The node to query
 * @return The file path, or NULL if not set
 */
const char *ast_node_get_file_path(const ASTNode *node);

/**
 * Set the signature of an AST node
 * @param node The node to modify
 * @param signature The signature to set (will be copied)
 * @return true on success, false on allocation failure
 */
bool ast_node_set_signature(ASTNode *node, char *signature, ASTStringSource source);

/**
 * Set the docstring of an AST node
 * @param node The node to modify
 * @param docstring The docstring to set (will be copied)
 * @return true on success, false on allocation failure
 */
bool ast_node_set_docstring(ASTNode *node, char *docstring, ASTStringSource source);

/**
 * Set the qualified name of an AST node
 * @param node The node to modify
 * @param qualified_name The qualified name to set (will be copied)
 * @return true on success, false on allocation failure
 */
bool ast_node_set_qualified_name(ASTNode *node, char *qualified_name, ASTStringSource source);

/**
 * Set an attribute on an AST node
 * @param node The node to modify
 * @param key The attribute key
 * @param value The attribute value (will be copied)
 * @return true on success, false on allocation failure
 */
bool ast_node_set_attribute(ASTNode *node, const char *key, const char *value);

/**
 * Find a child node by name
 * @param node The parent node to search in
 * @param name The name to search for
 * @return The found child node, or NULL if not found
 */
ASTNode *ast_node_find_child(const ASTNode *node, const char *name);

/**
 * Find a node by path from a root node
 * @param root The root node to start the search from
 * @param path The dot-separated path to search for (e.g., "class.method")
 * @return The found node, or NULL if not found
 */
ASTNode *ast_node_find_by_path(const ASTNode *root, const char *path);

/**
 * Clone an AST node and all its children
 * @param node The node to clone
 * @return A deep copy of the node and its children, or NULL on failure
 */
ASTNode *ast_node_clone(const ASTNode *node);

// Language type and functions are now imported from parser.h

#ifdef __cplusplus
}
#endif

#endif /* SCOPEMUX_AST_H */
