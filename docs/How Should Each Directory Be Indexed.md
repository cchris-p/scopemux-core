# How ScopeMux Indexes a Directory

The goal of the context collector is to enable fast, intelligent interaction with a large codebase—especially for LLMs, search, summarization, and tooling. To that end, ScopeMux indexes the relevant, extractable, and useful parts of the codebase that provide **semantic structure** and **operational intent**. This document outlines the conceptual stages involved in this indexing process, focusing on *how* code is analyzed and understood across various languages (including TypeScript, JavaScript, C++, C, Python).

## Core Indexing Process Stages

The indexing of a directory and its contents proceeds through the following conceptual stages:

### 1. Source Code Ingestion & Syntactic Parsing (CST/AST Generation)

* **Objective**: To read source files from a directory and convert them into fundamental syntactic structures.
* **Process**:
  * Identifies all relevant source files within the target directory, respecting ignore patterns (e.g., from `.gitignore`, excluding build artifacts, vendored dependencies unless specified).
  * For each file, employs a language-specific parser (e.g., leveraging Tree-sitter) to analyze its syntax. This involves breaking down the raw text into a sequence of tokens and then organizing these tokens according to the language's grammar.
  * Generates an initial Concrete Syntax Tree (CST), which is a detailed representation of the source code including all tokens like punctuation and whitespace, or more commonly, an Abstract Syntax Tree (AST). The AST is a simplified tree structure that represents the essential grammatical structure of the code, omitting superfluous details. This includes capturing basic elements like statements, expressions, and declarations with their precise start/end byte and line offsets.
* **Key Outputs (Conceptual)**: Per-file syntactic trees (CSTs/ASTs) with accurate positional (line/byte offset) information for all nodes.

### 2. Semantic Elaboration & Intermediate Representation (IR) Construction

* **Objective**: To enrich syntactic structures (ASTs) with semantic meaning and transform them into a standardized, queryable Intermediate Representation (e.g., composed of "InfoBlocks").
* **Process**:
  * Traverses the ASTs generated in the previous stage to identify and extract detailed information about key code constructs. This involves understanding the role and meaning of different parts of the code beyond just their syntax.
  * **Function and Method Definitions**:
    * Captures full signatures including name, parameter names, their declared types (if available), and return types.
    * Records start/end byte and line offsets for the entire function/method block.
    * Identifies the enclosing class, struct, or namespace if applicable.
    * Notes any modifiers (e.g., `static`, `const`, `virtual`, `async`, `public`, `private`, `protected`).
    * Associates leading comments and docstrings specifically pertaining to the function or method.
  * **Class, Struct, Interface, and Namespace Definitions**:
    * Records their names and precise source code locations (file, line/byte offsets).
    * Analyzes inheritance hierarchies (base classes/structs) and implemented interfaces.
    * Identifies and lists member variables (fields, properties), including their names, types, and access modifiers.
    * Lists all method declarations within the scope of the class/struct, including their signatures and modifiers.
    * Associates class-level docstrings or comments.
  * **Global Variables, Constants, and Macros**:
    * Identifies global or static variables, `constexpr` values in C++, `#define` macros in C/C++, and similar constructs in other languages (e.g., top-level `const` or `let` in JavaScript/TypeScript, module-level constants in Python).
    * Captures their names, declared types, and, where statically determinable, their assigned values or macro expansions.
    * Records their location and any associated comments.
  * **Enums and Typedefs/Aliases**:
    * Extracts enumeration names, their individual members (and their optional explicit values), and the underlying type of the enum if specified.
    * Identifies type definitions (`typedef` in C/C++, `type` aliases in TypeScript/Python) and `using` aliases in C++, capturing the alias name and the type it refers to.
  * **Import/Include Statements**:
    * Parses directives like `#include` (C/C++), `import` (Python, JavaScript, TypeScript), `use` (Rust, PHP), etc.
    * Understands the imported/included module or file path, attempting to resolve relative paths to absolute paths based on project structure or include paths. This information is crucial for building a module dependency graph.
  * **Comments and Docstrings**:
    * Beyond those attached to specific entities, systematically extracts and categorizes comments: per-function/class leading comments, significant inline comments that might annotate control flow or important logic, and special comment tags like `TODO:`, `FIXME:`, `NOTE:`, along with their locations.
  * **Language-Specific Entities**:
    * Recognizes and creates representations for constructs that have significant semantic meaning within specific languages. Examples include:
      * Python: decorators, `async def` functions, `yield` statements, `with` statements, list comprehensions, `@dataclass` and its fields.
      * C++: templates (class and function), virtual methods, friend declarations, lambda expressions, smart pointers.
      * JavaScript/TypeScript: closures, arrow functions, `async/await`, promises, `export/import` syntax for ES modules, TypeScript interfaces and generics, decorators.
      * C: function pointers, bitfields in structs.
  * Transforms this extracted semantic information into a collection of standardized IR units (often referred to as InfoBlocks). Each InfoBlock represents a distinct piece of code (like a function, class, or variable declaration) along with its attributes, content (or summary), and links to related InfoBlocks.
