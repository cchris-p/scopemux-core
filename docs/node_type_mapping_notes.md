# Config-Driven Node Type Mapping in ScopeMux

## Overview
ScopeMux now uses a configuration-driven approach for mapping semantic query types (from Tree-sitter queries) to internal AST node type enums. This allows for easy extension and maintenance without recompiling the codebase.

## Config File
- **Location:** `core/config/node_type_mapping.json`
- **Format:** JSON object mapping query type strings (e.g., "functions") to enum names (e.g., "NODE_FUNCTION").
- **Example:**
  ```json
  {
    "functions": "NODE_FUNCTION",
    "classes": "NODE_CLASS",
    "variables": "NODE_VARIABLE"
  }
  ```
- To add support for a new query type, simply add a new entry to this file.

## Loader Implementation
- **Header:** `core/include/scopemux/config/node_type_mapping_loader.h`
- **Source:** `core/src/config/node_type_mapping_loader.c`
- Provides:
  - `void load_node_type_mapping(const char *config_path);`
  - `ASTNodeType get_node_type_for_query(const char *query_type);`
  - `void free_node_type_mapping(void);`
- The loader is initialized at parser startup and cleaned up at shutdown.

## Parser Integration
- The parser uses `get_node_type_for_query` to map query types to AST node types.
- If a mapping is missing, a warning is logged and `NODE_UNKNOWN` is returned.

## Error Handling
- Missing config files or unmapped query types are logged as errors or warnings.
- The system falls back to `NODE_UNKNOWN` for unmapped types.

## Extensibility
- No code changes are required to add new mappings—just update the JSON config.
- If you add new semantic types to queries, update the config file accordingly.

## Maintenance Notes
- If the config format becomes more complex, consider using a full JSON parser.
- The loader is currently optimized for a small, flat mapping.

---

*See code comments in loader and parser files for further details on integration and usage.*
