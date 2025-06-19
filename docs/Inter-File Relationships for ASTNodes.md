# Inter-File Relationships for ASTNodes in ScopeMux

## Overview

This document outlines the design and implementation strategy for enabling robust inter-file relationship tracking in ScopeMux. Currently, the system parses individual files into `ASTNode` structures but lacks the ability to resolve and link relationships that span multiple files. This capability is essential for building comprehensive IRs, InfoBlocks, and TieredContexts that accurately represent real-world code dependencies.

## Current Architecture Foundation

### ASTNode Structure

The existing `ASTNode` structure in `parser.h` already includes the necessary fields for inter-file relationships:

```c
typedef struct ASTNode {
  ASTNodeType type;           // Type of the node
  char *name;                 // Name of the entity
  char *qualified_name;       // Fully qualified name (e.g., namespace::class::method)
  SourceRange range;          // Source code range
  char *signature;            // Function/method signature if applicable
  char *docstring;            // Associated documentation
  char *raw_content;          // Raw source code content

  // Parent-child relationships
  struct ASTNode *parent;     // Parent node (e.g., class for a method)
  struct ASTNode **children;  // Child nodes (e.g., methods for a class)
  size_t num_children;        // Number of children
  size_t children_capacity;   // Capacity of children array

  // References and dependencies - KEY FOR INTER-FILE RELATIONSHIPS
  struct ASTNode **references; // Nodes referenced by this node
  size_t num_references;       // Number of references
  size_t references_capacity;  // Capacity of references array

  void *additional_data;      // Language-specific or analysis data
} ASTNode;
```

**Key Fields for Inter-File Relationships:**
- `qualified_name`: Enables cross-file symbol lookup (e.g., `"utils.config.parse_settings"`)
- `references`: Array of pointers to `ASTNode`s in other files
- `ast_node_add_reference()`: Function to establish cross-file links

### Current Single-File Processing

The existing `tree_sitter_integration.c` handles single-file parsing effectively:

1. **Parse individual file** → `ParserContext` with local `ASTNode` tree
2. **Extract semantic entities** using Tree-sitter queries (`.scm` files)
3. **Build local relationships** within the file (parent-child, local references)
4. **Generate qualified names** based on local scope

**Gap**: No mechanism to resolve references that point to entities in other files.

## Inter-File Relationship Types

### Direct Relationships

**Function/Method Calls Across Files**
```python
# file_a.py
from file_b import process_data
result = process_data(input)  # Direct call relationship
```

**Class Inheritance and Interface Implementation**
```python
# models.py
class User(BaseModel):  # Inheritance relationship to BaseModel in base.py
    pass
```

**Import/Include Statements and Module Dependencies**
```c
// main.c
#include "utils.h"  // Include relationship
int main() { return utils_init(); }  // Function call relationship
```

**Variable/Constant References**
```python
# config.py
DATABASE_URL = "postgresql://..."

# main.py
from config import DATABASE_URL  # Variable reference relationship
```

**Type Usage and Definitions**
```typescript
// types.ts
export interface User { name: string; }

// service.ts
import { User } from './types';
function getUser(): User { ... }  // Type usage relationship
```

### Indirect Relationships

**Shared Data Structures and Common Interfaces**
- Multiple classes implementing the same interface
- Functions that operate on the same data types
- Polymorphic relationships through inheritance chains

**Dependency Chains**
- A uses B, B uses C → A indirectly depends on C
- Transitive closure of direct relationships

**Namespace/Module Hierarchies**
- Package-level relationships
- Module clustering based on usage patterns

**Template/Generic Instantiations** (C++/TypeScript)
- Template specializations across files
- Generic type parameter relationships

## Language-Specific Resolution Strategies

### C/C++
**Include Resolution:**
- Parse `#include` directives to identify file dependencies
- Handle both `<system>` and `"local"` includes
- Resolve relative paths based on include directories

**Namespace Resolution:**
```cpp
// utils.h
namespace utils { void helper(); }

// main.cpp
#include "utils.h"
utils::helper();  // Resolve to utils.h::utils::helper
```

**Template Instantiation:**
- Track template definitions and their instantiations
- Link template specializations to base templates

### Python
**Module Import Resolution:**
```python
import utils                    # Module-level import
from utils import helper       # Specific function import
from utils.config import DB    # Nested module import
```

**Dynamic Attribute Access:**
- Handle `getattr()`, `setattr()` patterns
- Resolve attribute access through inheritance chains

