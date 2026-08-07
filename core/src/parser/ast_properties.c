/**
 * @file ast_properties.c
 * @brief Implementation of AST node property management functions
 *
 * This file provides functions for setting and retrieving properties
 * on AST nodes, which are used for storing metadata such as reference
 * relationships and other semantic information.
 */

#include "scopemux/ast.h"
#include "scopemux/logging.h"
#include "scopemux/memory_debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Set a property on an AST node
 *
 * This function sets a named property on an AST node. If the property
 * already exists, it will be updated. Otherwise, a new property will
 * be created.
 *
 * @param node The AST node to set the property on
 * @param name The name of the property
 * @param value The value of the property
 * @return true if successful, false otherwise
 */
bool ast_node_set_property(ASTNode *node, const char *name, const char *value) {
  if (!node || !name || !value) {
    return false;
  }

  // Check if the property already exists
  for (size_t i = 0; i < node->num_properties; i++) {
    if (node->property_names[i] && strcmp(node->property_names[i], name) == 0) {
      // Update existing property
      FREE(node->property_values[i]);
      node->property_values[i] = STRDUP(value, "ast_property_value_update");
      return node->property_values[i] != NULL;
    }
  }

  // Need to add a new property
  if (node->num_properties >= node->properties_capacity) {
    // Resize property arrays
    size_t new_capacity = node->properties_capacity * 2;
    if (new_capacity == 0) {
      new_capacity = 4; // Initial capacity
    }

    char **new_names = (char **)REALLOC(node->property_names, new_capacity * sizeof(char *),
                                        "ast_property_names_resize");
    if (!new_names) {
      log_error("Failed to resize property names array");
      return false;
    }

    char **new_values = (char **)REALLOC(node->property_values, new_capacity * sizeof(char *),
                                         "ast_property_values_resize");
    if (!new_values) {
      log_error("Failed to resize property values array");
      // Roll back the first realloc
      node->property_names = (char **)REALLOC(new_names, node->properties_capacity * sizeof(char *),
                                              "ast_property_names_rollback");
      return false;
    }

    node->property_names = new_names;
    node->property_values = new_values;
    node->properties_capacity = new_capacity;
  }

  // Add the new property
  node->property_names[node->num_properties] = STRDUP(name, "ast_property_name_new");
  if (!node->property_names[node->num_properties]) {
    log_error("Failed to allocate memory for property name");
    return false;
  }

  node->property_values[node->num_properties] = STRDUP(value, "ast_property_value_new");
  if (!node->property_values[node->num_properties]) {
    log_error("Failed to allocate memory for property value");
    FREE(node->property_names[node->num_properties]);
    node->property_names[node->num_properties] = NULL;
    return false;
  }

  node->num_properties++;
  return true;
}

/**
 * Get a property from an AST node
 *
 * This function retrieves a named property from an AST node.
 *
 * @param node The AST node to get the property from
 * @param name The name of the property
 * @return The value of the property, or NULL if not found
 */
const char *ast_node_get_property(const ASTNode *node, const char *name) {
  if (!node || !name) {
    return NULL;
  }

  for (size_t i = 0; i < node->num_properties; i++) {
    if (node->property_names[i] && strcmp(node->property_names[i], name) == 0) {
      return node->property_values[i];
    }
  }

  return NULL;
}

/**
 * Remove a property from an AST node
 *
 * This function removes a named property from an AST node.
 *
 * @param node The AST node to remove the property from
 * @param name The name of the property
 * @return true if the property was removed, false if not found
 */
bool ast_node_remove_property(ASTNode *node, const char *name) {
  if (!node || !name) {
    return false;
  }

  for (size_t i = 0; i < node->num_properties; i++) {
    if (node->property_names[i] && strcmp(node->property_names[i], name) == 0) {
      // Free the memory
      FREE(node->property_names[i]);
      FREE(node->property_values[i]);

      // Shift remaining properties
      for (size_t j = i; j < node->num_properties - 1; j++) {
        node->property_names[j] = node->property_names[j + 1];
        node->property_values[j] = node->property_values[j + 1];
      }

      // Clear the last element and decrement count
      node->property_names[node->num_properties - 1] = NULL;
      node->property_values[node->num_properties - 1] = NULL;
      node->num_properties--;

      return true;
    }
  }

  return false;
}
