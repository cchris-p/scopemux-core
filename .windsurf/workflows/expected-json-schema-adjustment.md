---
description: Adjust .expected.json test goldens and ensure compliance with the AST/CST schema
---

# AST/CST Schema Adjustment & Golden Generation Workflow

This workflow describes the process for updating [.expected.json](cci:7://file:///home/matrillo/apps/scopemux/core/tests/examples/c/basic_syntax/hello_world.c.expected.json:0:0-0:0) test goldens to comply with the canonical AST/CST schema defined in [docs/ast_cst_schema.md](cci:7://file:///home/matrillo/apps/scopemux/docs/ast_cst_schema.md:0:0-0:0). It covers both reviewing and updating the schema, and ensuring the Python golden generator is correct. This is the authoritative process for all supported languages.

---

## 1. Examine the Current .expected.json Output

- Locate the relevant [.expected.json](cci:7://file:///home/matrillo/apps/scopemux/core/tests/examples/c/basic_syntax/hello_world.c.expected.json:0:0-0:0) file(s) in `core/tests/examples/[language]/[test_case].expected.json`.
- Review the structure of both the `ast` and `cst` fields.
- Compare the field presence, types, and values to the schema in [docs/ast_cst_schema.md](cci:7://file:///home/matrillo/apps/scopemux/docs/ast_cst_schema.md:0:0-0:0).

## 2. Refresh on the Canonical Schema

- Open [docs/ast_cst_schema.md](cci:7://file:///home/matrillo/apps/scopemux/docs/ast_cst_schema.md:0:0-0:0).
- Ensure you are referencing the latest schema for both AST and CST nodes.
    - All fields must be present in every node (use `null` or empty values if not applicable).
    - The schema is the single source of truth for downstream tools and documentation.

## 3. Update the Python Golden Generator

- Always check the implementation accuracy in the C parser first. This is the source of truth. If everything is reviewed here, then we will proceed to the python code.
- Edit `core/tests/tools/generate_expected_json.py`.
    - This script is responsible for generating [.expected.json](cci:7://file:///home/matrillo/apps/scopemux/core/tests/examples/c/basic_syntax/hello_world.c.expected.json:0:0-0:0) files for all supported languages.
    - Ensure that:
        - All required fields from the schema are present in every node (use `null`/empty as needed).
        - Fields are named and typed exactly as in the schema.
        - The structure of both AST and CST matches the schema, including children arrays and nested objects.
        - Any new fields or node types added to the schema are supported.
    - If the generator relies on C/C++ code (e.g., via pybind or subprocess), check the output from the parser (see `parser.c` and bindings).
        - If the parser output is missing fields, coordinate with C-side maintainers to update the IR or binding.

## 4. (Optional) Update the Schema

- If user guidance or new requirements dictate changes to the schema:
    - Update [docs/ast_cst_schema.md](cci:7://file:///home/matrillo/apps/scopemux/docs/ast_cst_schema.md:0:0-0:0) accordingly.
    - Ensure all test goldens and the generator script are updated to match the new schema.

---

## Related Files

- **docs/ast_cst_schema.md**: The source of truth for AST/CST structure.
- **core/tests/tools/generate_expected_json.py**: Python script to generate [.expected.json](cci:7://file:///home/matrillo/apps/scopemux/core/tests/examples/c/basic_syntax/hello_world.c.expected.json:0:0-0:0) goldens.
- **core/tests/examples/[language]/**: Directory containing test source files and their goldens.
- **parser.c**: C implementation of parsing logic and AST/CST IR generation.
- **build_all_and_pybind.sh**: Script to build and update Python bindings if changes are made to the C parser.

---

## Notes

- Never forward-declare a typedef for an enum in a header unless the full definition is present.
- Always include the header that defines the typedef if using it in function signatures or structs.
- All test runner scripts (e.g., `run_c_tests.sh`) perform a clean build automatically.
- Use the `grep` tool for searching code/text patterns in this project.

---

## Example: Updating hello_world.c.expected.json

- Ensure the root AST node is of type `ROOT`, has correct `name`, `qualified_name`, and contains all top-level children (DOCSTRING, INCLUDE, FUNCTION, etc.).
- All fields listed in the schema must be present, with `null` or empty values if not applicable.
- The CST must include all tokens and structure as described.

---

## Troubleshooting

- If fields are missing from the generated JSON, check both the Python script and the C parser output.
- If the schema is unclear, consult the latest [docs/ast_cst_schema.md](cci:7://file:///home/matrillo/apps/scopemux/docs/ast_cst_schema.md:0:0-0:0) or raise a documentation update request.