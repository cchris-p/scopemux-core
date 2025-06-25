/**
 * @file project_context.c
 * @brief Implementation of multi-file parsing and relationship management
 *
 * This is the main entry point for the ProjectContext functionality.
 * It delegates to specialized modules in the project_context/ directory
 * for different aspects of project management:
 *
 * - project_core.c: Core lifecycle management functions
 * - file_management.c: File tracking and discovery functions
 * - symbol_management.c: Symbol management and reference resolution
 * - dependency_management.c: Dependency tracking and include/import resolution
 */

#include "scopemux/project_context.h"
#include "scopemux/logging.h"
#include "scopemux/symbol_table.h"
#include "scopemux/reference_resolver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

// External functions defined in the specialized modules
// From project_core.c
extern ProjectContext *project_context_create(const char *root_directory);
extern void project_context_free(ProjectContext *project);
extern void project_context_set_config(ProjectContext *project, const ProjectConfig *config);
extern void project_set_error(ProjectContext *project, int code, const char *message);
extern const char *project_get_error(const ProjectContext *project, int *out_code);
extern void project_get_stats(const ProjectContext *project, size_t *out_total_files, 
                            size_t *out_total_symbols, size_t *out_total_references,
                            size_t *out_unresolved);

// From file_management.c
extern bool normalize_file_path(const char *project_root, const char *filepath, 
                             char *out_path, size_t out_size);
extern bool add_discovered_file(ProjectContext *project, const char *filepath);
extern bool is_file_parsed(const ProjectContext *project, const char *filepath);
extern bool project_add_file(ProjectContext *project, const char *filepath, LanguageType language);
extern size_t project_add_directory(ProjectContext *project, const char *dirpath, 
                                 const char **extensions, bool recursive);
extern ParserContext *project_get_file_context(const ProjectContext *project, const char *filepath);

// From symbol_management.c
extern void register_file_symbols(ProjectContext *project, ParserContext *ctx, const char *filepath);
extern ASTNode *project_get_symbol(const ProjectContext *project, const char *qualified_name);
extern size_t project_get_symbols_by_type(const ProjectContext *project, ASTNodeType type,
                                       const ASTNode **out_nodes, size_t max_nodes);
extern size_t project_find_references(const ProjectContext *project, const ASTNode *node,
                                   const ASTNode **out_references, size_t max_references);
extern bool project_resolve_references(ProjectContext *project);

// From dependency_management.c
extern void extract_and_process_includes(ProjectContext *project, ParserContext *ctx, const char *filepath);
extern bool project_parse_all_files(ProjectContext *project);

/**
 * Create a new project context
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
 */
void project_context_set_config(ProjectContext *project, const ProjectConfig *config) {
    if (!project || !config) {
        return;
    }
    
    memcpy(&project->config, config, sizeof(ProjectConfig));
}

/**
 * Add a file to the project for parsing
 */
bool project_add_file(ProjectContext *project, const char *filepath, LanguageType language) {
    if (!project || !filepath) {
        return false;
    }
    
    // Check for maximum files limit
    if (project->config.max_files > 0 && 
        project->num_files + project->num_discovered >= project->config.max_files) {
        project_set_error(project, 1, "Maximum file limit reached");
        return false;
    }
    
    // Normalize the file path
    char normalized_path[1024];
    if (!normalize_file_path(project->root_directory, filepath, normalized_path, sizeof(normalized_path))) {
        project_set_error(project, 2, "Failed to normalize file path");
        return false;
    }
    
    // Check if file is already parsed or discovered
    if (is_file_parsed(project, normalized_path)) {
        // Already in the project, not an error
        return true;
    }
    
    // Add to discovered files
    return add_discovered_file(project, normalized_path);
}

/**
 * Add all files in a directory to the project
 */
