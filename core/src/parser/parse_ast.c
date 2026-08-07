/**
 * @file parse_ast.c
 * @brief Standalone AST parser utility for ScopeMux
 * 
 * This utility parses a source file and outputs the Abstract Syntax Tree (AST)
 * as JSON to stdout. Language is automatically detected from file extension.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "scopemux/parser.h"
#include "scopemux/language.h"
#include "scopemux/logging.h"
#include "scopemux/memory_management.h"
#include "scopemux/ast.h"

/**
 * Read entire file contents into a string
 */
static char *read_file_contents(const char *filepath, size_t *out_size) {
    FILE *file = fopen(filepath, "rb");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file '%s': %s\n", filepath, strerror(errno));
        return NULL;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size < 0) {
        fprintf(stderr, "Error: Cannot determine file size for '%s'\n", filepath);
        fclose(file);
        return NULL;
    }

    // Allocate buffer and read file
    char *content = safe_malloc(file_size + 1);
    size_t bytes_read = fread(content, 1, file_size, file);
    content[bytes_read] = '\0';
    
    fclose(file);
    
    if (out_size) {
        *out_size = bytes_read;
    }
    
    return content;
}

// ast_node_type_to_string is already declared in ast.h, so we don't need to redefine it

/**
 * Print AST node as JSON recursively
 */
static void print_ast_node_json(const ASTNode *node, int indent) {
    if (!node) {
        printf("null");
        return;
    }

    // Print indentation
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }

    printf("{\n");
    
    // Print node type
    for (int i = 0; i < indent + 1; i++) {
        printf("  ");
    }
    printf("\"type\": \"%s\"", ast_node_type_to_string(node->type));
    
    // Print name if available
    if (node->name && strlen(node->name) > 0) {
        printf(",\n");
        for (int i = 0; i < indent + 1; i++) {
            printf("  ");
        }
        printf("\"name\": \"%s\"", node->name);
    }
    
    // Print qualified_name if available
    if (node->qualified_name && strlen(node->qualified_name) > 0) {
        printf(",\n");
        for (int i = 0; i < indent + 1; i++) {
            printf("  ");
        }
        printf("\"qualified_name\": \"%s\"", node->qualified_name);
    }
    
    // Print source range
    printf(",\n");
    for (int i = 0; i < indent + 1; i++) {
        printf("  ");
    }
    printf("\"range\": {");
    printf("\"start_line\": %u, ", node->range.start.line);
    printf("\"start_column\": %u, ", node->range.start.column);
    printf("\"end_line\": %u, ", node->range.end.line);
    printf("\"end_column\": %u", node->range.end.column);
    printf("}");
    
    // Print children if available
    if (node->num_children > 0) {
        printf(",\n");
        for (int i = 0; i < indent + 1; i++) {
            printf("  ");
        }
        printf("\"children\": [\n");
        
        for (size_t i = 0; i < node->num_children; i++) {
            print_ast_node_json(node->children[i], indent + 2);
            if (i < node->num_children - 1) {
                printf(",");
            }
            printf("\n");
        }
        
        for (int i = 0; i < indent + 1; i++) {
            printf("  ");
        }
        printf("]");
    }
    
    printf("\n");
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
    printf("}");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <source_file>\n", argv[0]);
        fprintf(stderr, "Parses the source file and outputs AST as JSON\n");
        return 1;
    }

    const char *filepath = argv[1];
    
    // Detect language from file extension
    Language lang = language_detect_from_extension(filepath);
    if (lang == LANG_UNKNOWN) {
        fprintf(stderr, "Error: Cannot detect language from file extension: %s\n", filepath);
        return 1;
    }

    // Read file contents
    size_t content_size;
    char *content = read_file_contents(filepath, &content_size);
    if (!content) {
        return 1;
    }

    // Initialize parser context
    ParserContext *ctx = parser_init();
    if (!ctx) {
        fprintf(stderr, "Error: Failed to initialize parser context\n");
        safe_free(content);
        return 1;
    }

    // Set AST mode and parse
    ctx->mode = PARSE_AST;
    bool success = parser_parse_string(ctx, content, content_size, filepath, lang);
    if (!success) {
        fprintf(stderr, "Error: Failed to parse file '%s'\n", filepath);
        const char *error = parser_get_last_error(ctx);
        if (error) {
            fprintf(stderr, "Parser error: %s\n", error);
        }
        parser_free(ctx);
        safe_free(content);
        return 1;
    }

    // Get AST nodes and print as JSON
    printf("{\n");
    printf("  \"file\": \"%s\",\n", filepath);
    printf("  \"language\": \"%s\",\n", language_to_string(lang));
    printf("  \"ast_nodes\": [\n");
    
    // Get nodes by type - iterate through all types
    ASTNodeType types[] = {NODE_FUNCTION, NODE_CLASS, NODE_METHOD, NODE_VARIABLE, 
                          NODE_MODULE, NODE_STRUCT, NODE_UNION, NODE_ENUM, 
                          NODE_TYPEDEF, NODE_INCLUDE, NODE_MACRO, NODE_DOCSTRING};
    size_t num_types = sizeof(types) / sizeof(types[0]);
    bool first_node = true;
    
    for (size_t t = 0; t < num_types; t++) {
        // Get count first
        size_t count = parser_get_ast_nodes_by_type(ctx, types[t], NULL, 0);
        if (count > 0) {
            const ASTNode **nodes = safe_malloc(sizeof(ASTNode*) * count);
            size_t retrieved = parser_get_ast_nodes_by_type(ctx, types[t], nodes, count);
            
            for (size_t i = 0; i < retrieved; i++) {
                if (!first_node) {
                    printf(",\n");
                }
                print_ast_node_json(nodes[i], 2);
                first_node = false;
            }
            
            safe_free(nodes);
        }
    }
    
    if (!first_node) {
        printf("\n");
    }
    
    printf("  ]\n");
    printf("}\n");

    // Cleanup
    parser_free(ctx);
    safe_free(content);
    
    return 0;
}
