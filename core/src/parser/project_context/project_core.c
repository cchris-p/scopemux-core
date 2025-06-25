/**
 * @file project_core.c
 * @brief Core implementation of ProjectContext lifecycle and management
 */

#include "scopemux/project_context.h"
#include "scopemux/logging.h"
#include "scopemux/symbol_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations of functions implemented in other files
extern bool normalize_file_path(const char *project_root, const char *filepath, 
                              char *out_path, size_t out_size);
extern bool add_discovered_file(ProjectContext *project, const char *filepath);
extern bool is_file_parsed(const ProjectContext *project, const char *filepath);

/**
 * Create a new project context
 * 
 * Initializes a ProjectContext structure with the specified root directory.
 * Allocates memory for internal data structures and initializes them with
 * default values.
 * 
 * @param root_directory The root directory of the project
 * @return A pointer to the newly created ProjectContext, or NULL on failure
 */
ProjectContext *project_context_create(const char *root_directory) {
    if (!root_directory) {
        return NULL;
    }
    
    ProjectContext *project = (ProjectContext *)malloc(sizeof(ProjectContext));
    if (!project) {
        return NULL;
    }
    
    // Initialize all fields to safe values
    memset(project, 0, sizeof(ProjectContext));
    
    // Copy the root directory
    project->root_directory = strdup(root_directory);
    if (!project->root_directory) {
        free(project);
        return NULL;
    }
    
    // Initialize file_contexts array
    project->files_capacity = 16;  // Initial capacity
    project->file_contexts = (ParserContext **)malloc(
        project->files_capacity * sizeof(ParserContext *));
    if (!project->file_contexts) {
        free(project->root_directory);
        free(project);
        return NULL;
    }
    
    // Initialize discovered_files array
    project->discovered_capacity = 32;  // Initial capacity
    project->discovered_files = (char **)malloc(
        project->discovered_capacity * sizeof(char *));
    if (!project->discovered_files) {
        free(project->file_contexts);
        free(project->root_directory);
        free(project);
        return NULL;
    }
    
    // Create global symbol table with reasonable initial capacity
    project->symbol_table = symbol_table_create(256);
    if (!project->symbol_table) {
        free(project->discovered_files);
        free(project->file_contexts);
        free(project->root_directory);
        free(project);
        return NULL;
    }
    
    // Set default configuration
    project->config.parse_headers = true;
    project->config.follow_includes = true;
    project->config.resolve_external_symbols = false;
    project->config.max_files = 0;  // No limit
    project->config.max_include_depth = 10;
    project->config.log_level = LOG_INFO;
    
    return project;
}

/**
 * Free all resources associated with a project context
 * 
 * @param project The ProjectContext to free
 */
void project_context_free(ProjectContext *project) {
    if (!project) {
        return;
    }
    
    // Free each parser context
    for (size_t i = 0; i < project->num_files; i++) {
        if (project->file_contexts[i]) {
            parser_free(project->file_contexts[i]);
        }
    }
    free(project->file_contexts);
    
    // Free each discovered file path
    for (size_t i = 0; i < project->num_discovered; i++) {
        free(project->discovered_files[i]);
    }
    free(project->discovered_files);
    
    // Free the symbol table
    if (project->symbol_table) {
        symbol_table_free(project->symbol_table);
    }
    
    // Free other allocated strings
    free(project->root_directory);
    free(project->error_message);
    
    // Free the project itself
    free(project);
}

/**
 * Set project configuration options
 * 
 * @param project The ProjectContext to configure
 * @param config The configuration options to set
 */
void project_context_set_config(ProjectContext *project, const ProjectConfig *config) {
    if (!project || !config) {
        return;
    }
    
    memcpy(&project->config, config, sizeof(ProjectConfig));
}

/**
 * Set an error message in the project context
 * 
 * @param project The ProjectContext to set the error in
 * @param code The error code
 * @param message The error message
 */
void project_set_error(ProjectContext *project, int code, const char *message) {
    if (!project || !message) {
        return;
    }
    
    // Free any previous error message
    free(project->error_message);
    
    // Set the new error
    project->error_code = code;
    project->error_message = strdup(message);
}

/**
 * Get the last error message
 * 
 * @param project The ProjectContext to get the error from
 * @param out_code If not NULL, will be set to the error code
 * @return The error message, or NULL if there is no error
 */
const char *project_get_error(const ProjectContext *project, int *out_code) {
    if (!project) {
        if (out_code) {
            *out_code = -1;
        }
        return NULL;
    }
    
    if (out_code) {
        *out_code = project->error_code;
    }
    
    return project->error_message;
}

/**
 * Get project statistics
 * 
 * @param project The ProjectContext to get statistics from
 * @param out_total_files If not NULL, will be set to the total number of parsed files
 * @param out_total_symbols If not NULL, will be set to the total number of symbols
 * @param out_total_references If not NULL, will be set to the total number of references
 * @param out_unresolved If not NULL, will be set to the number of unresolved references
 */
void project_get_stats(const ProjectContext *project, size_t *out_total_files,
                      size_t *out_total_symbols, size_t *out_total_references,
                      size_t *out_unresolved) {
    if (!project) {
        if (out_total_files) *out_total_files = 0;
        if (out_total_symbols) *out_total_symbols = 0;
        if (out_total_references) *out_total_references = 0;
        if (out_unresolved) *out_unresolved = 0;
        return;
    }
    
    if (out_total_files) {
        *out_total_files = project->num_files;
    }
    
    // For other statistics, need to aggregate across all files
    size_t total_symbols = 0;
    size_t total_references = 0;
    size_t total_unresolved = 0;
    
    // Get statistics from the symbol table
    if (project->symbol_table) {
        symbol_table_get_stats(project->symbol_table, &total_symbols, NULL, NULL);
    }
    
    // Aggregate reference statistics from all files
    for (size_t i = 0; i < project->num_files; i++) {
        if (project->file_contexts[i]) {
            total_references += project->file_contexts[i]->stats.total_references;
            total_unresolved += project->file_contexts[i]->stats.unresolved_references;
        }
    }
    
    if (out_total_symbols) *out_total_symbols = total_symbols;
    if (out_total_references) *out_total_references = total_references;
    if (out_unresolved) *out_unresolved = total_unresolved;
}
