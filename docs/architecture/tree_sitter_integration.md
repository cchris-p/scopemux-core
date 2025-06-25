# Tree-sitter Integration Architecture

## Overview

ScopeMux implements a powerful dual-mode parsing system using Tree-sitter to provide both concrete and abstract syntax tree representations of source code. This document describes the architecture, component interactions, and extension points of the system.

## Key Components

### 1. Tree-sitter Integration Layer

The Tree-sitter integration layer provides a bridge between the Tree-sitter C API and ScopeMux's parsing infrastructure. This layer is responsible for:

- Initializing Tree-sitter parser and language libraries
- Managing memory for Tree-sitter objects
- Providing wrapper functions to convert between Tree-sitter nodes and ScopeMux's CST/AST structures
- Handling errors and exceptions from Tree-sitter

**Key Files:**
- `core/src/parser/tree_sitter_integration.c`: Main integration facade
- `core/src/parser/ts_init.c`: Parser initialization and configuration
- `core/src/parser/ts_ast_builder.c`: Conversion from TS trees to AST
- `core/src/parser/ts_query_processor.c`: Query execution and AST node creation

### 2. Dual-Mode Parsing

ScopeMux supports two parsing modes:

#### CST Mode (Concrete Syntax Tree)

- **Purpose**: Preserve full syntactic structure with every token and punctuation
- **Source**: Direct Tree-sitter output through `ts_tree_to_cst`
- **Use Cases**: Source reconstruction, formatting, precise source range mapping

#### AST Mode (Abstract Syntax Tree)

- **Purpose**: Provide semantic structure for analysis and transformation
- **Source**: Generated via Tree-sitter queries through `ts_tree_to_ast`
- **Use Cases**: Symbol IR, call graphs, semantic analysis, refactoring

### 3. Query-Driven AST Extraction

AST generation follows a query-driven approach:

1. `ts_tree_to_ast` creates a basic AST root node
2. Tree-sitter queries are executed on the tree for different semantic constructs
3. Each match is converted into an AST node with semantic information
4. Nodes are organized into a hierarchical structure
5. Post-processing applies qualified naming and other refinements

**Query Ordering:**
The AST generation process follows a specific order for query execution:

1. `classes.scm` - Extract classes first to establish container hierarchy
2. `structs.scm` - Extract struct definitions
3. `functions.scm` - Top-level functions
4. `methods.scm` - Class/struct methods
5. `variables.scm` - Variable declarations
6. `imports.scm` - Import/include statements
7. `docstrings.scm` - Documentation strings

### 4. QueryManager

The QueryManager handles query compilation and caching:

- Loads Tree-sitter query (`.scm`) files from language-specific directories
- Compiles and caches queries for performance
- Reports errors in query compilation and execution

**Query Directory Structure:**
```
queries/
├── c/
├── cpp/
│   ├── classes.scm
│   ├── docstrings.scm
│   ├── functions.scm
│   ├── methods.scm
│   ├── structs.scm
│   └── ... other semantic queries
├── python/
└── ... other languages
```

### 5. Node Type Mapping

ScopeMux maps Tree-sitter query types to AST node types using a JSON configuration:

- Each language can define its own mappings
- Common node types are shared across languages
- Query names are mapped to enumerated node types

**Mapping Configuration:** `core/config/node_type_mapping.json`

## Extending the System

### Adding a New Language

1. **Add Tree-sitter Grammar**:
   - Include Tree-sitter grammar library
   - Register the language in `ts_init.c`

2. **Create Query Files**:
   - Add a language subdirectory in `queries/`
   - Create `.scm` files for each semantic construct

3. **Update Node Type Mappings**:
   - Add language-specific mappings to `node_type_mapping.json`

4. **Add Test Coverage**:
   - Create example test files in `core/tests/examples/<language>/`
   - Add language-specific test cases

### Adding New Semantic Queries

1. **Create a new `.scm` query file** for the semantic construct
2. **Add the query type** to the node type mapping JSON
3. **Update the query execution order** in `process_all_ast_queries`
4. **Add test cases** that utilize the new semantic construct

## Best Practices

### Query Development

- Use Tree-sitter's query language documentation as a reference
- Test queries against a variety of real code examples
- Include comments in query files to explain complex patterns
- Break down large queries into focused, maintainable files

### AST Post-processing

- Apply consistent naming schemes for qualified names
- Verify parent-child relationships are established correctly
- Add richer semantic information when available
- Preserve source ranges for source mapping

### Error Handling

- Implement fallback mechanisms for query failures
- Provide detailed diagnostics for query compilation issues
- Always ensure a valid AST root even when queries fail
- Log detailed information about node counts and performance

## Performance Considerations

- Queries are cached to avoid repeated compilations
- Parse only what's needed using selective query execution
- Consider memory use for large files and deep trees
- Use incremental parsing for continuously edited files

## References

- [Tree-sitter Documentation](https://tree-sitter.github.io/tree-sitter/)
- [Tree-sitter Query Language](https://tree-sitter.github.io/tree-sitter/using-parsers#query-syntax)