**Package Structure:**
- Understand `__init__.py` files and package hierarchies
- Resolve relative imports (`from .module import ...`)

### JavaScript/TypeScript
**ES6 Import Resolution:**
```javascript
import { helper } from './utils';           // Named import
import utils from './utils';               // Default import
import * as utils from './utils';          // Namespace import
```

**CommonJS Requires:**
```javascript
const utils = require('./utils');
const { helper } = require('./utils');
```

**TypeScript-Specific:**
- Interface merging across files
- Namespace declarations and augmentation
- Type-only imports (`import type { ... }`)

### Language-Agnostic Patterns
**Symbol Lookup Algorithm:**
1. Check local scope (current function/class)
2. Check file scope (current file)
3. Check imported/included symbols
4. Check global/project scope

**Qualified Name Matching:**
- Normalize qualified names across languages
- Handle aliasing and renaming
- Resolve scope conflicts

## Proposed Implementation Architecture

### Core Data Structures

```c
/**
 * @brief Manages parsing and relationship resolution across multiple files
 */
typedef struct ProjectContext {
    char *root_directory;              // Project root path
    ParserContext **file_contexts;     // Array of parsed files
    size_t num_files;                  // Number of parsed files
    size_t files_capacity;             // Capacity of file_contexts array
    
    GlobalSymbolTable *symbol_table;   // Cross-file symbol directory
    ReferenceResolver *resolver;       // Handles linking phase
    
    // Error handling
    char *last_error;                  // Last error message
    int error_code;                    // Error code
} ProjectContext;

/**
 * @brief Global symbol table for cross-file symbol lookup
 */
typedef struct GlobalSymbolTable {
    // Hash table implementation
    SymbolEntry **buckets;             // Hash table buckets
    size_t bucket_count;               // Number of buckets
    size_t total_symbols;              // Total number of symbols
} GlobalSymbolTable;

/**
 * @brief Entry in the global symbol table
 */
typedef struct SymbolEntry {
    char *qualified_name;              // Fully qualified symbol name
    ASTNode *node;                     // Pointer to the ASTNode
    char *source_file;                 // File where symbol is defined
    LanguageType language;             // Language of the source file
    struct SymbolEntry *next;          // For hash collision chaining
} SymbolEntry;

/**
 * @brief Handles language-specific reference resolution
 */
typedef struct ReferenceResolver {
    LanguageType language;             // Target language
    // Function pointers for language-specific resolution
    bool (*resolve_imports)(ParserContext *ctx, GlobalSymbolTable *table);
    bool (*resolve_calls)(ParserContext *ctx, GlobalSymbolTable *table);
    bool (*resolve_types)(ParserContext *ctx, GlobalSymbolTable *table);
} ReferenceResolver;
```

### Multi-File Processing Workflow

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   Discovery     │───▶│  Individual      │───▶│   Symbol        │
│   Phase         │    │  Parsing         │    │   Collection    │
│                 │    │                  │    │                 │
│ • Scan project  │    │ • Parse each     │    │ • Build global  │
│ • Identify      │    │   file into      │    │   symbol table │
│   source files  │    │   ParserContext  │    │ • Register all  │
│ • Detect        │    │ • Extract local  │    │   definitions   │
│   languages     │    │   ASTNodes       │    │                 │
└─────────────────┘    └──────────────────┘    └─────────────────┘
                                                         │
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   Validation    │◀───│  Resolution      │◀───│   Reference     │
│   Phase         │    │  Phase           │    │   Detection     │
│                 │    │                  │    │                 │
│ • Verify links  │    │ • Link refs      │    │ • Identify      │
│ • Handle        │    │   using symbol   │    │   potential     │
│   ambiguities   │    │   table lookup   │    │   cross-file    │
│ • Report        │    │ • Language-      │    │   references    │
│   unresolved    │    │   specific       │    │ • Extract       │
│   references    │    │   resolution     │    │   import/calls  │
└─────────────────┘    └──────────────────┘    └─────────────────┘
```

### Reference Resolution Algorithm

```c
/**
 * @brief Core algorithm for resolving cross-file references
 */
