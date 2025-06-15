# 🌲 ScopeMux AST Parsing Enhancement Tasks

This document outlines the specific tasks needed to complete the AST parsing capabilities in ScopeMux. It builds upon the architectural foundation that's already in place but focuses on expanding functionality to make AST parsing fully useful across multiple languages.

## 📋 Task Overview

Current status:

* ✅ CST parsing is complete and reliable
* ⚠️ AST parsing is architecturally sound but only partially implemented (Python functions only)

## 🔧 1. Expand Query Coverage

### Python Support Enhancement

* \[ ] **Classes**
  * \[ ] Create `queries/python/classes.scm` to capture:
    * \[ ] Class name, body, inheritance
    * \[ ] Decorators and class-level attributes
    * \[ ] Class docstrings
  * \[ ] Example capture pattern:
    ```
    (class_definition
      name: (identifier) @name
      body: (block) @body) @class
    ```

* \[ ] **Methods**
  * \[ ] Create `queries/python/methods.scm` to capture:
    * \[ ] Method name, parameters, body
    * \[ ] Decorators and return type annotations
    * \[ ] Method docstrings
  * \[ ] Example capture pattern:
    ```
    (class_definition
      body: (block 
        (function_definition
          name: (identifier) @name
          parameters: (parameters) @params
          body: (block) @body) @method))
    ```

* \[ ] **Enhanced Functions**
  * \[ ] Update `queries/python/functions.scm` to capture:
    * \[ ] Parameter names and types
    * \[ ] Return type annotations
    * \[ ] Function docstrings
    * \[ ] Default parameter values

* \[ ] **Variables**
  * \[ ] Create `queries/python/variables.scm` for:
    * \[ ] Global variables
    * \[ ] Class attributes
    * \[ ] Local variables
    * \[ ] Type annotations

* \[ ] **Imports**
  * \[ ] Create `queries/python/imports.scm` for:
    * \[ ] Regular imports (`import x`)
    * \[ ] From imports (`from x import y`)
    * \[ ] Aliased imports (`import x as y`)

* \[ ] **Control Flow**
  * \[ ] Create `queries/python/control_flow.scm` for:
    * \[ ] If/elif/else blocks
    * \[ ] For loops
    * \[ ] While loops
    * \[ ] Try/except blocks
    * \[ ] With statements

### C/C++ Support

* \[ ] **Functions**
  * \[ ] Create `queries/c/functions.scm` and/or `queries/cpp/functions.scm`
  * \[ ] Capture prototypes vs implementations
  * \[ ] Handle parameter types, return types

* \[ ] **Structs/Classes**
  * \[ ] Create structure queries for both languages
  * \[ ] Handle member variables and methods
  * \[ ] Capture inheritance for C++

* \[ ] **Variables**
  * \[ ] Global vs local
  * \[ ] Type definitions
  * \[ ] Static/extern qualifiers

* \[ ] **Preprocessor Directives**
  * \[ ] Includes
  * \[ ] Defines
  * \[ ] Conditionals (#if, #ifdef)

### Additional Languages (Pick at least 2-3 more)

* \[ ] JavaScript/TypeScript
* \[ ] Rust
* \[ ] Go
* \[ ] Java

## 🏗️ 2. Enhance AST Extraction Logic

* \[ ] **Update `ts_tree_to_ast` function** in `tree_sitter_integration.c`:
  * \[ ] Add support for loading multiple query types per language
  * \[ ] Example:
    ```c
    const TSQuery *function_query = query_manager_get_query(ctx->q_manager, ctx->language, "functions");
    const TSQuery *class_query = query_manager_get_query(ctx->q_manager, ctx->language, "classes");
    const TSQuery *method_query = query_manager_get_query(ctx->q_manager, ctx->language, "methods");
    ```

* \[ ] **Implement hierarchical AST building**:
  * \[ ] Create parent-child relationships between nodes
  * \[ ] Example: class nodes should contain their method nodes
  * \[ ] Ensure proper nesting of control flow structures

* \[ ] **Rich metadata extraction**:
  * \[ ] Populate all ASTNode fields:
    * \[ ] name
    * \[ ] type (using appropriate AST\_\* constants)
    * \[ ] range (start/end line/column)
    * \[ ] signature (for functions/methods)
    * \[ ] docstring (when present)
    * \[ ] qualified\_name (e.g., Module.Class.method)
    * \[ ] raw\_text
  * \[ ] Create helper functions for field extraction if needed

* \[ ] **Multi-language dispatch**:
  * \[ ] Implement language-specific handling in extraction logic
  * \[ ] Use function pointers or a strategy pattern if differences are substantial

## 🧪 3. Testing & Validation

* \[ ] **Create test files**:
  * \[ ] Python files with diverse constructs (classes, methods, nested functions, etc.)
  * \[ ] C/C++ files with various language features
  * \[ ] Files for each additional language

* \[ ] **Unit tests**:
  * \[ ] Test each query individually against sample code
  * \[ ] Verify capture groups extract expected information
  * \[ ] Test AST node creation with various inputs

* \[ ] **Integration tests**:
  * \[ ] Verify full AST structure for complete files
  * \[ ] Test hierarchical relationships (parent-child correctness)
  * \[ ] Validate field population (e.g., signatures, docstrings)

* \[ ] **Edge cases**:
  * \[ ] Test with unusual or complex code patterns
  * \[ ] Test with code that uses language-specific features
  * \[ ] Test with malformed but syntactically valid code

## 📝 4. Documentation & Refinement

* \[ ] **Update documentation**:
  * \[ ] Document all supported AST node types
  * \[ ] Document the structure of query files
  * \[ ] Create examples of AST extraction for each language

* \[ ] **Refactor for maintainability**:
  * \[ ] Extract common patterns into helper functions
  * \[ ] Consider a more declarative approach for query-to-field mapping

* \[ ] **Performance optimization**:
  * \[ ] Profile AST extraction for large files
  * \[ ] Implement caching if beneficial
  * \[ ] Consider lazy loading of certain AST information

## 📈 Detailed Implementation Plan

### Week 1: Query Expansion & Initial Logic

1. \[ ] Expand Python queries (classes, methods, variables)
2. \[ ] Update `ts_tree_to_ast` to handle multiple query types
3. \[ ] Implement initial hierarchical node building

### Week 2: Multi-Language Support

1. \[ ] Add C/C++ query files
2. \[ ] Add language-specific extraction logic
3. \[ ] Implement field population for all node types

### Week 3: Testing & Refinement

1. \[ ] Create comprehensive test suite
2. \[ ] Fix bugs and edge cases
3. \[ ] Optimize performance where needed

### Week 4: Documentation & Additional Languages

1. \[ ] Document the AST parsing system
2. \[ ] Add support for additional languages
3. \[ ] Final integration and validation

## 📚 References

* [Tree-sitter Query Syntax](https://tree-sitter.github.io/tree-sitter/using-parsers#query-syntax)
* [Tree-sitter Grammar Documentation](https://github.com/tree-sitter/tree-sitter/blob/master/docs/grammar.md)
