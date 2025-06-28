// Define _POSIX_C_SOURCE to make strdup available
#define _POSIX_C_SOURCE 200809L

#include "config/node_type_mapping_loader.h"
#include "../../core/include/scopemux/parser.h"
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Simple hash table for mapping query_type -> ASTNodeType
#define MAX_MAPPINGS 32

typedef struct {
  char *query_type;
  ASTNodeType node_type;
} NodeTypeMapping;

static NodeTypeMapping mappings[MAX_MAPPINGS];
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
 * @brief Loads the node type mapping from a JSON config file.
 * @param config_path Path to the JSON config file.
 *
 * If the file cannot be opened, logs an error and leaves the mapping empty.
 */
void load_node_type_mapping(const char *config_path) {
  FILE *f = fopen(config_path, "r");
  if (!f) {
    fprintf(stderr, "[scopemux] ERROR: Failed to open node type mapping config: %s\n", config_path);
    return;
  }
  char buf[4096];
  size_t len = fread(buf, 1, sizeof(buf) - 1, f);
  fclose(f);
  buf[len] = '\0';
  char *p = buf;
  printf("[scopemux] Loaded node type mapping config (showing all mappings):\n");
  // Minimal JSON object parser: expects { "key": "value", ... }
  // Does NOT support nested objects or arrays. Only works for flat string:string mappings.
  while (*p) {
    // Skip whitespace and delimiters
    while (*p && (isspace((unsigned char)*p) || *p == '{' || *p == ','))
      p++;
    if (*p == '"') {
      // Parse key
      char *key_start = ++p;
      while (*p && *p != '"')
        p++;
      if (!*p)
        break;
      size_t klen = p - key_start;
      char key[64];
      strncpy(key, key_start, klen);
      key[klen] = '\0';
      p++; // skip closing quote
      while (*p && (isspace((unsigned char)*p) || *p == ':'))
        p++;
      if (*p != '"') {
        // Malformed JSON, skip to next
        while (*p && *p != ',')
          p++;
        continue;
      }
      // Parse value
      char *val_start = ++p;
      while (*p && *p != '"')
        p++;
      if (!*p)
        break;
      size_t vlen = p - val_start;
      char val[64];
      strncpy(val, val_start, vlen);
      val[vlen] = '\0';
      p++; // skip closing quote
      // Store mapping with comprehensive memory safety
      if (mapping_count < MAX_MAPPINGS) {
        // **CRITICAL FIX**: Add bounds checking for key length
        if (klen >= sizeof(key)) {
          fprintf(stderr, "[scopemux] ERROR: Key too long (truncated): %.*s\n", (int)klen,
                  key_start);
          continue;
        }

        // **CRITICAL FIX**: Add bounds checking for value length
        if (vlen >= sizeof(val)) {
          fprintf(stderr, "[scopemux] ERROR: Value too long (truncated): %.*s\n", (int)vlen,
                  val_start);
          continue;
        }

        // **CRITICAL FIX**: Safe string duplication with error checking
        char *query_type_copy = strdup(key);
        if (!query_type_copy) {
          fprintf(stderr, "[scopemux] ERROR: Failed to allocate memory for query type: %s\n", key);
          continue;
        }

        // **CRITICAL FIX**: Validate the mapping before storing
        ASTNodeType node_type = parse_node_type(val);
        if (node_type == NODE_UNKNOWN && strcmp(val, "NODE_UNKNOWN") != 0) {
          fprintf(stderr, "[scopemux] WARNING: Unknown node type: %s, defaulting to NODE_UNKNOWN\n",
                  val);
        }

        // Store the mapping
        mappings[mapping_count].query_type = query_type_copy;
        mappings[mapping_count].node_type = node_type;
        printf("  %s -> %s\n", key, val);
        mapping_count++;
      } else {
        fprintf(stderr, "[scopemux] ERROR: Maximum mappings (%d) exceeded, ignoring: %s\n",
                MAX_MAPPINGS, key);
      }
    } else {
      p++;
    }
  }
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
  if (mapping_count < 0 || mapping_count > MAX_MAPPINGS) {
    fprintf(stderr, "[scopemux] ERROR: Corrupted mapping_count: %d (max: %d)\n", mapping_count,
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
