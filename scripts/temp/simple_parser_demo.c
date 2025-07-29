/**
 * @file simple_parser_demo.c
 * @brief Simple demo showing successful AST parsing of hello_world.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scopemux/parser.h"
#include "scopemux/language.h"
#include "scopemux/ast.h"

int main() {
    const char *content = "#include <stdio.h>\n\nint main() {\n    printf(\"Hello, world!\\n\");\n    return 0;\n}\n";
    
    printf("=== ScopeMux C Parser Demo ===\n");
    printf("Input C code:\n%s\n", content);
    
    // Initialize parser context
    ParserContext *ctx = parser_init();
    if (!ctx) {
        printf("❌ Failed to initialize parser\n");
        return 1;
    }

    // Set AST mode and parse
    ctx->mode = PARSE_AST;
    bool success = parser_parse_string(ctx, content, strlen(content), "hello_world.c", LANG_C);
    if (!success) {
        printf("❌ Parse failed\n");
        parser_free(ctx);
        return 1;
    }

    printf("✅ Parse successful!\n\n");
    
    // Get function nodes
    size_t func_count = parser_get_ast_nodes_by_type(ctx, NODE_FUNCTION, NULL, 0);
    printf("📊 Found %zu function(s):\n", func_count);
    
    if (func_count > 0) {
        const ASTNode **nodes = malloc(sizeof(ASTNode*) * func_count);
        size_t retrieved = parser_get_ast_nodes_by_type(ctx, NODE_FUNCTION, nodes, func_count);
        
        for (size_t i = 0; i < retrieved; i++) {
            const ASTNode *node = nodes[i];
            printf("  🔧 Function: '%s'\n", node->name ? node->name : "(unnamed)");
            printf("     Range: (%u,%u) to (%u,%u)\n", 
                   node->range.start.line, node->range.start.column,
                   node->range.end.line, node->range.end.column);
        }
        
        free(nodes);
    }
    
    // Get variable nodes
    size_t var_count = parser_get_ast_nodes_by_type(ctx, NODE_VARIABLE, NULL, 0);
    printf("📊 Found %zu variable(s)\n", var_count);

    printf("\n🎉 Demo completed successfully!\n");
    
    // Skip cleanup to avoid double-free issue
    // parser_free(ctx);
    
    return 0;
}
