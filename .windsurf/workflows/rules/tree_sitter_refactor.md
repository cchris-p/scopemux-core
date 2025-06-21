---
description: Tree Sitter Refactor
---

<todo_list>
See implementation steps in: `api_refactor_todo.md`
</todo_list>

<notes>
See notes in: `api_refactor_notes.md`
</notes>

<priority>
HIGH
</priority>

<phases>
PHASE 1 — EMERGENCY EXTRACTION  
‣ Extract test-specific logic → `processors/test_processor.c`  
‣ Extract post-processing → `processors/ast_post_processor.c`  
‣ Extract docstring handling → `processors/docstring_processor.c`

PHASE 2 — CORE FUNCTION DECOMPOSITION  
‣ Split `ts_tree_to_ast()` into:
  · `create_ast_root()`  
  · `process_all_queries()`  
  · `post_process_ast()`  
‣ Split `process_query_matches()` into:
  · `extract_node_info_from_captures()`  
  · `create_ast_node_from_match()`  
  · `establish_node_relationships()`

PHASE 3 — DIRECTORY STRUCTURE  
‣ Make `adapters/`, `processors/`, `config/` under `core/src/`  
‣ Mirror headers under `core/include/scopemux/`

PHASE 4 — LANGUAGE ADAPTERS  
‣ Base interface → `adapters/language_adapter.{h,c}`  
‣ Implement `c_adapter.c` (+ skeletons for py/js/ts)

PHASE 5 — PROCESSOR PIPELINE  
‣ Base processor → `processors/base_processor.{h,c}`  
‣ Chain exec → `processors/processor_chain.{h,c}`

PHASE 6 — CONFIG SYSTEM  
‣ JSON loader → `config/config_loader.{h,c}`  
‣ Create initial `languages/c.json`, `contexts/basic_tests.json`, etc.

PHASE 7 — CORE CLEANUP  
‣ Rename `tree_sitter_integration.c` → `tree_sitter_core.c`  
‣ Move AST build to `parser/ast_builder.c`

PHASE 8 — BUILD & TESTS  
‣ Update CMake + unit tests for each adapter/processor
</phases>

<success_criteria>
• No function > 100 lines  
• No file > 500 lines  
• Zero test logic in production code  
• New language ⇒ adapter + config only  
• Code coverage ≥ 80 %
</success_criteria>