size_t project_add_directory(ProjectContext *project, const char *dirpath, 
                            const char **extensions, bool recursive) {
    if (!project || !dirpath) {
        return 0;
    }
    
    size_t files_added = 0;
    char normalized_dir[1024];
    
    // Normalize the directory path
    if (!normalize_file_path(project->root_directory, dirpath, normalized_dir, sizeof(normalized_dir))) {
        project_set_error(project, 6, "Failed to normalize directory path");
        return 0;
    }
    
    // Use dirent.h for directory traversal
    DIR *dir = opendir(normalized_dir);
    if (!dir) {
        char err_msg[256];
        snprintf(err_msg, sizeof(err_msg), "Failed to open directory: %s", normalized_dir);
        project_set_error(project, 7, err_msg);
        return 0;
    }
    
    // Read directory entries
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Construct full path
        char full_path[2048];
        snprintf(full_path, sizeof(full_path), "%s/%s", normalized_dir, entry->d_name);
        
        // Handle directories if recursive
        if (entry->d_type == DT_DIR && recursive) {
            files_added += project_add_directory(project, full_path, extensions, recursive);
            continue;
        }
        
        // Handle files
        if (entry->d_type == DT_REG) {
            // Check extension if specified
            bool extension_match = true;
            if (extensions) {
                extension_match = false;
                const char **ext = extensions;
                while (*ext) {
                    const char *file_ext = strrchr(entry->d_name, '.');
                    if (file_ext && strcmp(file_ext + 1, *ext) == 0) {
                        extension_match = true;
                        break;
                    }
                    ext++;
                }
            }
            
            // Add file if extension matches
            if (extension_match) {
                if (project_add_file(project, full_path, LANG_UNKNOWN)) {
                    files_added++;
                }
            }
        }
    }
    
    closedir(dir);
    return files_added;
}

/**
 * Parse all files in the project
 */
bool project_parse_all_files(ProjectContext *project) {
    if (!project) {
        return false;
    }
    
    // Process all discovered files
    while (project->num_discovered > 0) {
        // Pop a file from the discovered list
        char *filepath = project->discovered_files[--project->num_discovered];
        
        // Initialize a parser context
        ParserContext *ctx = parser_init();
        if (!ctx) {
            project_set_error(project, 3, "Failed to initialize parser context");
            free(filepath);
            return false;
        }
        
        // Set the parser logging level from project config
        ctx->log_level = project->config.log_level;
        
        // Parse the file
        if (!parser_parse_file(ctx, filepath, LANG_UNKNOWN)) {
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg), "Failed to parse file: %s", filepath);
            project_set_error(project, 4, error_msg);
            parser_free(ctx);
            free(filepath);
            return false;
        }
        
        // Add the parser context to the project
        if (project->num_files >= project->files_capacity) {
            size_t new_capacity = project->files_capacity * 2;
            ParserContext **new_contexts = (ParserContext **)realloc(
                project->file_contexts, new_capacity * sizeof(ParserContext *));
            if (!new_contexts) {
                project_set_error(project, 5, "Failed to resize file_contexts array");
                parser_free(ctx);
                free(filepath);
                return false;
            }
            project->file_contexts = new_contexts;
            project->files_capacity = new_capacity;
        }
        
        // Store the parser context
        project->file_contexts[project->num_files++] = ctx;
        
        // Register all symbols from this file
        register_file_symbols(project, ctx, filepath);
        
        // Free the file path string
        free(filepath);
        
        // Process includes/imports if enabled
        if (project->config.follow_includes) {
            // Extract and process includes/imports
            extract_and_process_includes(project, ctx, filepath);
        }
    }
    
    return true;
}

/**
 * Resolve references across all files in the project
 */
bool project_resolve_references(ProjectContext *project) {
    if (!project) {
        return false;
    }
    
    // Create a reference resolver
    ReferenceResolver *resolver = reference_resolver_create(project->symbol_table);
    if (!resolver) {
        project_set_error(project, 8, "Failed to create reference resolver");
        return false;
    }
    
    // Initialize built-in resolvers
    if (!reference_resolver_init_builtin(resolver)) {
        project_set_error(project, 9, "Failed to initialize built-in resolvers");
        reference_resolver_free(resolver);
        return false;
    }
    
    // Resolve all references
    size_t resolved = reference_resolver_resolve_all(resolver, project);
    
    // Update statistics
    project->unresolved_references = project->total_references - resolved;
    
    // Clean up
    reference_resolver_free(resolver);
    
    return resolved > 0; // Consider successful if at least one reference was resolved
}

/**
 * Get a file context by filename
 */
ParserContext *project_get_file_context(const ProjectContext *project, const char *filepath) {
    if (!project || !filepath) {
        return NULL;
    }
    
    char normalized_path[1024];
    if (!normalize_file_path(project->root_directory, filepath, normalized_path, sizeof(normalized_path))) {
        return NULL;
    }
    
    // Linear search for the file context
    // TODO: Consider using a hash table for faster lookup
    for (size_t i = 0; i < project->num_files; i++) {
        // Compare with the filepath stored in the context
        // Note: This assumes the filepath is stored in the context
        // which may need to be added in the future
        if (strcmp(project->file_contexts[i]->filepath, normalized_path) == 0) {
            return project->file_contexts[i];
        }
    }
    
    return NULL;
}