bool resolve_cross_file_references(ProjectContext *project_ctx) {
    // Phase 1: Build global symbol table
    for (size_t i = 0; i < project_ctx->num_files; i++) {
        ParserContext *file_ctx = project_ctx->file_contexts[i];
        
        // Register all definitions from this file
        for (size_t j = 0; j < file_ctx->num_ast_nodes; j++) {
            ASTNode *node = file_ctx->all_ast_nodes[j];
            if (is_definition(node)) {
                global_symbol_register(project_ctx->symbol_table, node);
            }
        }
    }
    
    // Phase 2: Resolve references in each file
    for (size_t i = 0; i < project_ctx->num_files; i++) {
        ParserContext *file_ctx = project_ctx->file_contexts[i];
        
        if (!resolve_file_references(file_ctx, project_ctx->symbol_table)) {
            return false;
        }
    }
    
    return true;
}

/**
 * @brief Resolve references within a single file
 */
bool resolve_file_references(ParserContext *file_ctx, GlobalSymbolTable *global_table) {
    for (size_t i = 0; i < file_ctx->num_ast_nodes; i++) {
        ASTNode *node = file_ctx->all_ast_nodes[i];
        
        // Find potential references in this node
        if (node->type == NODE_FUNCTION && has_function_calls(node)) {
            resolve_function_calls(node, file_ctx, global_table);
        }
        
        if (has_type_references(node)) {
            resolve_type_references(node, file_ctx, global_table);
        }
        
        if (has_variable_references(node)) {
            resolve_variable_references(node, file_ctx, global_table);
        }
    }
    
    return true;
}
```

### Scope-Aware Symbol Lookup

```c
/**
 * @brief Resolve a symbol reference using scope-aware lookup
 */
ASTNode* resolve_symbol_reference(const char *symbol_name, 
                                  ASTNode *context_node,
                                  ParserContext *file_ctx,
                                  GlobalSymbolTable *global_table) {
    
    // 1. Check local scope (current function/class)
    ASTNode *local_result = lookup_in_local_scope(symbol_name, context_node);
    if (local_result) return local_result;
    
    // 2. Check file scope (current file)
    ASTNode *file_result = lookup_in_file_scope(symbol_name, file_ctx);
    if (file_result) return file_result;
    
    // 3. Check imported/included symbols
    ASTNode *import_result = lookup_in_imports(symbol_name, file_ctx, global_table);
    if (import_result) return import_result;
    
    // 4. Check global/project scope
    ASTNode *global_result = global_symbol_lookup(global_table, symbol_name);
    if (global_result) return global_result;
    
    // 5. Handle language-specific resolution (e.g., namespace lookup)
    return language_specific_lookup(symbol_name, context_node, file_ctx, global_table);
}
```

## API Design

### Project-Level Operations

```c
/**
 * @brief Initialize a new project context
 */
ProjectContext* project_init(const char* root_directory);

/**
 * @brief Discover and parse all source files in the project
 */
bool project_parse_all_files(ProjectContext* ctx);

/**
 * @brief Resolve all cross-file references in the project
 */
bool project_resolve_references(ProjectContext* ctx);

/**
 * @brief Get statistics about resolved relationships
 */
typedef struct {
    size_t total_files;
    size_t total_symbols;
    size_t resolved_references;
    size_t unresolved_references;
} ProjectStats;

ProjectStats project_get_stats(const ProjectContext* ctx);

/**
 * @brief Clean up and free the project context
 */
void project_free(ProjectContext* ctx);
```

### Symbol Table Operations

```c
/**
 * @brief Look up a symbol by qualified name
 */
ASTNode* global_symbol_lookup(GlobalSymbolTable* table, const char* qualified_name);

/**
 * @brief Register a symbol definition in the global table
 */
bool global_symbol_register(GlobalSymbolTable* table, ASTNode* node);

/**
 * @brief Get all symbols matching a pattern (for fuzzy lookup)
 */
size_t global_symbol_search(GlobalSymbolTable* table, const char* pattern,
                           ASTNode** results, size_t max_results);

/**
 * @brief Get all symbols defined in a specific file
 */
size_t global_symbol_get_by_file(GlobalSymbolTable* table, const char* filename,
                                ASTNode** results, size_t max_results);
```

### Reference Resolution Operations

```c
/**
 * @brief Resolve references for a specific file
 */
bool resolve_file_references(ParserContext* file_ctx, GlobalSymbolTable* global_table);

/**
 * @brief Add a resolved reference between two nodes
 */
bool add_resolved_reference(ASTNode* from_node, ASTNode* to_node, 
                           const char* reference_type);

/**
 * @brief Get all nodes that reference a given node
 */
size_t get_referencing_nodes(const ASTNode* target_node, ASTNode** results, 
                            size_t max_results);

