***

## description:

## /scopemux-refamiliarize

Help me (and future teammates) regain context on the ScopeMux project, identify the modules relevant to the task I’m about to tackle, and surface the best next steps.

### 1 ⸺ Confirm repo context

```bash
# Adapt the path if necessary
cd ~/apps/scopemux
git status
```

*If you’re not in the repo root, stop and navigate there before continuing.*

### 2 ⸺ Clarify TODAY’s task

Ask the user (or read the PR / issue you’re working from):

> “What specific bug-fix, feature, or refactor are we resuming today?”
> Store this description as **$TASK** for later reference.

### 3 ⸺ List top-level ScopeMux modules

```bash
echo "📂 Project structure (two levels deep):"
tree -L 2 core/{include,src}
```

> This prints directories such as `parser`, `context_engine`, `bindings`, `common`, etc.

### 4 ⸺ For *each* module directory

Perform the loop **one module at a time**:

1. **Identify name & location** — e.g. `core/src/parser`
2. **Open salient header or README** (if any):

   ```bash
   bat --style=plain --line-range=1:200 core/include/scopemux/<module>.h || true
   ```
3. **Generate a one-sentence purpose statement** (e.g. “`parser` — turns Tree-sitter output into a language-agnostic IR”).
4. **Append** that line to an in-memory markdown list.

### 5 ⸺ Present a concise module cheat-sheet

After all directories are processed, print a markdown block like:

```markdown
### ScopeMux Module Overview
| Module         | Purpose                                             |
| -------------- | --------------------------------------------------- |
| parser         | Generates IR from Tree-sitter CST/AST               |
| context_engine | Compresses & expands InfoBlocks into tiered contexts|
| bindings       | Pybind11 interface exposing C core to Python        |
| common         | Logging, error handling, memory management helpers  |
| ...            | ...                                                 |
```

### 6 ⸺ Map modules ➜ today’s task

Using **$TASK** and the cheat-sheet, answer:

* Which module(s) are most relevant to this task?
* Which supporting modules should I skim or test first?

### 7 ⸺ Suggest immediate next steps

* Open the critical source files in your IDE.
* Run or write the unit tests in `tests/` that touch those modules.
* If architecture knowledge is stale, scan the IR and context\_engine headers to refresh mental models.

### 8 ⸺ Wrap-up summary

Display:

* The full module table.
* The task recap.
* Recommended next actions (links to files or tests).