/**
 * Get a symbol by its qualified name from anywhere in the project
 */
const ASTNode *project_get_symbol(const ProjectContext *project, const char *qualified_name) {
    if (!project || !qualified_name || !project->symbol_table) {
        return NULL;
    }
    
    SymbolEntry *entry = symbol_table_lookup(project->symbol_table, qualified_name);
    return entry ? entry->node : NULL;
}

/**
 * Get all symbols of a specific type across the entire project
 */
size_t project_get_symbols_by_type(const ProjectContext *project, ASTNodeType type,
                                  const ASTNode **out_nodes, size_t max_nodes) {
    if (!project) {
        return 0;
    }
    
    size_t count = 0;
    
    // Temporary array for symbol entries
    SymbolEntry **entries = NULL;
    if (max_nodes > 0 && out_nodes) {
        entries = (SymbolEntry **)malloc(max_nodes * sizeof(SymbolEntry *));
        if (!entries) {
            return 0;
        }
    }
    
    // Get symbols from the table
    if (project->symbol_table) {
        count = symbol_table_get_by_type(project->symbol_table, type, entries, max_nodes);
    }
    
    // Copy nodes to output array
    if (entries && out_nodes) {
        for (size_t i = 0; i < count; i++) {
            out_nodes[i] = entries[i]->node;
        }
    }
    
    free(entries);
    return count;
}

/**
 * Find all references to a symbol across the project
 */
size_t project_find_references(const ProjectContext *project, const ASTNode *node,
                              const ASTNode **out_references, size_t max_references) {
    if (!project || !node) {
        return 0;
    }
    
    size_t count = 0;
    
    // Traverse all AST nodes in all files
    for (size_t i = 0; i < project->num_files && count < max_references; i++) {
        ParserContext *ctx = project->file_contexts[i];
        if (!ctx || !ctx->all_nodes) {
            continue;
        }
        
        // Check all nodes in this file
        for (size_t j = 0; j < ctx->num_nodes && count < max_references; j++) {
            ASTNode *current = ctx->all_nodes[j];
            if (!current || !current->references) {
                continue;
            }
            
            // Check all references in this node
            for (size_t k = 0; k < current->num_references && count < max_references; k++) {
                if (current->references[k] == node) {
                    // Found a reference to our target node
                    if (out_references) {
                        out_references[count] = current;
                    }
                    count++;
                }
            }
        }
    }
    
    return count;
}

/**
 * Get project statistics
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
    
    if (out_total_files) *out_total_files = project->num_files;
    if (out_total_symbols) *out_total_symbols = project->total_symbols;
    if (out_total_references) *out_total_references = project->total_references;
    if (out_unresolved) *out_unresolved = project->unresolved_references;
}

/**
 * Set an error message in the project context
 */
void project_set_error(ProjectContext *project, int code, const char *message) {
    if (!project || !message) {
        return;
    }
    
    // Free any existing error message
    free(project->error_message);
    
    // Store the new error
    project->error_code = code;
    project->error_message = strdup(message);
}

/**
 * Get the last error message
 */
const char *project_get_error(const ProjectContext *project, int *out_code) {
    if (!project) {
        if (out_code) *out_code = 0;
        return NULL;
    }
    
    if (out_code) *out_code = project->error_code;
    return project->error_message;
}

//-----------------------------------------------------------------------------
// Internal helper functions
//-----------------------------------------------------------------------------

/**
 * Normalize a file path relative to the project root
 */
static bool normalize_file_path(const char *project_root, const char *filepath, 
                              char *out_path, size_t out_size) {
    // Simple implementation - just prepend root if path is relative
    // TODO: Implement proper path normalization with ".." and "." handling
    
    if (!project_root || !filepath || !out_path || out_size == 0) {
        return false;
    }
    
    // Check if path is already absolute
    if (filepath[0] == '/') {
        strncpy(out_path, filepath, out_size - 1);
        out_path[out_size - 1] = '\0';
        return true;
    }
    
    // Prepend project root
    size_t root_len = strlen(project_root);
    if (root_len + 1 + strlen(filepath) >= out_size) {
        // Path too long
        return false;
    }
    
    strcpy(out_path, project_root);
    
    // Add separator if needed
    if (project_root[root_len - 1] != '/') {
        out_path[root_len] = '/';
        strcpy(out_path + root_len + 1, filepath);
    } else {
        strcpy(out_path + root_len, filepath);
    }
    
    return true;
}

/**
 * Add a file to the discovered files list
 */
