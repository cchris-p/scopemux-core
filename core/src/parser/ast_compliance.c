/**
 * @file ast_compliance.c
 * @brief Implementation of the schema compliance interface
 *
 * This module implements the registry for language-specific schema compliance
 * and post-processing callbacks used by the AST builder.
 */

#include "scopemux/ast_compliance.h"
#include "scopemux/logging.h"

#include <stdlib.h>
#include <string.h>

// Maximum number of supported languages
#define MAX_LANGUAGES 16

// Registry for schema compliance callbacks
static struct {
  Language language;
  SchemaComplianceCallback callback;
} compliance_registry[MAX_LANGUAGES] = {0};

// Registry for post-processing callbacks
static struct {
  Language language;
  ASTPostProcessCallback callback;
} post_process_registry[MAX_LANGUAGES] = {0};

// Track the number of registered callbacks
static int num_compliance_callbacks = 0;
static int num_post_process_callbacks = 0;

int register_schema_compliance_callback(Language language, SchemaComplianceCallback callback) {
  // Validate parameters
  if (callback == NULL) {
    log_error("Attempted to register NULL schema compliance callback");
    return -1;
  }

  if (language < 0) {
    log_error("Invalid language ID: %d", language);
    return -1;
  }

  // Check for duplicates
  for (int i = 0; i < num_compliance_callbacks; i++) {
    if (compliance_registry[i].language == language) {
      log_warning("Overwriting existing schema compliance callback for language %d", language);
      compliance_registry[i].callback = callback;
      return 0;
    }
  }

  // Check if we have room for a new callback
  if (num_compliance_callbacks >= MAX_LANGUAGES) {
    log_error("Schema compliance registry is full, cannot register callback for language %d",
              language);
    return -1;
  }

  // Add the new callback
  compliance_registry[num_compliance_callbacks].language = language;
  compliance_registry[num_compliance_callbacks].callback = callback;
  num_compliance_callbacks++;

  log_debug("Registered schema compliance callback for language %d", language);
  return 0;
}

int register_ast_post_process_callback(Language language, ASTPostProcessCallback callback) {
  // Validate parameters
  if (callback == NULL) {
    log_error("Attempted to register NULL AST post-process callback");
    return -1;
  }

  if (language < 0) {
    log_error("Invalid language ID: %d", language);
    return -1;
  }

  // Check for duplicates
  for (int i = 0; i < num_post_process_callbacks; i++) {
    if (post_process_registry[i].language == language) {
      log_warning("Overwriting existing AST post-process callback for language %d", language);
      post_process_registry[i].callback = callback;
      return 0;
    }
  }

  // Check if we have room for a new callback
  if (num_post_process_callbacks >= MAX_LANGUAGES) {
    log_error("AST post-process registry is full, cannot register callback for language %d",
              language);
    return -1;
  }

  // Add the new callback
  post_process_registry[num_post_process_callbacks].language = language;
  post_process_registry[num_post_process_callbacks].callback = callback;
  num_post_process_callbacks++;

  log_debug("Registered AST post-process callback for language %d", language);
  return 0;
}

SchemaComplianceCallback get_schema_compliance_callback(Language language) {
  if (language < 0) {
    log_error("Invalid language ID: %d", language);
    return NULL;
  }

  for (int i = 0; i < num_compliance_callbacks; i++) {
    if (compliance_registry[i].language == language) {
      log_debug("Found schema compliance callback for language %d", language);
      return compliance_registry[i].callback;
    }
  }

  // No callback registered for this language
  log_debug("No schema compliance callback registered for language %d", language);
  return NULL;
}

ASTPostProcessCallback get_ast_post_process_callback(Language language) {
  if (language < 0) {
    log_error("Invalid language ID: %d", language);
    return NULL;
  }

  for (int i = 0; i < num_post_process_callbacks; i++) {
    if (post_process_registry[i].language == language) {
      log_debug("Found post-process callback for language %d", language);
      return post_process_registry[i].callback;
    }
  }

  // No callback registered for this language
  log_debug("No post-process callback registered for language %d", language);
  return NULL;
}
