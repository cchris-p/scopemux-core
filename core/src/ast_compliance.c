/**
 * @file ast_compliance.c
 * @brief Implementation of schema compliance interface for language-specific AST adjustments
 *
 * This module implements the interfaces for language-specific schema compliance
 * and post-processing callbacks used by the AST builder.
 */

#include "../include/scopemux/ast_compliance.h"
#include "../include/scopemux/logging.h"
#include <stdlib.h>
#include <string.h>

// Array size for language callbacks (must be larger than the highest language enum value)
#define MAX_LANGUAGES 10

// Arrays to store callbacks for each language
static SchemaComplianceCallback schema_compliance_callbacks[MAX_LANGUAGES] = {0};
static ASTPostProcessCallback post_process_callbacks[MAX_LANGUAGES] = {0};

int register_schema_compliance_callback(Language language, SchemaComplianceCallback callback) {
  if (language < 0 || language >= MAX_LANGUAGES) {
    log_error("Invalid language ID for schema compliance callback registration: %d", language);
    return -1;
  }

  schema_compliance_callbacks[language] = callback;
  log_debug("Registered schema compliance callback for language %d", language);
  return 0;
}

int register_ast_post_process_callback(Language language, ASTPostProcessCallback callback) {
  if (language < 0 || language >= MAX_LANGUAGES) {
    log_error("Invalid language ID for AST post-process callback registration: %d", language);
    return -1;
  }

  post_process_callbacks[language] = callback;
  log_debug("Registered AST post-process callback for language %d", language);
  return 0;
}

SchemaComplianceCallback get_schema_compliance_callback(Language language) {
  if (language < 0 || language >= MAX_LANGUAGES) {
    log_error("Invalid language ID for schema compliance callback lookup: %d", language);
    return NULL;
  }

  return schema_compliance_callbacks[language];
}

ASTPostProcessCallback get_ast_post_process_callback(Language language) {
  if (language < 0 || language >= MAX_LANGUAGES) {
    log_error("Invalid language ID for AST post-process callback lookup: %d", language);
    return NULL;
  }

  return post_process_callbacks[language];
}