static bool add_discovered_file(ProjectContext *project, const char *filepath) {
    if (!project || !filepath) {
        return false;
    }
    
    // Check if already in the discovered list
    for (size_t i = 0; i < project->num_discovered; i++) {
        if (strcmp(project->discovered_files[i], filepath) == 0) {
            return true;  // Already discovered
        }
    }
    
    // Resize if needed
    if (project->num_discovered >= project->discovered_capacity) {
        size_t new_capacity = project->discovered_capacity * 2;
        char **new_files = (char **)realloc(
            project->discovered_files, new_capacity * sizeof(char *));
        if (!new_files) {
            return false;
        }
        project->discovered_files = new_files;
        project->discovered_capacity = new_capacity;
    }
    
    // Add to the list
    project->discovered_files[project->num_discovered] = strdup(filepath);
    if (!project->discovered_files[project->num_discovered]) {
        return false;
    }
    project->num_discovered++;
    
    return true;
}

/**
 * Check if a file is already parsed
 */
static bool is_file_parsed(const ProjectContext *project, const char *filepath) {
    if (!project || !filepath) {
        return false;
    }
    
    // Check against all parsed files
    for (size_t i = 0; i < project->num_files; i++) {
        // Compare with the filepath stored in the context
        // Note: This assumes the filepath is stored in the context
        // which may need to be added in the future
        if (strcmp(project->file_contexts[i]->filename, filepath) == 0) {
            return true;
        }
    }
    
    return false;
}

/**
 * Register symbols from a parsed file into the global symbol table
 */
static void register_file_symbols(ProjectContext *project, ParserContext *ctx, const char *filepath) {
    if (!project || !ctx || !filepath || !project->symbol_table) {
        return;
    }
    
    // Skip if there are no nodes
    if (!ctx->all_nodes || ctx->num_nodes == 0) {
        return;
    }
    
    // Language detection
    LanguageType language = LANG_UNKNOWN;
    const char *extension = strrchr(filepath, '.');
    if (extension) {
        extension++; // Skip the dot
        if (strcasecmp(extension, "c") == 0) {
            language = LANG_C;
        } else if (strcasecmp(extension, "h") == 0) {
            language = LANG_C;
        } else if (strcasecmp(extension, "cpp") == 0 || 
                   strcasecmp(extension, "cc") == 0 ||
                   strcasecmp(extension, "hpp") == 0) {
            language = LANG_CPP;
        } else if (strcasecmp(extension, "py") == 0) {
            language = LANG_PYTHON;
        } else if (strcasecmp(extension, "js") == 0) {
            language = LANG_JAVASCRIPT;
        } else if (strcasecmp(extension, "ts") == 0) {
            language = LANG_TYPESCRIPT;
        }
    }
    
    // Register all nodes with qualified names
    for (size_t i = 0; i < ctx->num_nodes; i++) {
        ASTNode *node = ctx->all_nodes[i];
        if (!node || !node->qualified_name || strlen(node->qualified_name) == 0) {
            continue;
        }
        
        // Determine symbol scope
        SymbolScope scope = SCOPE_LOCAL;
        switch (node->type) {
            case NODE_FUNCTION:
            case NODE_CLASS:
            case NODE_MODULE:
            case NODE_STRUCT:
            case NODE_ENUM:
            case NODE_INTERFACE:
                scope = SCOPE_GLOBAL;
                break;
                
            case NODE_METHOD:
            case NODE_PROPERTY:
                scope = SCOPE_CLASS;
                break;
                
            default:
                scope = SCOPE_LOCAL;
                break;
        }
        
        // Register the symbol
        SymbolEntry *entry = symbol_table_register(
            project->symbol_table,
            node->qualified_name,
            node,
            filepath,
            scope,
            language);
        
        if (entry) {
            // Track the total number of registered symbols
            project->total_symbols++;
            
            // Register the scope for name resolution
            if (scope == SCOPE_GLOBAL && node->name) {
                // For global symbols, register their parent scope
                const char *parent_scope = NULL;
                if (node->parent && node->parent->qualified_name) {
                    parent_scope = node->parent->qualified_name;
                    symbol_table_add_scope(project->symbol_table, parent_scope);
                }
            }
        }
    }
}

/**
 * Extract includes/imports from a parsed file and add them to project
 */
