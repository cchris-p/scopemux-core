# Supported IRs

The Intermediate Representation (IR) layer is key to enabling structured understanding, compression, and transformation of source materials (code or notebooks). The IR forms act as bridges between raw syntax trees and higher-level InfoBlocks or semantic queries.

## 🔹 1. **Structural IRs (Syntax-Preserving)**

These IRs preserve the structure of the original content but abstract away surface-level syntax:

### ✅ **AST-Reduced IR**

* Source: Tree-sitter or Python AST
* Flattened/filtered AST with only semantically meaningful nodes
* Removes tokens like braces, commas, etc.
* Use: Foundation for tiering, context slicing, and graph generation

### ✅ **Symbol IR**

* Contains all declarations: functions, classes, variables, imports
* Includes:
  * Signature
  * Scope
  * Visibility (public/private)
  * Metadata (docstrings, decorators)
* Use: Fast lookup, context-tiering, IDE jump-to-definition

***

## 🔹 2. **Relational / Graph IRs**

These are more abstract and semantic, built on top of AST or symbol information:

### ✅ **Call Graph IR**

* Nodes: Functions or methods
* Edges: Function/method calls
* Use: Dependency tracking, execution mapping, backtracking stack traces

### ✅ **Data Flow IR**

* Tracks how data moves across variables/functions
* Shows assignment, mutation, propagation
* Use: Refactor safety checks, variable usage compression

### ✅ **Import/Dependency IR**

* Modules/files as nodes
* Edges for import/include/use statements
* Use: Prune large codebases, module-tier context slicing

***

## 🔹 3. **Semantic IRs (Meaning-Preserving)**

These IRs focus on capturing *intent*, purpose, or user-facing meaning:

### ✅ **Docstring IR**

* Extracts only docstrings, grouped by file/symbol
* Can be flattened, hierarchically nested, or aligned to symbol IR
* Use: Code summarization, documentation previews

### ✅ **Activity IR**

* Inspired by UML activity diagrams
* Captures control flow: conditions, loops, branching logic
* Abstracts away details to show *what the code does*
* Use: High-level overviews, strategy simulation, planning

### ✅ **Notebook Execution IR (for ipynb)**

* Tracks code cell execution order, variable state, and dependencies
* Maintains which variables/functions were defined or overwritten by which cells
* Use: Reproducibility tools, cell impact analysis, smart re-execution

***

## 🔹 4. **Tiered Context IR (Compression-Aware)**

Special to your project:

### ✅ **Context Tiers IR**

* Maps each InfoBlock to its level of abstraction:
  * Tier 0: Local line or statement
  * Tier 1: Function/class body
  * Tier 2: File/module level
  * Tier 3: Project/graph level
* Use: Enables ScopeMux’s intelligent expansion/compression pipeline

***

## 🔹 5. **Metadata/Annotation IRs**

Optional overlays for control, tracking, or debugging:

### ✅ **Error Annotation IR**

* Stores error or warning messages tied to IR nodes
* Use: Diagnostic compression, LLM debug prompts

### ✅ **Edit History IR**

* Tracks mutations of files or notebooks over time
* Use: Time-travel analysis, change detection, and contextual caching
