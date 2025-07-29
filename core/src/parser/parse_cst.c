/**
 * @file parse_cst.c
 * @brief Standalone CST parser utility for ScopeMux
 * 
 * This utility parses a source file and outputs the Concrete Syntax Tree (CST)
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
#include "cst_node.h"

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

/**
 * Print CST node as JSON recursively
 */
static void print_cst_node_json(const CSTNode *node, int indent) {
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
    printf("\"type\": \"%s\"", node->type ? node->type : "unknown");
    
    // Print content if available
    if (node->content && strlen(node->content) > 0) {
        printf(",\n");
        for (int i = 0; i < indent + 1; i++) {
            printf("  ");
        }
        printf("\"content\": \"%s\"", node->content);
    }
    
    // Print children if available
    if (node->children_count > 0) {
        printf(",\n");
        for (int i = 0; i < indent + 1; i++) {
            printf("  ");
        }
        printf("\"children\": [\n");
        
        for (unsigned int i = 0; i < node->children_count; i++) {
            print_cst_node_json(node->children[i], indent + 2);
            if (i < node->children_count - 1) {
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
        fprintf(stderr, "Parses the source file and outputs CST as JSON\n");
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

    // Set CST mode and parse
    ctx->mode = PARSE_CST;
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

    // Get CST root and print as JSON
    const CSTNode *cst_root = parser_get_cst_root(ctx);
    if (!cst_root) {
        fprintf(stderr, "Error: No CST generated\n");
        parser_free(ctx);
        safe_free(content);
        return 1;
    }

    printf("{\n");
    printf("  \"file\": \"%s\",\n", filepath);
    printf("  \"language\": \"%s\",\n", language_to_string(lang));
    printf("  \"cst\": ");
    print_cst_node_json(cst_root, 1);
    printf("\n}\n");

    // Cleanup
    parser_free(ctx);
    safe_free(content);
    
    return 0;
}
