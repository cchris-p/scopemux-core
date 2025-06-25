#pragma once

#include "../../scopemux/parser.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Loads the node type mapping from a JSON config file.
 * @param config_path Path to the JSON config file.
 */
void load_node_type_mapping(const char *config_path);

/**
 * @brief Gets the ASTNodeType for a given query type string.
 * @param query_type The query type string (e.g., "functions").
 * @return The corresponding ASTNodeType, or NODE_UNKNOWN if not found.
 */
ASTNodeType get_node_type_for_query(const char *query_type);

/**
 * @brief Frees all memory used by the node type mapping.
 */
void free_node_type_mapping(void);

#ifdef __cplusplus
}
#endif
