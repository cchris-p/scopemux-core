# ✅ ScopeMux Parser Evaluation & Finalization Report (AST/CST Readiness)

## 📦 Modules Evaluated

| Module | % Complete | Notes |
| --- | --- | --- |
| `parser.h` | **90%** | Well-structured definitions for `ASTNode`, `CSTNode`, and `ParserContext`. Minor naming inconsistencies (`*_new` vs `*_create`) should be resolved. |
| `tree_sitter_integration.c` – CST | **95%** | Stable recursive CST generator (`ts_tree_to_cst`) with good fidelity and correct range tracking. |
| `tree_sitter_integration.c` – AST | **70%** | Basic AST construction via query matches works. Missing robust parent linking, reference extraction, and detailed signature parsing. |
| `parser.c` | **75%** | Functional orchestration, but disorganized. Redundant function definitions and mixed naming patterns. |
| Language Detection | **40%** | Extension-based only. No support for shebangs, modelines, or embedded language heuristics. |
| `query_manager.c` | **95%** | Solid query loading, compiling, and caching system. Simple linked list may limit future scalability. |
| AST Tests | **60%** | Tests exist but don’t validate hierarchy, references, or signature extraction edge cases. |

***

## 🚦 InfoBlock Implementation Readiness

### ✅ What You Can Build Now ( do not implement yet !!! ):

* InfoBlocks for **functions**, **classes**, **methods**
* Metadata: `qualified_name`, `docstring`, `source_range`, `raw_content`
* Basic hierarchy (parent-child)

### ❗️What Must Be Implemented Next:

> To unlock full InfoBlock functionality — including advanced analysis, navigation, and tooling — the following must be implemented as high-priority next steps:

* **Call Graphs / Symbol References**
  🔴 `ASTNode->references` is currently unused. This blocks usage tracking, call graphs, and symbol linking.
* **Structured Signature Parsing**
  🔴 Parameter/return types are not extracted or structured. This blocks rich InfoBlocks, hover tooltips, and function introspection.
* **Cross-File Symbol Resolution**
  🔴 No infrastructure yet for resolving names across files. This is essential for building global symbol tables and refactoring tools.

***

## 🛠️ To-Do List with Concise “How-To” Details

### 1. 🧹 Refactor `parser.c`

* 🔁 Remove redundant helpers (`ast_node_new`, `ast_node_create`, etc.)
* ✅ Pick and standardize one naming convention for node creation
* 🧭 Group logic into clear blocks: init, parse, free, helpers

***

### 2. 🌐 Improve Language Detection

* 🔍 Add `detect_language_from_contents(source: const char*)`
* ⚙️ Scan first few lines for:
  * Shebang lines (`#!/usr/bin/env python`)
  * Modelines (`// -*- C++ -*-`, `# vim:`)
* 🧪 Fall back to extension-based detection if above fail

***

### 3. 🌳 **\[Critical] Enhance AST Functionality**

### a. 🔴 **Implement Symbol References**

* Add `.scm` queries for:
  * `@reference.call`
  * `@reference.identifier`
* In `process_query`, find referenced nodes and call `ast_node_add_reference(...)`
* Store reference metadata (e.g. type, target\_id, range)

### b. 🔴 **Structured Signature Parsing**

* Extend `.scm` to capture:
  * Parameter names/types
  * Return types
* Store in structured form (array of `Parameter` objects or JSON-parsable string)
* Add to `ASTNode->signature` or `ASTNode->parameters[]`

### c. 🟡 **Fix Hierarchy Resolution**

* Replace naive `node_map` with two-pass AST build:
  1. Create all nodes first
  2. Then resolve parent-child relations via `scope_id` or Tree-sitter fields

***

### 4. 🧩 Improve Query Definitions

* Add missing capture groups:
  * `@parameter`, `@return_type`, `@nested`
* Namespace capture labels to avoid collision:
  * `@definition.function`, `@reference.call`, etc.

***

### 5. 🧪 Expand AST Tests

* Add tests for:
  * Parent-child hierarchy correctness
  * Reference extraction
  * Signature parsing structure
* Update `.expected.json` files in example directory to reflect changes
* Add golden file tests for complex nesting & symbol usage

***

### 6. 📦 Launch Minimal InfoBlock Implementation

* Define `InfoBlock` schema with:
  * `id`, `type`, `qualified_name`, `docstring`, `source_range`, `raw_content`
* Traverse AST and emit InfoBlocks for known node types
* Output to JSON or SQLite (to be consumed by CLI/UI tools)
* Validate early via CLI tool:

  ```bash
  ./scopemux info list --file sample.c

  ```

## 📝 Additional (Optional) Suggestions

* Add hash-based query cache if `.scm` count grows
* Auto-generate Graphviz diagram of CST ➜ AST ➜ InfoBlock pipeline
* Add a `CONTRIBUTING.md` section for adding new language support

***

## 📌 Summary

You are **80–85% complete** on parsing infrastructure.

### ✅ CST: Fully usable

### ✅ AST: Sufficient for basic structure

### 🚨 AST Enhancement **Required Immediately** for:

* Symbol references
* Signature breakdown
* Cross-file symbol linking

**✅ Final Next Steps (Do These First)**

1. Refactor and clean up parser.c
2. Implement symbol reference extraction
3. Add structured signature parsing
4. Improve AST node hierarchy robustness
5. Build thin InfoBlock layer and iterate on real output
