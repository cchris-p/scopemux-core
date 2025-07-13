/**
 * node_type_mapping_loader.c
 *
 * Hardcoded node type mapping system for ScopeMux.
 *
 * This module provides a simple, reliable mapping from semantic query types
 * (e.g., "functions", "classes") to ASTNodeType enums. All mappings are
 * hardcoded in the source code for maximum reliability and build reproducibility.
 *
 * Key features:
 *   - No external dependencies or config files
 *   - All mappings are hardcoded in the source code
 *   - Simple string comparison for lookups
 *   - Memory-safe with proper error handling
 *   - Thread-safe (read-only after initialization)
 *
 * See also: core/include/config/node_type_mapping_loader.h
 */
// Define _POSIX_C_SOURCE to make strdup available
#define _POSIX_C_SOURCE 200809L

#include "config/node_type_mapping_loader.h"
#include "../../core/include/scopemux/parser.h"
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scopemux/memory_debug.h"

// Simple hash table for mapping query_type -> ASTNodeType
#define MAX_MAPPINGS 32

typedef struct {
  char *query_type;
  ASTNodeType node_type;
} NodeTypeMapping;

static NodeTypeMapping mappings[MAX_MAPPINGS] = {0};
static int mapping_count = 0;

static ASTNodeType parse_node_type(const char *enum_str) {
  if (strcmp(enum_str, "NODE_FUNCTION") == 0)
    return NODE_FUNCTION;
  if (strcmp(enum_str, "NODE_CLASS") == 0)
    return NODE_CLASS;
  if (strcmp(enum_str, "NODE_METHOD") == 0)
    return NODE_METHOD;
  if (strcmp(enum_str, "NODE_VARIABLE") == 0)
    return NODE_VARIABLE;
  if (strcmp(enum_str, "NODE_MODULE") == 0)
    return NODE_MODULE;
  if (strcmp(enum_str, "NODE_UNKNOWN") == 0)
    return NODE_UNKNOWN;
  if (strcmp(enum_str, "NODE_STRUCT") == 0)
    return NODE_STRUCT;
  if (strcmp(enum_str, "NODE_UNION") == 0)
    return NODE_UNION;
  if (strcmp(enum_str, "NODE_ENUM") == 0)
    return NODE_ENUM;
  if (strcmp(enum_str, "NODE_TYPEDEF") == 0)
    return NODE_TYPEDEF;
  if (strcmp(enum_str, "NODE_INCLUDE") == 0)
    return NODE_INCLUDE;
  if (strcmp(enum_str, "NODE_MACRO") == 0)
    return NODE_MACRO;
  if (strcmp(enum_str, "NODE_DOCSTRING") == 0)
    return NODE_DOCSTRING;
  if (strcmp(enum_str, "NODE_INTERFACE") == 0)
    return NODE_INTERFACE;
  if (strcmp(enum_str, "NODE_TEMPLATE_SPECIALIZATION") == 0)
    return NODE_TEMPLATE_SPECIALIZATION;
  return NODE_UNKNOWN;
}

/**
 * @brief Loads the node type mapping from hardcoded defaults.
 *
 * This function uses hardcoded mappings instead of loading from any external files.
 * This is the source of truth for node type mappings in ScopeMux.
 */
