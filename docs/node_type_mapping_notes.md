# Node Type Mapping in ScopeMux (2025+)

## Overview
ScopeMux now uses a **hardcoded, code-driven** approach for mapping semantic query types (from Tree-sitter queries) to internal AST node type enums. This replaces the previous config/JSON-based system for improved reliability, simplicity, and build reproducibility.

## How It Works
- Semantic query types (e.g., "functions", "classes") are mapped to `ASTNodeType` enums **directly in C code**.
- The mapping logic lives in:
  - **Header:** `core/include/config/node_type_mapping_loader.h`
  - **Source:** `core/src/config/node_type_mapping_loader.c`
- The function `ASTNodeType get_node_type_for_query(const char *query_type);` provides the mapping at runtime.

## Rationale for the Change
- **No external dependencies:** Eliminates runtime errors due to missing or out-of-sync config files.
- **Performance:** No file I/O or parsing overhead.
- **Build reproducibility:** Mapping is version-controlled and updated with the codebase.
- **Simplicity:** Adding a new mapping is a single-line code change.

## How to Add or Change a Mapping
1. **Edit the C source:**
   - Open `core/src/config/node_type_mapping_loader.c`.
   - Locate the static mapping table or the logic in `get_node_type_for_query`.
   - Add or modify an entry for your new query type and corresponding enum.
2. **Recompile the project.**
3. **(Optional)**: Update `core/include/config/node_type_mapping_loader.h` if you add new enums.

## Example
```c
// In node_type_mapping_loader.c:
if (strcmp(query_type, "functions") == 0)
    return NODE_FUNCTION;
if (strcmp(query_type, "classes") == 0)
    return NODE_CLASS;
// ...
```

## Error Handling
- If a mapping is missing, a warning is logged and `NODE_UNKNOWN` is returned.
- No config file errors can occur.

## Extensibility
- To add support for a new semantic query type, just add a new `strcmp` branch or entry in the mapping logic.
- All mappings are now tracked and reviewed through code review/version control.

## Maintenance Notes
- The code-based mapping is optimized for a small, flat set of semantic types.
- If the mapping grows large, consider using a hash table or macro table for maintainability.

---

## AST Schema Compliance and Test Requirements

### Source of Truth

**The canonical source of truth for AST schema requirements is the `.expected.json` files in the test directory (`core/tests/parser/interfile_tests/expected/`).** These files define the exact schema structure that must be matched by the parser output.

### Critical Schema Requirements

Some tests have very specific requirements for node structure and ordering:

- **variables_loops_conditions.c test:**
  - Node at index 0 (root): Must have empty name and qualified_name
  - Node at index 4: Must be DOCSTRING with empty name and qualified_name
  - Node at index 12: Must be FUNCTION with name and qualified_name "main"

### Compliance Implementation

The schema compliance logic is implemented in language-specific compliance handlers:

- C language: `core/src/parser/lang/c_compliance.c`
- Python language: `core/src/parser/lang/python_compliance.c`
- etc.

### Modifying Compliance Logic

WARNING: Changes to compliance logic MUST be synchronized with updates to the corresponding `.expected.json` test files. Failure to maintain this synchronization will result in test failures.

---

*See code comments in `core/src/config/node_type_mapping_loader.c` for further details on integration and extension.*
