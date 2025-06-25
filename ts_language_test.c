#include <stdio.h>

// Include the necessary Tree-sitter headers
#include "vendor/tree-sitter/lib/include/tree_sitter/api.h"
#include "vendor/tree-sitter-cpp/bindings/c/tree-sitter-cpp.h"
#include "vendor/tree-sitter-c/bindings/c/tree_sitter/tree-sitter-c.h"

int main() {
    // Print addresses of language functions
    printf("Address of tree_sitter_cpp: %p\n", (void*)&tree_sitter_cpp);
    printf("Address of tree_sitter_c: %p\n", (void*)&tree_sitter_c);
    
    // Call the language functions and print results
    const TSLanguage *cpp_lang = tree_sitter_cpp();
    const TSLanguage *c_lang = tree_sitter_c();
    
    printf("Result of tree_sitter_cpp(): %p\n", (void*)cpp_lang);
    printf("Result of tree_sitter_c(): %p\n", (void*)c_lang);
    
    // Try to create parser and set language
    if (cpp_lang) {
        TSParser *parser = ts_parser_new();
        if (parser) {
            bool success = ts_parser_set_language(parser, cpp_lang);
            printf("ts_parser_set_language result: %s\n", success ? "SUCCESS" : "FAILURE");
            
            const TSLanguage *set_lang = ts_parser_language(parser);
            printf("ts_parser_language result: %p (should match cpp_lang)\n", (void*)set_lang);
            
            ts_parser_delete(parser);
        } else {
            printf("Failed to create parser\n");
        }
    }
    
    return 0;
}
