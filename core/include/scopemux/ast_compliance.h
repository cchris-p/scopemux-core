/**
 * @file ast_compliance.h
 * @brief Schema compliance interface for language-specific AST adjustments
 *
 * This module defines the interfaces for language-specific schema compliance
 * and post-processing callbacks used by the AST builder.
 */

#ifndef SCOPEMUX_AST_COMPLIANCE_H
#define SCOPEMUX_AST_COMPLIANCE_H

#include "scopemux/ast.h"
#include "scopemux/parser.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Function signature for language-specific schema compliance
 *
 * This callback type is used for language-specific schema compliance
 * processing of AST nodes.
 *
 * @param node The AST node to process
 * @param ctx Parser context containing source information
 * @return int Status code (0 for success)
 */
typedef int (*SchemaComplianceCallback)(ASTNode *node, ParserContext *ctx);

/**
 * @brief Function signature for language-specific AST post-processing
 *
 * This callback type is used for language-specific post-processing
 * of the entire AST after it has been constructed.
 *
 * @param root_node The root AST node
 * @param ctx Parser context containing source information
 * @return ASTNode* The processed AST (may be different from input)
 */
typedef ASTNode *(*ASTPostProcessCallback)(ASTNode *root_node, ParserContext *ctx);

/**
 * @brief Register a schema compliance callback for a language
 *
 * @param language The language to register the callback for
 * @param callback The compliance function to register
 * @return int 0 on success, non-zero on failure
 */
int register_schema_compliance_callback(Language language, SchemaComplianceCallback callback);

/**
 * @brief Register a post-processing callback for a language
 *
 * @param language The language to register the callback for
 * @param callback The post-processing function to register
 * @return int 0 on success, non-zero on failure
 */
int register_ast_post_process_callback(Language language, ASTPostProcessCallback callback);

/**
 * @brief Get the schema compliance callback for a language
 *
 * @param language The language to get the callback for
 * @return SchemaComplianceCallback The registered callback or NULL if not found
 */
SchemaComplianceCallback get_schema_compliance_callback(Language language);

/**
 * @brief Get the post-processing callback for a language
 *
 * @param language The language to get the callback for
 * @return ASTPostProcessCallback The registered callback or NULL if not found
 */
ASTPostProcessCallback get_ast_post_process_callback(Language language);

#ifdef __cplusplus
}
#endif

#endif /* SCOPEMUX_AST_COMPLIANCE_H */
