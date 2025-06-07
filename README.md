# ScopeMux

**Purpose**:
`ScopeMux` is an **MCP (Modular Context Processor) server** that dynamically filters and compresses project context for LLMs, allowing intelligent integration with developer tools like Windsurf and Cursor.

**Key Features**:

* **Tiered Context System**: Controls how much of the code/project is exposed based on context tiers (e.g., line-level, file-level, module-level, project-level).
* **InfoBlocks**: Fine-grained chunks of project data that users or heuristics can selectively expose to LLMs.
* **Compression/Expansion**: Dynamically resizes project context to fit token constraints while retaining semantic relevance.
* **IDE Integration**: Responds to API calls from IDEs to serve the right amount of scoped context for a given task (e.g., code generation, explanation, test-writing).
* **Memory Embedding**: Uses a local vector store to embed InfoBlocks for fast similarity retrieval during queries.

**Use Case**:
Give LLMs the *right amount* of compressed project context for any given task without overloading or underinforming them.

## ScopeMux Indexing & Tiered Contexts – FAQ

| # | Question | Answer |
| --- | --- | --- |
| **1** | **What is the overall pipeline?** | `Tree-Sitter` → **IR generator (C core)** → **InfoBlocks** → user-composable **Tiered Contexts** (e.g., “all docstrings”, “activity diagram for payment flow”). |
| **2** | **Why Tree-Sitter for parsing?** | It offers fast, incremental, multi-language parsing with robust C API bindings—ideal for the C core that feeds Python bindings. |
| **3** | **Which IR(s) will we start with?** | *Symbol IR* (defs/uses), *Call-Graph IR*, *Activity IR* (CFG-to-UML mapping). Others can be added later (Data-Flow IR, Import Graph, etc.). |
| **4** | **What qualifies as an InfoBlock?** | Any semantically coherent slice: one class, one function, an entire file, or an ad-hoc bundle (e.g., `email_*` helpers from three files). |
| **5** | **Examples of Tiered Contexts we plan to support?** | *All docstrings + func names*, *UML class diagram per pkg*, *Activity diagram of critical path*, *Interface map for a directory*, *TLA⁺ spec template* (future). |
| **6** | **Will tier assignment be automatic or manual?** | **Automatic** at first (rules on size, nesting, relevance); later we’ll expose user overrides and scoring (complexity, recency, usage). |
| **7** | **Static vs dynamic information?** | Both: static parsing for structure; dynamic hooks (e.g., execution traces) can annotate or refine IR and Tiered Contexts. |
| **8** | **Storage & search strategy?** | Batch builds → store structured IR in **Postgres**; embeddings for fast similarity/go-to-definition stored in **pgvector**. |
| **9** | **Directory meaning—how is hierarchy handled?** | Directories are treated as **semantic containers**; path segments become nodes in the Import/Dependency IR and boundaries for Tier 2 contexts. |
| **10** | **Scaling plan for large monorepos?** | Start with batch indexing on medium repos; later add sharding + incremental rebuilds so even 10 k+-file monorepos stay responsive. |
| **11** | **Why consider TLA⁺ as a Tiered Context?** | TLA⁺ specs let LLMs reason formally about system behavior; emitting TLA⁺ templates from IR unlocks validation, scaffolding, and test-case synthesis. |
| **12** | **Immediate C-core priorities?** | • Expose **parse → IR → InfoBlock** pipeline through Python bindings.  • Implement Symbol & Call-Graph IR emitters.  • Basic Tiered Context builder (auto tiers 0–3). |
| **13** | **Future enhancements?** | • Incremental indexing.  • Heuristic score-based tier resizing.  • Live IDE lenses that let users click-assemble custom Tiered Contexts on demand. |

> Tip: Treat Tiered Contexts as views over your IR/InfoBlock graph—just like SQL views—so you can add new forms (e.g., “only public APIs”, “business-rule English summaries”) without touching the underlying index.

***

# FastAPI MCP Server (Very High Level)

I am keeping this at a very high level because I have not yet scoped out the details of the scopemux-core bindings.

Goal: Build an FastAPI MCP server that:

* Connects with modern IDEs (like Windsurf, Cursor)
* Compresses/uncompresses project context intelligently
* Exposes only the **most relevant** parts of a project to LLMs
* Supports multiple **tiers** of context granularity and fallback strategies

Some API might look like:

* `read_docstrings`
* `read_function_names`
* `select_function_names`

## 🗂️ Tiered Context Model

| Tier | Content | Compression | Use Case |
| --- | --- | --- | --- |
| **T0** | File snippets (e.g., 20-line window) | Minimal | Real-time completions |
| **T1** | Full active file, collapsed docstrings/comments | Light | Code summarization |
| **T2** | Dependencies + current file | AST/deduplication | Refactoring |
| **T3** | Cross-project context, e.g., all entrypoints | Aggressive | Bug triage, architecture review |
| **T4** | Whole project, condensed | Embedding or chunk indexing | Retrieval-augmented prompting |

We can build encoders for each tier, and the MCP server can expose an endpoint like:

```
GET /context?tier=T2&file=src/foo.py&cursor=1234

```

## ⚙️ Architecture Outline

### 1. **MCP Server (Core)**

* Written in Python, Node.js, or Rust
* Accepts file + cursor context
* Applies tier-specific compressors
* Returns a transformed prompt (for OpenAI, Claude, etc.)

### 2. **Tier Encoders**

Each tier will have its own strategy:

* Token budget enforcement (truncate, collapse, etc.)
* AST and symbol-based filtering
* Graph-based dependency resolution
* Embedding-based relevance filtering

### 3. **IDE Integration Adapter**

For Windsurf and Cursor:

* Use their plugin APIs or Language Server Protocol (LSP)
* Hook into events like "onSave", "onCursorMove", etc.
* Stream context updates to the MCP server

***

## 🧪 Development Plan

1. **Define Tiering Strategy**
   * Establish what content each tier includes and how it's derived
2. **Implement Compression Techniques**
   * Lightweight: Token truncation, comment stripping
   * Medium: AST pruning, identifier preservation
   * Heavy: Embedding-based selection + semantic deduplication
3. **Build MCP API**
   * `/context` for context expansion
   * `/suggest` for full LLM pipeline if desired
4. **IDE Plugin (Start with Cursor)**
   * React or Typescript plugin to communicate with MCP
   * Trigger updates and forward editor state

***

## 🧰 Libraries & Tools

* **Tree-sitter** for AST parsing and incremental syntax trees
* **tiktoken** (or Claude tokenizer) for token counting
* **Langchain/LLM Engine** for retrieval + prompt formatting
* [\*\*Socket.io](http://socket.io/) or WebSockets\*\* for real-time IDE-server comms
* **SQLite** or in-memory graph for file/module relationships

***

## 🔁 Example Workflow

1. User selects a function in `foo.py`

2. Cursor IDE sends:

   ```json
   {
     "file": "src/foo.py",
     "cursor": 215,
     "tier": "T2"
   }

   ```

3. MCP server:
   * Parses `foo.py`
   * Identifies imports/dependencies
   * Summarizes docstrings/comments
   * Returns context of ~4k tokens

4. Cursor displays context-aware completions or diagnostics
