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

### ✅ **Memory/Object Lifetime IR**

* Purpose: Track creation, references, and lifetimes of objects (esp. in OOP or systems code).
* Use: UML Object Diagrams, Scope-aware memory snapshots, Python/C++ destructor lifecycle tracing

### ✅ **Concurrency IR**

* Description: Maps threads, async calls, locks, and other concurrency primitives.
* Use: Understanding concurrent behavior, debugging race conditions, performance analysis of parallel systems.

***

## 🔹 3. **Semantic IRs (Meaning-Preserving)**

These IRs focus on capturing *intent*, purpose, or user-facing meaning:

### ✅ **Doc IR**

* Description: Extracts and maps docstrings, code comments, annotations, and their relationships to symbols and files. It can represent this information hierarchically or flattened.
* Use: Generating API documentation, code summarization, documentation previews, powering LLM docstring completion.

### ✅ **Activity IR**

* Inspired by UML activity diagrams
* Captures control flow: conditions, loops, branching logic
* Abstracts away details to show *what the code does*
* Use: High-level overviews, strategy simulation, planning

### ✅ **Notebook Execution IR (for ipynb)**

* Tracks code cell execution order, variable state, and dependencies
* Maintains which variables/functions were defined or overwritten by which cells
* Use: Reproducibility tools, cell impact analysis, smart re-execution

### ✅ **Type IR**

* Purpose: Track inferred or declared types for all symbols (variables, functions, parameters, return values).
* Structure: Links symbols to their type information, including user-defined types, built-in types, and generics.
* Use: Type checking, code completion, hover hints, refactoring, generating type overview diagrams, LLM-guided type correction.

### ✅ **Exception Flow IR**

* Purpose: Map which blocks of code or functions can raise specific exceptions and how these exceptions are caught and handled.
* Structure: Represents a graph of potential exception origins and their propagation paths to handler blocks.
* Use: Analyzing error handling patterns, resilience modeling, generating error propagation diagrams, debugging.

### ✅ **Semantic Summary IR**

* Description: Provides a natural-language summary of the purpose or behavior for symbols (functions, classes, etc.), often generated or assisted by LLMs.
* Use: High-level code understanding, documentation generation, onboarding new developers.

### ✅ **Contract IR**

* Description: Captures formal or informal contracts for software components, such as pre-conditions, post-conditions, and invariants for functions or methods.
* Use: Design by contract, automated testing, verification, ensuring component integrity.

### ✅ **Regulatory Compliance IR**

* Description: Identifies and extracts logic, data handling, or code sections related to specific regulatory requirements (e.g., GDPR, HIPAA, safety standards).
* Use: Compliance auditing, impact analysis of regulatory changes, generating compliance reports.

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

### ✅ **Execution Trace IR**

* Purpose: Combines static IRs with dynamic runtime data, such as execution paths, profiling information, variable values at specific points, or logged events.
* Structure: Annotates static IR nodes (e.g., function calls, statements) with runtime observations.
* Use: Debugging, performance profiling, generating trace-based sequence diagrams, dynamic type inference, coverage analysis.

### ✅ **Test Coverage IR**

* Purpose: Maps source code elements (functions, lines, branches) to the tests that cover them.
* Structure: Links test cases to IR nodes representing code units.
* Use: Visualizing test coverage, identifying untested code, suggesting new tests, critical path testing analysis.
