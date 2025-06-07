# ScopeMux C Bindings

This directory contains C implementations for performance-critical components of the ScopeMux project, with Python bindings using pybind11.

## TODO - Building the Minimal Directory Indexing

* \[\_] Implement Parse file into CST (Concrete Syntax Tree)
* \[\_] Implement Convert CST to AST (Abstract Syntax Tree)
* \[\_] Implement Generate IR(s) from AST

## TODO - Building the UML Semantic Models

* \[\_] Implement Build Control Flow Graph (CFG)
* \[\_] Implement Build Call Graph
* \[\_] Implement Build Activity Diagram / Data Flow Graph

## TODO - Add Support For InfoBlocks

* \[\_] Implement Extract Symbol Table (defs, uses, scope)
* \[\_] Implement Extract Docstrings and Comments

## TODO - For Later

* \[\_] Implement Generate Tiered Contexts
* \[\_] Implement Embed InfoBlocks for semantic search
* \[\_] Implement Apply Change Tracking (diff-aware IR)
* \[\_] Implement Perform Relevance Ranking or Scoring
* \[\_] Implement Visualize (e.g., UML, dependency diagrams)
* \[\_] Implement Store in Context Database / Vector Index
* \[\_] Implement Serve via API or MCP interface

### Phase 1: Parser + IR Generator

* Implement Tree-sitter integration
* Build AST traversal and IR generation
* Support C++ and Python code parsing

### Phase 2: Context Engine

* Implement token budget management
* Create block ranking algorithms
* Develop compression strategies

### Future Components

* Graph Analysis Engine (CFG, Call Graph)
* Vector Search / Semantic Search
* Metadata + Change Tracking
* FlatBuffer Serialization
* API Daemon Core

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
