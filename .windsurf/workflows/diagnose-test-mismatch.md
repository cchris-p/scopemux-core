---
description: This workflow ensures the assistant aligns Tree-sitter query edits with actual test expectations and corrects misattribution of errors.
---

## Step 1: Anchor to the Primary Task

Ask:
- What test(s) are currently failing?
- What file(s) or queries are believed to be responsible?
- What specific JSON output is expected vs. received?

Summarize the primary goal:
> “Get the test suite to pass by ensuring Tree-sitter queries produce ASTs that match `.expected.json` files.”

---

## Step 2: Disambiguate the Current Focus

Ask explicitly:
> “Is the current failure related to functions, variables, or something else?”

If the assistant is referencing an unrelated test (e.g., talking about `variables.scm` when `functions.scm` is failing), clarify:

✅ Only proceed with edits related to the currently failing query  
🚫 Do **not** drift into previously fixed areas unless a new test failure is clearly introduced

---

## Step 3: Gather and Compare Test Output

Request or extract:
- The failing `.expected.json` and the actual test output JSON
- Any diffs shown by the test runner
- The name of the test, e.g., `ast_extraction::c_functions`

If this is not available, ask the user to rerun the test suite and provide the `[QUERY_DEBUG]` or `.diff` output

---

## Step 4: Map Output Expectations to Query Captures

For each failing node (e.g., a missing `"FUNCTION"`):

- Identify the required fields in `.expected.json` (e.g., `name`, `return_type`, `params`)
- Verify whether the current `.scm` query:
  - Matches that node type
  - Captures the correct metadata with proper names (@name, @variable_type, etc.)
  - Avoids multiple captures on the same node

Log clearly:
> ❌ Missing capture for @params in (function_definition)  
> ✅ Capturing @name and @return_type

---

## Step 5: Edit Queries Only If Backed by Ground Truth

Do not propose any changes until you verify:
- What the grammar allows (`grammar.js`, `node-types.json`)
- That the capture names used align with the processor
- That the test diff justifies the change

If uncertain, output:
> “I cannot confirm this pattern is valid. Please review `vendor/tree-sitter-*/grammar.js`.”

---

## Step 6: Confirm Fixes by Re-running Tests

Once a query is changed:
- Re-run the failing test only
- Review `[QUERY_DEBUG]` for matched nodes and captures
- Confirm the output JSON now matches `.expected.json`

---

## Step 7: Summary & Decision

Summarize:
- Which test was failing
- What was actually fixed
- Which parts were hallucinated or wrongly assumed
- What still needs investigation

Ask the user:
> Would you like to continue fixing other failing nodes or refactor the query to improve structure/completeness?

Then list the failing nodes.

---

## ✅ Benefits

- **Stops hallucinated causality**: Forces model to align query changes with actual test failures
- **Traceable fixes**: Encourages before/after diffs and grammar-backed changes
- **Works across languages**: Any Tree-sitter grammar and `.expected.json` system
- **Reduces noise**: Prioritizes failing tests over speculative improvements
