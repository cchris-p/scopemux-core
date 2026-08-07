# ScopeMux Core

`scopemux-core` is the native parsing and context-compression engine behind ScopeMux.

It provides:
- source parsing through Tree-sitter
- AST and CST access from Python
- multi-language project context extraction
- context ranking and compression for LLM workflows

## Current Scope

The current codebase is aimed at local development and experimental integration rather than polished package distribution.

Supported language coverage in the repository today includes:
- C
- C++
- Python
- JavaScript
- TypeScript

## Repository Layout

Key directories:
- `core/`: C sources, headers, Python bindings, and tests
- `scripts/`: build and test entry points
- `vendor/`: Tree-sitter grammars and `pybind11`
- `queries/`: Tree-sitter query files

## Prerequisites

Required:
- Python `3.10` or `3.11`
- CMake `3.16+`
- a C and C++ toolchain
- `make`
- git submodule support

For C test runs:
- `libcriterion-dev`

Example on Debian or Ubuntu:

```bash
sudo apt-get update
sudo apt-get install build-essential cmake python3-dev python3-pip libcriterion-dev
```

## Clone And Bootstrap

Clone the repository and initialize submodules:

```bash
git clone <repo-url>
cd scopemux-core
git submodule update --init --recursive
```

The repository stores Tree-sitter grammars and `pybind11` as submodules under `vendor/`.

## Build Python Bindings

The simplest supported path is:

```bash
./scripts/build_all_and_pybind.sh
```

This script:
- installs `scopemux_core` as an editable dependency from `./core`
- drives the native build through `core/pyproject.toml`
- builds the `scopemux_core` Python extension without manual `PYTHONPATH` setup

You can also run the editable install directly:

```bash
python3 -m pip install -e ./core
```

## Run Tests

Common test entry points:
- `./scripts/run_c_tests.sh`
- `./scripts/run_cpp_tests.sh`
- `./scripts/run_python_tests.sh`
- `./scripts/run_js_tests.sh`
- `./scripts/run_ts_tests.sh`
- `./scripts/run_all_lang_tests.sh`

These scripts create separate build directories for their own runs.

## Python Usage

Basic file parsing:

```python
import scopemux_core as sm

parser = sm.ParserContext()
parser.parse_file("example.py")

ast_root = parser.get_ast_root()
print(ast_root)
```

Parsing a string with explicit language selection:

```python
import scopemux_core as sm

parser = sm.ParserContext()
parser.parse_string(
    "def add(a, b):\n    return a + b\n",
    filename="example.py",
    language="python",
)

print(parser.get_ast_root())
```

Building compressed context:

```python
import scopemux_core as sm

parser = sm.ParserContext()
parser.parse_file("example.py")

engine = sm.ContextEngine({"max_tokens": 512})
engine.add_parser_context(parser)
engine.rank_blocks("example.py", 1, 1, query="summarize the main logic")
engine.compress()

print(engine.get_context())
```

## Exposed Python API

Main types currently exposed by the extension:
- `ParserContext`
- `ContextEngine`

Common `ParserContext` methods:
- `parse_file(filename, language=None)`
- `parse_string(content, filename=None, language=None)`
- `get_ast_root()`
- `get_cst_root()`
- `get_last_error()`

Common `ContextEngine` methods:
- `add_parser_context(parser_ctx)`
- `rank_blocks(cursor_file, cursor_line, cursor_column, query=None)`
- `compress()`
- `get_context()`
- `estimate_tokens(text)`
- `update_focus(node_qualified_names, focus_value)`
- `reset_compression()`

## Limitations

Current limitations to expect:
- the build flow is development-oriented and not yet packaged as a standard `pip install` experience
- supported APIs are still evolving
- some repository scripts and test utilities are intended for internal validation rather than end-user workflows

## Troubleshooting

If the Python module does not import:

1. Confirm submodules are initialized:

```bash
git submodule update --init --recursive
```

2. Rebuild the extension:

```bash
./scripts/build_all_and_pybind.sh
```

3. Ensure Python is loading the local build artifact:

```bash
PYTHONPATH="$(pwd)/build/core" python3 -c "import scopemux_core; print(scopemux_core.__file__)"
```

If test builds fail, verify that `libcriterion-dev` is installed.
