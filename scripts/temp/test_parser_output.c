/**
 * @file test_parser_output.c
 * @brief Simple test to output AST for hello_world.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scopemux/parser.h"
#include "scopemux/language.h"
#include "scopemux/logging.h"
#include "scopemux/memory_management.h"
#include "scopemux/ast.h"

int main() {
    const char *filepath = "core/tests/examples/c/basic_syntax/hello_world.c";
    const char *content = "#include <stdio.h>\n\nint main() {\n    printf(\"Hello, world!\\n\");\n    return 0;\n}\n";
    
    printf("=== Testing AST output for hello_world.c ===\\n");
    
    // Initialize parser context
    ParserContext *ctx = parser_init();
    if (!ctx) {
        fprintf(stderr, "Error: Failed to initialize parser context\\n");
        return 1;
    }

    // Set AST mode and parse
    ctx->mode = PARSE_AST;
    bool success = parser_parse_string(ctx, content, strlen(content), "hello_world.c", LANG_C);
    if (!success) {
        fprintf(stderr, "Error: Failed to parse file\\n");
        const char *error = parser_get_last_error(ctx);
        if (error) {
            fprintf(stderr, "Parser error: %s\\n", error);
        }
        parser_free(ctx);
        return 1;
    }

    printf("Parse successful!\\n");
    
    // Get AST nodes by type
    ASTNodeType types[] = {NODE_FUNCTION, NODE_VARIABLE, NODE_STRUCT};
    size_t num_types = sizeof(types) / sizeof(types[0]);
    
    for (size_t t = 0; t < num_types; t++) {
        size_t count = parser_get_ast_nodes_by_type(ctx, types[t], NULL, 0);
        printf("Found %zu nodes of type %s\\n", count, ast_node_type_to_string(types[t]));
        
        if (count > 0) {
            const ASTNode **nodes = safe_malloc(sizeof(ASTNode*) * count);
            size_t retrieved = parser_get_ast_nodes_by_type(ctx, types[t], nodes, count);
            
            for (size_t i = 0; i < retrieved; i++) {
                const ASTNode *node = nodes[i];
                printf("  Node %zu: name='%s', range=(%u,%u)-(%u,%u)\\n", 
                       i, node->name ? node->name : "(null)",
                       node->range.start.line, node->range.start.column,
                       node->range.end.line, node->range.end.column);
            }
            
            safe_free(nodes);
        }
    }

    // Cleanup
    parser_free(ctx);
    
    return 0;
}