void load_node_type_mapping(void) {
  fprintf(stderr, "[scopemux] INFO: Loading hardcoded node type mappings\n");

  printf("[scopemux] Loading hardcoded node type mappings:\n");

  // Define hardcoded mappings
  // These are the core mappings needed for the parser to work correctly
  struct {
    const char *query_type;
    const char *node_type_str;
  } default_mappings[] = {
      {"functions", "NODE_FUNCTION"},
      {"classes", "NODE_CLASS"},
      {"methods", "NODE_METHOD"},
      {"variables", "NODE_VARIABLE"},
      {"modules", "NODE_MODULE"},
      {"structs", "NODE_STRUCT"},
      {"unions", "NODE_UNION"},
      {"enums", "NODE_ENUM"},
      {"typedefs", "NODE_TYPEDEF"},
      {"includes", "NODE_INCLUDE"},
      {"macros", "NODE_MACRO"},
      {"docstrings", "NODE_DOCSTRING"},
      {"interfaces", "NODE_INTERFACE"},
      {"template_specializations", "NODE_TEMPLATE_SPECIALIZATION"},
      // Add any additional mappings here
  };

  // Clear existing mappings
  for (int i = 0; i < mapping_count; ++i) {
    if (mappings[i].query_type) {
      memory_debug_free(mappings[i].query_type, __FILE__, __LINE__);
      mappings[i].query_type = NULL;
    }
  }
  mapping_count = 0;

  // Load the hardcoded mappings
  size_t num_default_mappings = sizeof(default_mappings) / sizeof(default_mappings[0]);
  for (size_t i = 0; i < num_default_mappings && mapping_count < MAX_MAPPINGS; ++i) {
    const char *query_type = default_mappings[i].query_type;
    const char *node_type_str = default_mappings[i].node_type_str;

    // Skip if either is NULL
    if (!query_type || !node_type_str) {
      fprintf(stderr, "[scopemux] ERROR: NULL mapping entry at index %zu, skipping\n", i);
      continue;
    }

    // Safe string duplication with error checking
    size_t query_type_len = strlen(query_type);
    char *query_type_copy = (char *)memory_debug_malloc(query_type_len + 1, __FILE__, __LINE__, "query_type_mapping");
    if (!query_type_copy) {
      fprintf(stderr, "[scopemux] ERROR: Failed to allocate memory for query type: %s\n",
              query_type);
      continue;
    }
    strncpy(query_type_copy, query_type, query_type_len);
    query_type_copy[query_type_len] = '\0';

    // Validate the mapping before storing
    ASTNodeType node_type = parse_node_type(node_type_str);
    if (node_type == NODE_UNKNOWN && strcmp(node_type_str, "NODE_UNKNOWN") != 0) {
      fprintf(stderr, "[scopemux] WARNING: Unknown node type: %s, defaulting to NODE_UNKNOWN\n",
              node_type_str);
    }

    // Store the mapping
    mappings[mapping_count].query_type = query_type_copy;
    mappings[mapping_count].node_type = node_type;
    printf("  %s -> %s\n", query_type, node_type_str);
    mapping_count++;
  }

  // Log summary
  fprintf(stderr, "[scopemux] Loaded %d hardcoded node type mappings\n", mapping_count);
}

/**
 * @brief Gets the ASTNodeType for a given query type string.
 * @param query_type The query type string (e.g., "functions").
 * @return The corresponding ASTNodeType, or NODE_UNKNOWN if not found.
 *
 * Logs a warning if the mapping is missing.
 */
ASTNodeType get_node_type_for_query(const char *query_type) {
  // **CRITICAL SAFETY CHECK**: Validate input parameter
  if (!query_type) {
    fprintf(stderr, "[scopemux] ERROR: NULL query_type passed to get_node_type_for_query\n");
    return NODE_UNKNOWN;
  }

  // **CRITICAL SAFETY CHECK**: Validate mapping state
  if (mapping_count <= 0 || mapping_count > MAX_MAPPINGS) {
    fprintf(stderr, "[scopemux] ERROR: Invalid mapping_count: %d (max: %d)\n", mapping_count,
            MAX_MAPPINGS);
    return NODE_UNKNOWN;
  }

  // **CRITICAL SAFETY CHECK**: Validate each mapping before string comparison
  for (int i = 0; i < mapping_count; ++i) {
    // Check for corrupted mapping entry
    if (!mappings[i].query_type) {
      fprintf(stderr, "[scopemux] ERROR: NULL query_type in mapping[%d], skipping\n", i);
      continue;
    }

    // **SAFE STRING COMPARISON** with additional validation
    if (strcmp(mappings[i].query_type, query_type) == 0) {
      // Validate the node type before returning
      if (mappings[i].node_type >= 0 && mappings[i].node_type < 256) { // Reasonable bounds
        return mappings[i].node_type;
      } else {
        fprintf(stderr, "[scopemux] ERROR: Invalid node_type %d for query '%s'\n",
                mappings[i].node_type, query_type);
        return NODE_UNKNOWN;
      }
    }
  }

  // Log missing mapping (this is expected for some query types)
  fprintf(stderr, "[scopemux] WARNING: No AST node type mapping for query type: '%s'\n",
          query_type);
  return NODE_UNKNOWN;
}

/**
 * @brief Frees all memory used by the node type mapping.
 */
void free_node_type_mapping(void) {
  for (int i = 0; i < mapping_count; ++i) {
    free(mappings[i].query_type);
  }
  mapping_count = 0;
}
