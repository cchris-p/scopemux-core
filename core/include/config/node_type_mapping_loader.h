#pragma once
#include "parser.h"

#ifdef __cplusplus
extern "C" {
#endif

void load_node_type_mapping(const char *config_path);
ASTNodeType get_node_type_for_query(const char *query_type);
void free_node_type_mapping();

#ifdef __cplusplus
}
#endif