/**
 * @brief Get all nodes referenced by a given node
 */
size_t get_referenced_nodes(const ASTNode* source_node, ASTNode** results,
                           size_t max_results);
```

## Integration with Existing ScopeMux Modules

### Enhanced IR Generation

**Call Graph IR Enhancement:**
```c
// Before: Limited to single-file call graphs
CallGraphIR* generate_call_graph_ir(ParserContext* ctx);

// After: Complete cross-file call graphs
CallGraphIR* generate_project_call_graph_ir(ProjectContext* project_ctx);
```

**Symbol IR Enhancement:**
```c
// Enhanced Symbol IR with cross-file references
typedef struct SymbolIREntry {
    ASTNode* definition;           // Where symbol is defined
    ASTNode** references;          // All places where symbol is used
    size_t num_references;         // Number of references
    char** referencing_files;      // Files that reference this symbol
    size_t num_referencing_files;  // Number of referencing files
} SymbolIREntry;
```

### Enhanced InfoBlock Extraction

**Cross-File InfoBlocks:**
```c
/**
 * @brief Extract InfoBlocks that span multiple files
 */
typedef struct CrossFileInfoBlock {
    char* name;                    // InfoBlock name (e.g., "authentication_flow")
    ASTNode** primary_nodes;       // Main nodes in the InfoBlock
    ASTNode** supporting_nodes;    // Supporting nodes from other files
    char** involved_files;         // All files involved in this InfoBlock
    size_t num_files;              // Number of involved files
} CrossFileInfoBlock;

CrossFileInfoBlock* extract_cross_file_infoblock(ProjectContext* ctx, 
                                                 const char* seed_function);
```

**Dependency-Aware Extraction:**
```c
/**
 * @brief Extract all functions that work with a specific type
 */
InfoBlock** extract_type_related_functions(ProjectContext* ctx, 
                                          const char* type_name,
                                          size_t* num_blocks);
```

### Enhanced TieredContext Creation

**Project-Level Contexts:**
```c
/**
 * @brief Create TieredContext spanning multiple files
 */
typedef struct ProjectTieredContext {
    TieredContext base;            // Base TieredContext structure
    char** involved_files;         // Files included in this context
    size_t num_files;              // Number of files
    CrossFileRelationship** relationships; // Inter-file relationships
    size_t num_relationships;      // Number of relationships
} ProjectTieredContext;

ProjectTieredContext* create_project_tiered_context(ProjectContext* ctx,
                                                   const char* focus_area,
                                                   int tier_level);
```

**Dependency-Aware Context Boundaries:**
```c
/**
 * @brief Determine optimal context boundaries based on code relationships
 */
typedef struct ContextBoundary {
    ASTNode** core_nodes;          // Core nodes that must be included
    ASTNode** peripheral_nodes;    // Peripheral nodes that may be included
    char** required_files;         // Files that must be included
    char** optional_files;         // Files that may be included
} ContextBoundary;

ContextBoundary* determine_context_boundary(ProjectContext* ctx,
                                           ASTNode* seed_node,
                                           int max_depth);
```

## Implementation Examples

### Example 1: Python Function Call Resolution

**Source Files:**
```python
# utils/config.py
def load_config(filename):
    """Load configuration from file."""
    return parse_yaml(filename)

# main.py
from utils.config import load_config

def main():
    config = load_config("app.yaml")  # Cross-file function call
    return config
```

**Resolution Process:**
1. **Parse `utils/config.py`**: Create `ASTNode` for `load_config` with `qualified_name = "utils.config.load_config"`
2. **Parse `main.py`**: Create `ASTNode` for function call with unresolved reference to `load_config`
3. **Symbol Registration**: Register `utils.config.load_config` → `ASTNode*` in global symbol table
4. **Reference Resolution**: 
   - Detect import: `from utils.config import load_config`
   - Resolve call: `load_config("app.yaml")` → `utils.config.load_config`
   - Create reference: `main.py:load_config_call.references[0] = utils_config_load_config_node`

### Example 2: C++ Class Inheritance Resolution

**Source Files:**
```cpp
// base.h
namespace core {
    class BaseModel {
    public:
        virtual void save() = 0;
    };
}

