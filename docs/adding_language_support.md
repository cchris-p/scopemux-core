# Adding Support for New Languages in ScopeMux

This guide explains how to add support for a new programming language in ScopeMux, focusing on AST schema compliance and post-processing.

## Overview

ScopeMux's modular language compliance system allows adding support for new languages without modifying the core AST builder. Each language implements two key callbacks:

1. **Schema Compliance Callback**: Ensures language-specific nodes conform to expected schema patterns
2. **Post-Processing Callback**: Applies language-specific transformations to the AST after general processing

## Step 1: Create a Language Compliance Module

Create a new file named `<language>_ast_compliance.c` in the `/core/src/parser/lang/` directory.

```c
/**
 * @file <language>_ast_compliance.c
 * @brief <Language>-specific AST schema compliance and post-processing
 *
 * This file implements schema compliance and post-processing functions
 * specifically for the <Language> programming language.
 */

#include "../../../include/scopemux/ast_compliance.h"
#include "../../../include/scopemux/parser_types.h"
#include "../ast_node.h"
#include "../memory_tracking.h"
#include "../parser_internal.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Apply <Language>-specific schema compliance rules to an AST node
 * 
 * @param node The node to apply schema compliance to
 * @param ctx The parser context
 * @return true if successful, false otherwise
 */
static bool <language>_schema_compliance_callback(ASTNode *node, ParserContext *ctx) {
    if (!node) return false;
    
    // Handle language-specific node types and conversions
    if (node->type == NODE_UNKNOWN && node->name) {
        // Example: Convert language-specific constructs to generic node types
        if (strcmp(node->name, "<language_specific_node>") == 0) {
            node->type = NODE_FUNCTION;
            // Update other properties as needed
            return true;
        }
        
        // Add more language-specific conversions as needed
    }
    
    return true;
}

/**
 * @brief Apply <Language>-specific post-processing to an AST
 * 
 * @param ast_root The root of the AST to process
 * @param ctx The parser context
 * @return The processed AST root or NULL on error
 */
static ASTNode *<language>_post_process_callback(ASTNode *ast_root, ParserContext *ctx) {
    if (!ast_root || !ctx) return ast_root;
    
    // Implement language-specific AST transformations
    
    return ast_root;
}

/**
 * @brief Register <Language> compliance and post-processing callbacks
 */
void register_<language>_compliance(void) {
    register_schema_compliance_callback(LANGUAGE_<LANGUAGE>, <language>_schema_compliance_callback);
    register_ast_post_process_callback(LANGUAGE_<LANGUAGE>, <language>_post_process_callback);
    
    log_info("Registered <Language> language compliance callbacks");
}
```

## Step 2: Update Language Compliance Header

Add your language registration function to `core/include/scopemux/lang_compliance.h`:

```c
/**
 * @brief Register <Language> language-specific callbacks
 * 
 * This function registers the <Language>-specific schema compliance and
 * post-processing callbacks with the AST compliance system.
 */
void register_<language>_compliance(void);
```

## Step 3: Update Language Registration

Add your language to the `register_all_language_compliance` function in `core/src/parser/lang/lang_compliance.c`:

```c
void register_all_language_compliance(void) {
    log_debug("Registering all language-specific compliance callbacks");
    
    // Register callbacks for each supported language
    register_python_ast_compliance();
    register_javascript_ast_compliance();
    register_typescript_ast_compliance();
    register_c_compliance();
    register_<language>_compliance();  // Add your language here
    
    log_debug("All language-specific compliance callbacks registered");
}
```

## Step 4: Create Tree-sitter Queries

For proper AST generation, create language-specific queries in:

```
queries/<language>/functions.scm
queries/<language>/classes.scm
queries/<language>/variables.scm
```

See existing language query files for examples.

## Step 5: Define Language Enum

Ensure your language is added to the `Language` enum in the appropriate header file:

```c
typedef enum {
    LANG_UNKNOWN = 0,
    LANG_C,
    LANG_CPP,
    LANG_PYTHON,
    LANG_JAVASCRIPT,
    LANG_TYPESCRIPT,
    LANG_<LANGUAGE>,  // Add your language here
    // ...
} Language;
```

## Best Practices

1. **Error Handling**: Always include error checking in callbacks and return appropriate values
2. **Memory Management**: Be careful with memory allocation and freeing
3. **Node Types**: Use appropriate node types from the existing node type enum
4. **Logging**: Include descriptive logs at appropriate levels
5. **Backwards Compatibility**: Test with existing code to ensure compatibility

## Handling Mixed-Language Files

When dealing with mixed-language files (like JavaScript with embedded HTML), follow these guidelines:

1. The primary language's compliance module should handle the embedded language
2. Use node attributes to mark embedded language regions
3. Consider creating specialized mixed-language compliance modules for common combinations

## Testing

After implementation:

1. Run the test suite to verify basic functionality
2. Create language-specific test files with known structures
3. Compare generated ASTs with expected schema

## Common Challenges

- **Node Naming**: Ensure consistent naming conventions across languages
- **Schema Mapping**: Map language-specific constructs to canonical node types
- **Performance**: Optimize for large files with many nodes
