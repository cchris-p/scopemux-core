# TODO - General

## Building the Minimal Directory Indexing

* \[\_] Implement Parse file into CST (Concrete Syntax Tree)
* \[\_] Implement Convert CST to AST (Abstract Syntax Tree)
* \[\_] Implement Generate IR(s) from AST
* \[\_] Implement Tree-sitter integration
* \[\_] Implement Build AST traversal and IR generation
* \[\_] Implement Support C++ and Python code parsing

## Building the UML Semantic Models

* \[\_] Implement Build Control Flow Graph (CFG)
* \[\_] Implement Build Call Graph
* \[\_] Implement Build Activity Diagram / Data Flow Graph

## Add Support For InfoBlocks

* \[\_] Implement Extract Symbol Table (defs, uses, scope)
* \[\_] Implement Extract Docstrings and Comments

## Add Support For Tiered Contexts

* \[\_] Implement Generate Tiered Contexts
* \[\_] Implement Embed InfoBlocks for semantic search
* \[\_] Implement Apply Change Tracking (diff-aware IR)
* \[\_] Implement Perform Relevance Ranking or Scoring
* \[\_] Implement Visualize (e.g., UML, dependency diagrams)
* \[\_] Implement Store in Context Database / Vector Index
* \[\_] Implement Serve via API or MCP interface

## FastAPI Server

* \[\_] Implement FastAPI Server

### Tiered Context Engine

* \[\_] Implement token budget management
* \[\_] Implement Create block ranking algorithms
* \[\_] Implement Develop compression strategies

## Misc

* \[\_] Implement Graph Analysis Engine (CFG, Call Graph)
* \[\_] Implement Vector Search / Semantic Search
* \[\_] Implement Metadata + Change Tracking
* \[\_] Implement FlatBuffer Serialization
* \[\_] Implement API Daemon Core

# Tasks Derived from Design Documents

### Core Indexing Process (from 'How Should Each Directory Be Indexed.md')

* \[\_] Implement Stage 1: Source Code Ingestion & Syntactic Parsing
  * \[\_] Implement file identification and filtering (respecting .gitignore, language-specific files)
  * \[\_] Ensure parsers (e.g., Tree-sitter) generate ASTs with precise positional (line/byte offset) information.
* \[\_] Implement Stage 2: Semantic Elaboration & IR Construction
  * \[\_] Develop comprehensive extraction for function/method details (signatures, scope, modifiers, associated docs).
  * \[\_] Develop comprehensive extraction for class/struct/interface details (inheritance, members, methods, associated docs).
  * \[\_] Develop comprehensive extraction for globals, constants, macros, enums, typedefs.
  * \[\_] Implement robust parsing of import/include statements and path resolution for module dependency graph.
  * \[\_] Implement system for recognizing and creating IRs for language-specific entities (Python, C++, JS/TS, C, etc.).
  * \[\_] Standardize transformation of semantic info into InfoBlocks.
* \[\_] Implement Stage 3: Semantic Graph Construction & Deep Code Understanding
  * \[\_] Enhance Symbol Table generation with full resolution and scope analysis.
  * \[\_] Implement detailed Data Flow Analysis.
* \[\_] Implement Stage 4: Contextual Indexing & Tiered Context Preparation
  * \[\_] Implement explicit Tier Assignment to InfoBlocks (Tiers 0-4).
  * \[\_] Implement generation of Text Embeddings for InfoBlocks for semantic search.
  * \[\_] Design and implement optimized data structures for multifaceted search and querying of InfoBlocks.
  * \[\_] Implement functionality to link InfoBlocks to external documentation and resources.
* \[\_] Implement Stage 5: Watcher & Incremental Indexer
  * \[\_] Implement file system monitoring for changes (create, delete, modify).
  * \[\_] Develop robust incremental indexing logic (re-process only necessary parts, propagate changes).
  * \[\_] Implement caching strategies for intermediate results in incremental indexing.

### Specific IR Implementations (from 'Supported IRs.md' and others)

* \[\_] Implement Import/Dependency IR (module relationships).
* \[\_] Implement Memory/Object Lifetime IR.
* \[\_] Implement Concurrency IR (threads, async, locks).
* \[\_] Implement Notebook Execution IR (for .ipynb files, cell order, state).
* \[\_] Implement Type IR (inferred/declared types for symbols).
* \[\_] Implement Exception Flow IR (raise/catch analysis).
* \[\_] Implement Semantic Summary IR (potentially LLM-assisted natural language summaries).
* \[\_] Implement Contract IR (pre/post-conditions, invariants).
* \[\_] Implement Regulatory Compliance IR.
* \[\_] Implement Error Annotation IR (linking errors to IR nodes).
* \[\_] Implement Edit History IR (refine from "Apply Change Tracking").
* \[\_] Implement Execution Trace IR (runtime data linked to static IRs).
* \[\_] Implement Test Coverage IR (linking tests to code units).

### Tiered Contexts & User Configuration (from various docs)

* \[\_] Allow user definition of Tiered Contexts via a configuration file.
* \[\_] Implement automation for Tiered Context generation and suggestion.
* \[\_] Allow user configuration for selecting modules/files eligible for semantic modeling and diagram generation.

### Advanced Features (from 'User-Defined IRs.md')

* \[\_] Implement an agent to map natural language requests to relevant InfoBlocks.
* \[\_] Implement a system for storing and reusing InfoBlock templates (with variable omission).

# Ideas

*
