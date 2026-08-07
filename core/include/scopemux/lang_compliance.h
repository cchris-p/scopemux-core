/**
 * @file lang_compliance.h
 * @brief Language-specific compliance registration
 *
 * This module declares the registration functions for language-specific
 * schema compliance and post-processing callbacks.
 */

#ifndef SCOPEMUX_LANG_COMPLIANCE_H
#define SCOPEMUX_LANG_COMPLIANCE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register Python language-specific callbacks
 *
 * This function registers the Python-specific schema compliance and
 * post-processing callbacks with the AST compliance system.
 */
void register_python_ast_compliance(void);

/**
 * @brief Register JavaScript language-specific callbacks
 *
 * This function registers the JavaScript-specific schema compliance and
 * post-processing callbacks with the AST compliance system.
 */
void register_javascript_ast_compliance(void);

/**
 * @brief Register TypeScript language-specific callbacks
 *
 * This function registers the TypeScript-specific schema compliance and
 * post-processing callbacks with the AST compliance system.
 */
void register_typescript_ast_compliance(void);

/**
 * @brief Register C language-specific callbacks
 *
 * This function registers the C-specific schema compliance and
 * post-processing callbacks with the AST compliance system.
 */
void register_c_compliance(void);

/**
 * @brief Register all language-specific compliance callbacks
 *
 * This function registers all available language-specific schema compliance
 * and post-processing callbacks with the AST compliance system.
 */
void register_all_language_compliance(void);

#ifdef __cplusplus
}
#endif

#endif /* SCOPEMUX_LANG_COMPLIANCE_H */
