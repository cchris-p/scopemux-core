/**
 * @file lang_compliance_registry.c
 * @brief Implementation of language-specific compliance registration
 *
 * This module implements the registration functions for language-specific
 * schema compliance and post-processing callbacks.
 */

#include "../../../include/scopemux/ast_compliance.h"
#include "../../../include/scopemux/lang_compliance.h"
#include "../../../include/scopemux/logging.h"
#include "../../../include/scopemux/parser.h"

// Forward declarations of language-specific compliance functions
extern void register_c_compliance(void);

/**
 * Register Python language-specific compliance callbacks
 * @note Implemented in python_ast_compliance.c
 */
extern void register_python_ast_compliance(void);

/**
 * Register JavaScript language-specific compliance callbacks
 * @note Implemented in javascript_ast_compliance.c
 */
extern void register_javascript_ast_compliance(void);

/**
 * Register TypeScript language-specific compliance callbacks
 * @note Implemented in typescript_ast_compliance.c
 */
extern void register_typescript_ast_compliance(void);

/**
 * Register all language-specific compliance callbacks
 *
 * This function registers all available language-specific schema compliance
 * and post-processing callbacks with the AST compliance system.
 */
void register_all_language_compliance(void) {
  log_debug("Registering all language compliance callbacks");

  // Register C language compliance
  register_c_compliance();

  // Register other language compliance as they are implemented
  register_python_ast_compliance();
  register_javascript_ast_compliance();
  register_typescript_ast_compliance();
}
