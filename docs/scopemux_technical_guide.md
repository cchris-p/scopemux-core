# ScopeMux Technical Guide

This document outlines the technical architecture, implementation details, and design principles for the ScopeMux codebase analysis system.

## Table of Contents

1. [Indexing Strategy](#indexing-strategy)
2. [C/C++ Core Implementation](#cc-core-implementation)
3. [Architecture Boundary](#architecture-boundary)

---

<a name="indexing-strategy"></a>
## 1. Indexing Strategy

The goal of the context collector is to enable fast, intelligent interaction with a large codebase—especially for LLMs, search, summarization, and tooling. To that end, you should index only the *relevant, extractable, and useful* parts of the codebase that provide **semantic structure** and **operational intent**.

Here's what should be indexed:

### ✅ **1. Function and Method Definitions**

* **Signature**
* **Start/end byte and line offsets**
* **Parameters and return types**
* **Enclosing class (if applicable)**
* **Modifiers** (static, const, inline, etc.)
* **Comments and docstrings above the function**

🔧 *Why:* Functions are the smallest reusable, callable units. They're where logic lives and where questions often point (e.g., "What does `apply_mask()` do?").

### ✅ **2. Class / Struct Definitions**

* **Name and location**
* **Inheritance / interfaces**
* **Member variables and types**
* **Access modifiers (public/private/protected)**
* **All method declarations**
* **Docstrings or comments at the class level**

🔧 *Why:* Classes are containers of behavior and structure. They define relationships and patterns in OO systems.

### ✅ **3. Global Constants and Macros**

* `#define`, `constexpr`, `const T name = ...`
* Capture their **values**, **types**, **comments**, and **location**

🔧 *Why:* These often encode fixed configuration values or key invariants used across multiple files.

### ✅ **4. Enums and Typedefs**

* Name, underlying type, member values
* Typedefs and `using` aliases

🔧 *Why:* These establish program-specific types and can improve LLM understanding of type-dependent logic.

### ✅ **5. Include / Import Statements**

* `#include`, `import`, `use`, etc.
* Resolve relative includes to absolute paths when possible

🔧 *Why:* Builds the **include graph** or **module dependency tree**, which is essential for cross-file context building.

### ✅ **6. Comments & Docstrings**

* Per-function/class leading comments
* Inline comments for CFG edge annotation
* TODO/FIXME tags with locations

🔧 *Why:* These give intent, usage semantics, and edge cases that are hard to infer from code alone.

### ✅ **7. Control Flow Graph (CFG)**

For each function:

* Nodes = basic blocks
* Edges = branches (`if`, `while`, `switch`)
* Labelled with condition expressions and loop types

🔧 *Why:* This powers activity diagram generation, info-block summarization, and token budget-aware compression.

### ✅ **8. Call Graph Edges**

* Function A → Function B (callsite recorded)
* Capture argument structure if possible

🔧 *Why:* Helps upstream/downstream traceability, especially in "impact analysis" use cases.

### ✅ **9. Language-Specific Entities**

Examples:

* **Python**: decorators, `async def`, `@dataclass`
* **C++**: templates, virtual methods, friend classes
* **Rust**: traits, impl blocks, lifetimes
* **JavaScript**: closures, arrow functions, module exports

🔧 *Why:* These add nuance to how behavior is defined and invoked.

### 🚫 **What NOT to Index**

* Raw comments that aren't near a code entity
* `#if 0`/dead code blocks
* Code inside `.gitignore`d files (unless explicitly allowed)
* Build artifacts, vendored dependencies, test snapshots
* Inline temporary variables unless used in expressions

### Bonus: Tier Assignment

You can also assign **tiers** during indexing to aid later compression:

| Tier | Meaning         | Examples                           |
| ---- | --------------- | ---------------------------------- |
| 0    | Line-level      | Single token span or variable      |
| 1    | Function/method | A single named callable unit       |
| 2    | File            | All symbols within a file          |
| 3    | Module / Folder | Package or directory scope         |
| 4    | Project-wide    | Top-level graph of the entire repo |

---

<a name="cc-core-implementation"></a>
## 2. C/C++ Core Implementation

Expanding the **functional logic covered by C** in your ScopeMux project is a strategic performance upgrade, especially for workloads that are:

* **CPU-bound** (e.g. AST parsing, graph building)
* **low-latency-critical** (e.g. compressing context on-the-fly)
* **memory-sensitive** (e.g. working with token budgets or multi-GB repos)

Below is a list of **performance-critical areas**, how to implement them in C, and what you gain by doing so—along with the recommended **expansion stages**:

### 🔧 1. **Parser + IR Generator**

#### What to implement:

* Fully own the Tree-sitter integration in C
* Emit a compact, binary IR per function/class with:
  * Function signature
  * Line/byte ranges
  * Control-flow primitives
  * Docstrings and comments
  * Call expressions and dependencies

#### Why:

* Tree-sitter is already C; removing Python wrappers saves GC cost, marshalling time
* Enables **zero-copy IR transfer** to other stages (graph, embed)

### 🧠 2. **Graph Analysis Engine (CFG, Call Graph, Activity Flow)**

#### What to implement:

* Build a custom C graph engine that:
  * Converts AST to control-flow graphs (CFG)
  * Optionally converts OO call chains to *interaction graphs* or *sequence diagrams*
  * Supports adjacency lists and fast neighbor traversal

#### Why:

* Graph construction is O(n²) on function bodies—must be fast
* Python graph libraries (e.g. NetworkX) are too slow
* Enables real-time slicing and tiered-context proximity scans

### 🧊 3. **Tiered Context Engine (Compressor/Expander)**

#### What to implement:

* Write C code to:
  * Take a pool of InfoBlocks (functions, classes, doc chunks)
  * Estimate token cost (e.g. use pre-compiled tiktoken rules)
  * Rank blocks by relevance: recency, cursor proximity, semantic similarity
  * Apply a greedy compression until the context fits token budget

#### Why:

* CPU-bound and latency-sensitive: you want <50ms response time
* Ranking and filtering scales poorly in Python at large repo sizes

### 🧲 4. **Semantic Search / Vector Retrieval**

#### What to implement:

* Wrap `pgvector` or a native ANN index (like `faiss` or `hnswlib`) directly in C
* Use SIMD or GPU-based cosine similarity scoring

#### Why:

* Vector search is latency-sensitive when filtering and scoring >1k candidates
* Bypassing Python cuts down response times by 50–80%

### 🧮 5. **Text Embedding Pipeline (optional)**

#### What to implement:

* Use a C++ wrapper (e.g. GGML) or CUDA kernel to embed functions/comments locally
* Expose as:

  ```c
  float* embed_text(const char* utf8_input);
  ```

#### Why:

* Batch embedding in C avoids overhead of subprocess or Python LLM clients
* Keeps your stack fully in-process if latency or offline mode matters

### 🧠 6. **Metadata + Change Tracking (Hasher / Diff Cache)**

#### What to implement:

* Track file hashes (e.g. `xxh3`), byte-offset changes, or line diffs in C
* Detect when AST or CFG materially changed → skip redundant indexing

#### Why:

* Avoiding redundant re-parses is critical for repos >1000 files
* Gives \~5–10× improvement in watch mode performance

### 📦 7. **FlatBuffer / Cap'n Proto Serialization**

#### What to implement:

* Generate and parse schema-verified IR blobs
* Pipe IR between modules (e.g. parser → graph → compressor) using `mmap` or in-memory ring buffers

#### Why:

* Eliminates JSON/Protobuf overhead
* Enables direct memory access from VS Code plugin if needed

### 🔌 8. **API Daemon Core (Threadpool, Dispatch, Memory Management)**

#### What to implement:

* C core manages:
  * Threadpools for concurrent parse/index
  * Event queues for debounce/batching
  * Memory arenas for temp IR/graph allocations
  * Lifecycle of LRU caches (e.g. recent files, expanded graphs)

#### Why:

* Centralizing memory/thread logic in C gives consistent performance under load
* Avoids Python GIL, improves concurrency scalability (esp. on multi-core dev servers)

### 🪞 9. **VS Code Ext Local Accelerator (Client-Side Cache + WASM)**

#### What to implement:

* Compile IR-handling C code to **WASM** for use inside VS Code CLI ext
* Allow:
  * Instant preview of compressed context
  * Background indexing
  * Embedded `scope query --expr="nearest_func(mutex)"`

#### Why:

* Enables blazing-fast "peek definition", "shrink this buffer" without RPC
* Great for remote or low-latency environments

### 📈 Implementation Priority Order (Performance ROI)

| Priority | Component                 | Reason                                     |
| -------- | ------------------------- | ------------------------------------------ |
| ✅ #1     | Parser + IR Generator     | High call frequency, no GC, huge speed win |
| ✅ #2     | Compressor / Tiered Logic | Direct impact on every LLM call            |
| ✅ #3     | Graph Engine              | Enables advanced context slicing           |
| ✅ #4     | Vector Search             | Bottleneck under large repos               |
| ✅ #5     | File Change Cache         | Avoids unnecessary work                    |
| ✅ #6     | Embedding Engine          | Optional, only if doing local embeddings   |
| ✅ #7     | FlatBuffer Transfer       | Enables clean multi-module comms           |
| ✅ #8     | Threaded Daemon Core      | Improves scalability                       |
| ✅ #9     | WASM Ext Accelerator      | High polish, useful for remote devs        |

---

<a name="architecture-boundary"></a>
## 3. Architecture Boundary

```
+---------------------------+
|         FastAPI          |  ← thin Python layer
+------------+-------------+
             |
       Pybind (nanobind)
             |
+---------------------------+
|      C / C++ Core         |
|                           |
|  - File Watcher           |
|  - Tree-sitter IR         |
|  - CFG / Diagrams         |
|  - Token Budgeter         |
|  - Compressor / Expander  |
|  - Vector Search (optional)|
|  - Graph Engine           |
|  - Thread Pool + Caches   |
+---------------------------+
```