* **Key Outputs (Conceptual)**: A collection of rich, semantic InfoBlocks; an initial module dependency graph based on import/include statements.

### 3. Semantic Graph Construction & Deep Code Understanding

* **Objective**: To connect individual InfoBlocks and build higher-level semantic models representing the codebase's architecture, behavior, and interdependencies. This stage moves from individual code elements to understanding how they interact.
* **Process**:
  * **Control Flow Graph (CFG) Generation**:
    * For each function or method body, analyzes its statements and expressions to construct a Control Flow Graph.
    * Nodes in the CFG represent basic blocks (sequences of non-branching instructions).
    * Edges represent potential control flow transfers, such as those resulting from conditional statements (`if`, `else if`, `else`), `switch` or `match` statements, loops (`for`, `while`, `do-while`), and exception handling blocks (`try-catch-finally`).
    * Edges are often labeled with the conditions that cause the branch (e.g., the boolean expression in an `if` statement) or the type of loop.
  * **Call Graph Construction**:
    * Identifies all call expressions (function calls, method invocations) within the codebase.
    * For each call site, attempts to resolve the call to the specific function(s) or method(s) that could be invoked. This can be complex due to polymorphism, function pointers, dynamic dispatch, or first-class functions.
    * Records these caller-callee relationships, including the location of the call site and potentially the argument structure if it can be statically determined. This builds a graph where nodes are functions/methods and directed edges represent calls.
  * **Symbol Table Resolution and Scope Analysis**:
    * Builds and refines symbol tables for different scopes (global, module/file, class, function, block).
    * For each identifier (variable name, function name, etc.), resolves it to its original declaration to understand its type, origin, and scope of visibility. This helps differentiate between variables with the same name in different scopes.
  * **Data Flow Analysis (Conceptual)**:
    * (Potentially at a basic level) Traces how data is defined through variable declarations or parameter passing, how it is used in expressions, and how it might be modified. This can help in understanding dependencies between different parts of the code.
* **Key Outputs (Conceptual)**: Enriched InfoBlocks with resolved symbol links and relational information; Control Flow Graphs (CFGs) for all functions/methods; a comprehensive Call Graph (or graphs) for the indexed directory; resolved Symbol Tables for various scopes.

### 4. Contextual Indexing & Tiered Context Preparation

