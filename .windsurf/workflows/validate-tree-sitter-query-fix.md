---
description: Ensure proposed Tree-sitter query fixes are grounded in actual source grammar definitions before modifying query files.
---

## Step 1: Re-anchor to the Primary Issue

Summarize:
- The original test failure or debug log output.
- What specific node or pattern failed to match or compile.
- Why this issue matters to the broader test or application.
- Do **not** revert to a minimal query. Instead, iterate over the existing `functions.scm` contents to fix what’s broken and reach an ideal long-term state.

## Step 2: Gather Actual Grammar Definitions

For the language in question, locate and summarize the following files from the vendor grammar directory:

- `grammar.js` (or `grammar.json`)
- `node-types.json`
- `queries/<lang>/<query>.scm` (existing patterns if present)

These files are typically located under:

```

vendor/tree-sitter-<language>/

````

### Extract:
- Definitions of relevant node types (e.g., `function_definition`, `declaration`, `function_declarator`).
- How nesting or field names (e.g., `field('declarator', ...)`) are structured.
- What `_`-prefixed rules expand into (`_declarator`, `_expression`, etc.).

⚠️ Do **not** rely on prior memory or guesses. Use `fs.readFile()` or semantic file search across the repo to verify.

## Step 3: Identify Ground Truth Structure

Based on the above source code:

- Map the real nesting hierarchy of the failing construct.
- List allowed captures per node from `node-types.json`.
- Identify whether the proposed capture (e.g., `@params`) is valid for that node.

Mark any discrepancy between the proposed query fix and the real grammar.

## Step 4: Rewrite the Minimal Matching Pattern

Create a minimal matching query (e.g., `(function_definition) @function`) that:
- Is guaranteed to compile
- Only uses valid node types and capture targets
- Avoids double captures on the same node

Test this in isolation before adding additional captures.

## Step 5: Incrementally Propose Fixes

For each incremental query addition:
- Show the new proposed pattern
- Cite the node-type and grammar source backing it
- Confirm the capture is unique per node

Example:
```scm
(declaration
  declarator: (function_declarator) @function) ;; backed by grammar.js L47 and node-types.json L12
````

## Step 6: Re-run Test Suite

Ask the user (or the system) to re-run the relevant AST or query tests.
If it fails, output:

* \[QUERY\_DEBUG] errors
* Compiled match count
* Which patterns failed to match

## Step 7: Confidence Report & Divergence Summary

Summarize:

* Confidence level in current fix (%)
* Any prior divergent paths that were taken but not grounded in grammar
* Any unexplored grammar constructs (e.g., pointer\_declarator, init\_declarator)

## Step 8: Repeat or Finalize

If success:w

* Save final working query
* Log grammar-backed rationale

If not:

* Return to Step 3 and explore alternative node paths based on full grammar tree

```

---

### ✅ Benefits

- **Language-agnostic**: Works with any Tree-sitter grammar (C, Python, JS, etc.)
- **Failsafe**: Prevents invalid `.scm` edits by requiring grammar introspection
- **Contextual**: Keeps the assistant grounded in the real source tree
- **Traceable**: Ties each pattern change to specific source lines