/**
 * @file ast.c
 * @brief ASTNodeType mapping utilities for ScopeMux
 *
 * This file provides hardcoded mapping between ASTNodeType enum values and their string representations.
 * It replaces the need for a separate JSON mapping file.
 */

#include "scopemux/ast.h"
#include <string.h>

// Array of string representations for ASTNodeType
static const char* ASTNodeTypeNames[] = {
    "NODE_TYPE_UNKNOWN",
    "NODE_TYPE_ROOT",
    "NODE_TYPE_FUNCTION",
    "NODE_TYPE_CLASS",
    "NODE_TYPE_METHOD",
    "NODE_TYPE_VARIABLE",
    "NODE_TYPE_PARAMETER",
    "NODE_TYPE_IDENTIFIER",
    "NODE_TYPE_IMPORT",
    "NODE_TYPE_MODULE",
    "NODE_TYPE_STATEMENT",
    "NODE_TYPE_EXPRESSION",
    "NODE_TYPE_CALL",
    "NODE_TYPE_REFERENCE",
    "NODE_TYPE_NAMESPACE",
    "NODE_TYPE_STRUCT",
    "NODE_TYPE_ENUM",
    "NODE_TYPE_INTERFACE",
    "NODE_TYPE_CONTROL_FLOW",
    "NODE_TYPE_TEMPLATE_SPECIALIZATION",
    "NODE_TYPE_LAMBDA",
    "NODE_TYPE_USING",
    "NODE_TYPE_FRIEND",
    "NODE_TYPE_OPERATOR"
};

_Static_assert(sizeof(ASTNodeTypeNames)/sizeof(ASTNodeTypeNames[0]) == NODE_TYPE_COUNT, "ASTNodeTypeNames and ASTNodeType enum must match in length");

/**
 * @brief Convert ASTNodeType enum to string.
 * @param type The ASTNodeType value.
 * @return The string representation.
 */
const char* ast_node_type_to_string(ASTNodeType type) {
    if (type < 0 || type >= NODE_TYPE_COUNT) return "NODE_TYPE_UNKNOWN";
    return ASTNodeTypeNames[type];
}

/**
 * @brief Convert string to ASTNodeType enum.
 * @param str The string representation.
 * @return The ASTNodeType value, or NODE_TYPE_UNKNOWN if not found.
 */
ASTNodeType ast_node_type_from_string(const char* str) {
    for (int i = 0; i < NODE_TYPE_COUNT; ++i) {
        if (strcmp(str, ASTNodeTypeNames[i]) == 0) {
            return (ASTNodeType)i;
        }
    }
    return NODE_TYPE_UNKNOWN;
}
