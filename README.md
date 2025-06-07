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

# ScopeMux C Bindings Project

This directory contains C implementations for performance-critical components of the ScopeMux project, with Python bindings using pybind11.

## Project Overview

ScopeMux C Bindings provides high-performance implementations of:

1. **Parser + IR Generator** - Parses source code using Tree-sitter and generates a compact, binary Intermediate Representation (IR) for each function/class with metadata.

2. **Tiered Context Engine** - Manages a pool of InfoBlocks (functions, classes, doc chunks), estimates token costs, ranks blocks by relevance, and applies compression to fit within token budgets.

## Directory Structure (needs to be updated)

```
c-bindings/
├── CMakeLists.txt              # Main build configuration
├── include/
│   └── scopemux/               # Public API headers
│       ├── parser.h            # Parser and IR interfaces
│       ├── context_engine.h    # Context engine interfaces
│       ├── tree_sitter_integration.h # Tree-sitter integration
│       └── python_bindings.h   # Pybind11 interfaces
├── src/
│   ├── parser/                 # Parser implementation
│   │   ├── parser.c
│   │   ├── ir_generator.c
│   │   └── tree_sitter_integration.c
│   ├── context_engine/         # Context engine implementation
│   │   ├── compressor.c
│   │   ├── expander.c
│   │   └── token_budgeter.c
│   ├── common/                 # Shared utilities
│   │   ├── error_handling.c
│   │   ├── memory_management.c
│   │   └── logging.c
│   └── bindings/               # Python bindings
│       ├── module.c
│       ├── parser_bindings.c
│       ├── context_engine_bindings.c
│       └── tree_sitter_bindings.c
├── tests/                      # Unit tests
└── examples/                   # Example usage
```

## Build Instructions

```bash
# Create a build directory
mkdir build && cd build

# Configure
cmake ..

# Build
make

# Install
make install
```

## Usage

### Python Usage

```python
import scopemux_core

# Initialize the parser
parser = scopemux_core.ParserContext()

# Parse a file
parser.parse_file("example.py")

# Get functions
functions = parser.get_nodes_by_type(scopemux_core.NODE_FUNCTION)

# Initialize the context engine
engine = scopemux_core.ContextEngine()

# Add parser results to the context engine
engine.add_parser_context(parser)

# Rank blocks by cursor position
engine.rank_blocks("example.py", 10, 5)

# Compress to fit token budget
engine.compress()

# Get compressed context
context = engine.get_context()
print(context)
```

***

When adding new features:

1. Add corresponding header files in `include/scopemux/`
2. Implement in appropriate subdirectory under `src/`
3. Add Python bindings in `src/bindings/`
4. Update `CMakeLists.txt` to include new source files
5. Add tests in `tests/`