* **Objective**: To prepare the semantic models and InfoBlocks for efficient querying, contextual retrieval (especially for LLMs and search tools), and to assign levels of detail (tiers) for flexible context presentation.
* **Process**:
  * **Tier Assignment**: Assigns "tiers" or levels of detail to InfoBlocks. This classification helps in later stages (like context compression for LLMs) by allowing the system to select or summarize information based on its granularity and importance.
    | Tier | Meaning         | Examples                               |
    |------|-----------------|----------------------------------------|
    | 0    | Line-level      | Single token span, specific variable use, an individual expression |
    | 1    | Function/method | A single named callable unit, a class member definition, a significant block within a function |
    | 2    | File/Module     | All symbols and structures within a single source file or logical module    |
    | 3    | Package/Folder  | A distinct package, library, or significant subdirectory scope  |
    | 4    | Project-wide    | Top-level graph or summary of the entire indexed codebase     |
  * **Text Embedding Generation (Implementation Note)**: As an optional but often crucial step for advanced semantic search and relevance ranking, relevant textual content from InfoBlocks (e.g., docstrings, function signatures, natural language descriptions of code, summarized code logic) may be converted into dense vector embeddings using a pre-trained language model. These embeddings capture the semantic meaning of the code snippets.
  * **Indexing for Search and Querying**:
    * Creates optimized data structures (e.g., inverted indexes for text search, lookup tables for symbol names, graph databases or adjacency lists for relationship traversal) to allow fast and flexible searching and retrieval of InfoBlocks.
    * Queries can be based on names, types, textual content (using keyword or semantic search if embeddings are available), relationships (e.g., "find all callers of this function," "find all classes inheriting from this base class"), or tiers.
  * **Linking to Documentation and External Resources**:
    * (If applicable) Establishes links between code InfoBlocks and their corresponding external documentation, issue tracker items, or other relevant metadata.
* **Key Outputs (Conceptual)**: Tier-annotated InfoBlocks; (potentially) vector embeddings for semantic search; comprehensive and efficient searchable indexes of symbols, text, and relationships.

### 5. Watcher & Incremental Indexer

* **Objective**: To efficiently update all generated semantic models and indexes when source files within the monitored directory are created, modified, or deleted, ensuring the information remains current and accurate with minimal delay.
* **Process**:
  * Actively monitors the file system for any changes (creation, deletion, modification) to files and directories within the indexed scope.
  * Upon detecting a change:
    * **File Creation**: The new file undergoes the full indexing pipeline (Stages 1-4). The resulting InfoBlocks and semantic graph information are then integrated into the existing global models and indexes.
    * **File Deletion**: All InfoBlocks, semantic graph nodes/edges, and index entries originating from the deleted file are removed. Relationships in other parts of the codebase that depended on elements from the deleted file (e.g., call graph edges pointing to its functions) are updated or marked as unresolved.
    * **File Modification**:
      * Employs strategies to determine the extent of the change and minimize re-computation. This might involve comparing file hashes, tracking modification timestamps, or performing a quick diff.
      * For minor changes, it might be possible to re-parse and re-analyze only the modified sections of the file (if the parser and subsequent stages support fine-grained updates). For substantial changes, the entire file is typically re-processed through Stages 1-4.
      * The affected InfoBlocks, CFGs, call graph participations, symbol table entries, and search indexes are updated or replaced.
      * Changes are propagated: e.g., if a function's signature changes, call sites referencing it may need re-evaluation; if a class interface changes, derived classes or instantiations might be affected.
  * The core principle is to re-process only what is necessary to reflect the changes accurately, avoiding a full re-index of the entire directory for every small modification. This often involves caching intermediate results and intelligently managing dependencies between different pieces of indexed information.
* **Key Outputs (Conceptual)**: An up-to-date and consistent set of semantic models (IR, graphs) and searchable indexes that accurately reflect the current state of the codebase in the indexed directory.

## Items Excluded from Indexing

To maintain focus, relevance, and efficiency, the following items are generally **not** indexed by ScopeMux unless explicitly configured otherwise:

* Raw comments that are not directly associated with a specific, identifiable code entity (like a function, class, variable declaration, or significant control block). General file-header comments might be indexed at the file/module level, but isolated comment blocks without clear context are usually skipped.
* Code explicitly marked as inactive or "dead," such as blocks within `#if 0 ... #endif` in C/C++ or similar conditional compilation directives that permanently exclude code.
* Files and directories that are matched by standard ignore patterns (e.g., rules in `.gitignore`, editor-specific configuration files, common build output directories like `node_modules` or `build/`) unless an override explicitly includes them for indexing.
* Build artifacts themselves (e.g., object files (`.o`, `.obj`), compiled binaries, shared libraries, JAR files), vendored third-party dependencies (unless the goal is to index them as part of the project), and test-generated snapshots or fixtures.
* Inline temporary variables within function bodies, especially those with very limited scope and simple usage, unless they are integral to understanding complex expressions, data flow for critical operations, or are returned/passed elsewhere. The focus is on more persistent semantic elements.

***
