#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/include/scopemux/parser.h"

int main() {
    printf("Testing AST node double-free fix...\n");
    
    // Initialize parser context
    ParserContext *ctx = parser_init();
    if (!ctx) {
        printf("ERROR: Failed to initialize parser context\n");
        return 1;
    }
    
    // Simple C code to parse
    const char *source_code = "#include <stdio.h>\nint main() { return 0; }";
    
    // Parse the source code
    bool parse_success = parser_parse_string(ctx, source_code, strlen(source_code), "test.c", LANG_C);
    if (!parse_success) {
        printf("ERROR: Failed to parse source code\n");
        parser_free(ctx);
        return 1;
    }
    
    // Get AST nodes
    const ASTNode *ast_nodes[10];
    size_t node_count = parser_get_ast_nodes_by_type(ctx, NODE_FUNCTION, ast_nodes, 10);
    printf("Found %zu function nodes\n", node_count);
    
    // Free the parser context - this should not cause double-free anymore
    printf("Freeing parser context...\n");
    parser_free(ctx);
    printf("SUCCESS: Parser freed without double-free!\n");
    
    return 0;
}
