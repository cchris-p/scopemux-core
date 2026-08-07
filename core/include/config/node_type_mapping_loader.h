#ifndef NODE_TYPE_MAPPING_LOADER_H
#define NODE_TYPE_MAPPING_LOADER_H

#include "scopemux/parser.h"

// Load hardcoded node type mappings (no config file needed)
void load_node_type_mapping(void);

// Get the ASTNodeType for a given query type string
ASTNodeType get_node_type_for_query(const char *query_type);

// Free all memory used by the node type mapping
void free_node_type_mapping(void);

#endif // NODE_TYPE_MAPPING_LOADER_H