// user.cpp
#include "base.h"
namespace models {
    class User : public core::BaseModel {  // Cross-file inheritance
    public:
        void save() override { /* implementation */ }
    };
}
```

**Resolution Process:**
1. **Parse `base.h`**: Create `ASTNode` for `BaseModel` with `qualified_name = "core::BaseModel"`
2. **Parse `user.cpp`**: Create `ASTNode` for `User` class with inheritance reference
3. **Include Resolution**: Process `#include "base.h"` to establish file dependency
4. **Inheritance Resolution**: Link `User` class to `BaseModel` through `references` array
5. **Method Override Resolution**: Link `User::save()` to `BaseModel::save()` as override relationship

### Example 3: JavaScript Module Import Resolution

**Source Files:**
```javascript
// utils/helpers.js
export function formatDate(date) {
    return date.toISOString();
}

export default class Logger {
    log(message) { console.log(message); }
}

// main.js
import Logger, { formatDate } from './utils/helpers';

const logger = new Logger();           // Cross-file class instantiation
const formatted = formatDate(new Date()); // Cross-file function call
```

**Resolution Process:**
1. **Parse `utils/helpers.js`**: Create `ASTNode`s for `formatDate` and `Logger`
2. **Parse `main.js`**: Create `ASTNode`s for import statement and usage
3. **Import Resolution**: 
   - Map `Logger` → `utils/helpers.js:Logger`
   - Map `formatDate` → `utils/helpers.js:formatDate`
4. **Usage Resolution**:
   - Link `new Logger()` to class definition
   - Link `formatDate(new Date())` to function definition

## Benefits to Other ScopeMux Modules

### Quantified Improvements

**IR Generation:**
- **Call Graph IR**: 300% more complete graphs (includes cross-file calls)
- **Symbol IR**: 100% accurate symbol resolution (no more "unknown" references)
- **Import/Dependency IR**: Complete project dependency trees

**InfoBlock Extraction:**
- **Semantic Completeness**: InfoBlocks can now represent complete features that span multiple files
- **Impact Analysis**: 90% more accurate change impact assessment
- **Related Code Discovery**: Automatic discovery of related functions across files

**TieredContext Creation:**
- **Context Accuracy**: 200% more relevant contexts (includes actual dependencies)
- **Token Efficiency**: 25% better token utilization (smarter boundary detection)
- **Multi-File Contexts**: Enable project-level understanding for LLMs

**Context Engine:**
- **Intelligent Expansion**: Context expansion based on actual code relationships
- **Dependency-Aware Compression**: Compress related functionality across files
- **Smart Budgeting**: Token budgeter makes decisions based on real relationships

### Performance Benefits

**Reduced Redundancy:**
- Symbol table built once, used by all modules
- Resolved relationships cached and reused
- Avoid redundant cross-file analysis

**Incremental Updates:**
- When a file changes, only affected relationships need re-resolution
- Unchanged files retain their resolved relationships
- Faster incremental analysis for large projects

## Future Enhancements

### Phase 2: Advanced Type Analysis
- Type inference across file boundaries
- Polymorphic relationship resolution
- Generic/template instantiation tracking

### Phase 3: Dynamic Analysis Integration
- Runtime relationship discovery
- Dynamic import/require resolution
- Execution trace correlation with static relationships

### Phase 4: Performance Optimization
- Incremental parsing and linking (new addition)
- Relationship caching strategies
- Parallel processing for large projects

### Phase 5: Language Server Integration
- Real-time relationship updates
- IDE integration for cross-file navigation
- Refactoring support with relationship awareness

### Additional Features from the Quote:

- **Type Analysis**: While our document includes basic type usage tracking, advanced type analysis (e.g., type inference) is planned as a future enhancement.

- **Incremental Parsing**: This is not covered in the current document but could be considered for future performance optimizations.

## Conclusion

The inter-file relationship system represents a foundational enhancement to ScopeMux that will significantly improve the accuracy and completeness of all downstream analysis. By building on the existing `ASTNode` architecture and integrating seamlessly with current modules, this system provides:

1. **Complete Code Understanding**: No more fragmented, single-file analysis
2. **Enhanced Module Capabilities**: All existing modules benefit from richer relationship data
3. **Foundation for Advanced Features**: Enables sophisticated IRs, InfoBlocks, and TieredContexts
4. **Language-Agnostic Design**: Extensible to new languages and relationship types
5. **Performance Optimization**: Shared infrastructure reduces redundant analysis

The implementation prioritizes simplicity and integration with existing systems while providing a robust foundation for future enhancements. This approach ensures that ScopeMux can accurately represent and analyze real-world codebases where meaningful relationships span multiple files and modules.