static void extract_and_process_includes(ProjectContext *project, ParserContext *ctx, const char *filepath) {
    if (!project || !ctx || !filepath) {
        return;
    }
    
    // Skip if no nodes or reached max include depth
    if (!ctx->all_nodes || ctx->num_nodes == 0 || project->current_include_depth >= project->config.max_include_depth) {
        return;
    }
    
    // Detect file language
    LanguageType language = LANG_UNKNOWN;
    const char *extension = strrchr(filepath, '.');
    if (extension) {
        extension++; // Skip the dot
        if (strcasecmp(extension, "c") == 0 || strcasecmp(extension, "h") == 0) {
            language = LANG_C;
        } else if (strcasecmp(extension, "cpp") == 0 || 
                   strcasecmp(extension, "cc") == 0 ||
                   strcasecmp(extension, "hpp") == 0) {
            language = LANG_CPP;
        } else if (strcasecmp(extension, "py") == 0) {
            language = LANG_PYTHON;
        } else if (strcasecmp(extension, "js") == 0) {
            language = LANG_JAVASCRIPT;
        } else if (strcasecmp(extension, "ts") == 0) {
            language = LANG_TYPESCRIPT;
        }
    }
    
    // Find include/import nodes based on language
    for (size_t i = 0; i < ctx->num_nodes; i++) {
        ASTNode *node = ctx->all_nodes[i];
        if (!node) {
            continue;
        }
        
        char *include_path = NULL;
        bool is_system_include = false;
        
        // Language-specific include/import extraction
        switch (language) {
            case LANG_C:
            case LANG_CPP:
                // Look for nodes with type NODE_INCLUDE or check raw_content for #include
                if (node->type == NODE_INCLUDE && node->raw_content) {
                    // Parse out the include path
                    char *start = strchr(node->raw_content, '"');
                    if (start) {
                        // Local include with ""
                        start++;
                        char *end = strchr(start, '"');
                        if (end) {
                            size_t len = end - start;
                            include_path = malloc(len + 1);
                            if (include_path) {
                                strncpy(include_path, start, len);
                                include_path[len] = '\0';
                                is_system_include = false;
                            }
                        }
                    } else {
                        // System include with <>
                        start = strchr(node->raw_content, '<');
                        if (start) {
                            start++;
                            char *end = strchr(start, '>');
                            if (end) {
                                size_t len = end - start;
                                include_path = malloc(len + 1);
                                if (include_path) {
                                    strncpy(include_path, start, len);
                                    include_path[len] = '\0';
                                    is_system_include = true;
                                }
                            }
                        }
                    }
                }
                break;
                
            case LANG_PYTHON:
                // Look for import statements
                if ((node->type == NODE_IMPORT || node->type == NODE_MODULE_REF) && node->name) {
                    include_path = strdup(node->name);
                }
                break;
                
            case LANG_JAVASCRIPT:
            case LANG_TYPESCRIPT:
                // Look for import/require statements
                if ((node->type == NODE_IMPORT || node->type == NODE_MODULE_REF) && node->raw_content) {
                    // Parse import statements like: import X from 'path'
                    char *start = strstr(node->raw_content, "from");
                    if (start) {
                        start = strchr(start, '\'');
                        if (!start) {
                            start = strchr(node->raw_content, '"');
                        }
                        if (start) {
                            start++;
                            char *end = strchr(start, start[-1]); // matching quote
                            if (end) {
                                size_t len = end - start;
                                include_path = malloc(len + 1);
                                if (include_path) {
                                    strncpy(include_path, start, len);
                                    include_path[len] = '\0';
                                }
                            }
                        }
                    }
                }
                break;
                
            default:
                // Unknown language, can't extract includes
                break;
        }
        
        // Process the include if found
        if (include_path) {
            if (is_system_include && !project->config.parse_headers) {
                // Skip system includes if not configured to parse them
                free(include_path);
                continue;
            }
            
            // Determine the full path
            char full_path[1024];
            if (is_system_include) {
                // For system includes, try standard include paths
                // This is very simplified and should be expanded
                snprintf(full_path, sizeof(full_path), "/usr/include/%s", include_path);
            } else {
                // For local includes, resolve relative to current file
                char *dir_path = strdup(filepath);
                char *last_slash = strrchr(dir_path, '/');
                if (last_slash) {
                    *last_slash = '\0';
                    snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, include_path);
                } else {
                    // No directory part, use project root
                    snprintf(full_path, sizeof(full_path), "%s/%s", project->root_directory, include_path);
                }
                free(dir_path);
            }
            
            // Increase include depth
            project->current_include_depth++;
            
            // Add the file to the project for parsing
            project_add_file(project, full_path, language);
            
            // Restore include depth
            project->current_include_depth--;
            
            free(include_path);
        }
    }
}
