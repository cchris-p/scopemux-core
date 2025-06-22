# ScopeMux Canonical AST/CST Schema

This document defines the canonical schema for Abstract Syntax Tree (AST) and Concrete Syntax Tree (CST) JSON outputs in ScopeMux. All parser outputs, test goldens, and downstream features must conform to this schema.

---

## Top-Level Structure

```json
{
  "language": "C", // or other supported language
  "ast": { ... },    // AST root node (see below)
  "cst": { ... }     // CST root node (see below)
}
```

---

## AST Node Schema (Hierarchical, Node-Based)

Each AST node is an object with the following fields:

| Field           | Type            | Required | Description                                                      |
|-----------------|-----------------|----------|------------------------------------------------------------------|
| type            | string          | yes      | Node type (e.g., ROOT, FUNCTION, INCLUDE, DOCSTRING, etc.)       |
| name            | string          | yes      | Symbolic name of the node (may be empty for some nodes)          |
| qualified_name  | string          | yes      | Fully qualified name (file or scope prefixed)                    |
| docstring       | string/null     | no       | Docstring text, if present                                       |
| signature       | string/null     | no       | Function signature, if present                                   |
| return_type     | string/null     | no       | Function return type, if present                                 |
| parameters      | array           | no       | List of parameter objects (see below), may be empty              |
| path            | string/null     | no       | Path for includes/imports, if present                            |
| system          | boolean/null    | no       | True if include is a system header, false for user includes      |
| range           | object          | yes      | `{start_line, start_column, end_line, end_column}`               |
| raw_content     | string          | yes      | Original source text for this node                               |
| children        | array           | yes      | Child AST nodes, may be empty                                    |

### Parameter Object (for functions)
| Field    | Type   | Description           |
|----------|--------|----------------------|
| name     | string | Parameter name       |
| type     | string | Parameter type       |
| default  | string | Default value, if any|

---

## CST Node Schema (Tree-Sitter Concrete Syntax Tree)

Each CST node is an object with the following fields:

| Field    | Type    | Required | Description                                        |
|----------|---------|----------|----------------------------------------------------|
| type     | string  | yes      | Node type (e.g., comment, preproc_include, etc.)   |
| content  | string  | yes      | Source text for the node                           |
| range    | object  | yes      | `{start: {line, column}, end: {line, column}}`     |
| children | array   | yes      | Child CST nodes, may be empty                      |

---

## Example (C, hello_world.c)

```json
{
  "language": "C",
  "ast": {
    "type": "ROOT",
    "name": "ROOT",
    "qualified_name": "hello_world.c",
    "children": [
      {
        "type": "DOCSTRING",
        "name": "file_docstring",
        "qualified_name": "hello_world.c.file_docstring",
        "docstring": "@file hello_world.c\n@brief A simple ...",
        "range": {"start_line": 1, "start_column": 0, "end_line": 11, "end_column": 3},
        "raw_content": "/** ... */",
        "children": []
      },
      {
        "type": "INCLUDE",
        "name": "stdio_include",
        "qualified_name": "hello_world.c.stdio_include",
        "path": "stdio.h",
        "system": true,
        "range": {"start_line": 13, "start_column": 0, "end_line": 13, "end_column": 19},
        "raw_content": "#include <stdio.h>",
        "children": []
      },
      {
        "type": "FUNCTION",
        "name": "main",
        "qualified_name": "hello_world.c.main",
        "signature": "int main()",
        "return_type": "int",
        "parameters": [],
        "docstring": "@brief ...",
        "range": {"start_line": 19, "start_column": 0, "end_line": 22, "end_column": 1},
        "raw_content": "int main() { ... }",
        "children": [ /* function body nodes */ ]
      }
    ]
  },
  "cst": {
    "type": "translation_unit",
    "content": null,
    "range": {
      "start": {"line": 0, "column": 0},
      "end": {"line": 22, "column": 1}
    },
    "children": [
      {
        "type": "comment",
        "content": "/** ... */",
        "range": {"start": {"line": 1, "column": 0}, "end": {"line": 11, "column": 3}},
        "children": []
      },
      {
        "type": "preproc_include",
        "content": "#include <stdio.h>",
        "range": {"start": {"line": 13, "column": 0}, "end": {"line": 13, "column": 19}},
        "children": [
          {
            "type": "system_lib_string",
            "content": "stdio.h",
            "range": {"start": {"line": 13, "column": 10}, "end": {"line": 13, "column": 18}},
            "children": []
          }
        ]
      },
      {
        "type": "function_definition",
        "content": "int main() { ... }",
        "range": {"start": {"line": 19, "column": 0}, "end": {"line": 22, "column": 1}},
        "children": [ /* ... */ ]
      }
    ]
  }
}
```

---

## Notes
- All fields must be present in every node (use `null` or empty values if not applicable).
- The schema is designed to be extensible for new node types, languages, and features.
- Any changes to this schema must be documented and reflected in all test goldens.
- Downstream tools and documentation generators should rely on this structure as the single source of truth.
