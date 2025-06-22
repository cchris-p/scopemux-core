# Parser Testing Framework

This package provides a framework for testing the ScopeMux parser by generating expected JSON output for test cases. It includes utilities for parsing source code, converting AST and CST nodes to canonical representations, and comparing results.

## Features

- Language-specific adapters for Python, C++, and more languages
- Converters for AST and CST nodes to canonical dictionary representations
- JSON generators for creating expected output files
- Memory management utilities to prevent leaks
- File and diff utilities for comparing and updating test files
- Command-line interface with the same features as the original script

## Directory Structure

```
core/parser_testing/
├── __init__.py               # Package initialization, imports, exports
├── cli.py                    # Command-line interface
├── converters/               # AST/CST node converters
│   ├── __init__.py
│   ├── ast_converter.py      # Convert AST nodes
│   ├── base_converter.py     # Base converter class
│   └── cst_converter.py      # Convert CST nodes
├── generators/               # Generate JSON from source code
│   ├── __init__.py
│   └── json_generator.py     # Generate JSON representations
├── languages/                # Language-specific adapters
│   ├── __init__.py           # Registry & factory
│   ├── base.py               # Base language adapter
│   ├── cpp_adapter.py        # C++ adapter
│   └── python_adapter.py     # Python adapter
└── utils/                    # Utility functions
    ├── __init__.py
    ├── diff_utils.py         # Diff generation and comparison
    ├── file_utils.py         # File operations
    └── memory_utils.py       # Memory management
```

## Usage

### As a Module

```python
from core.parser_testing.generators import JSONGenerator
from core.parser_testing.converters import ASTConverter, CSTConverter

# Create converters and generator
ast_converter = ASTConverter()
cst_converter = CSTConverter()
generator = JSONGenerator(ast_converter, cst_converter)

# Generate JSON representation from a source file
result = generator.generate_from_file("path/to/source.py", mode="both")

# Write the result to a file
from core.parser_testing.utils import write_json_file
write_json_file("path/to/output.json", result)
```

### From the Command Line

```bash
# Direct use of the CLI module
python -m core.parser_testing.cli path/to/source.py --mode both

# Or using the compatibility script (same as the original)
python core/tests/tools/generate_expected_json.py path/to/source.py --mode both
```

## Language-Specific Adapters

One of the key improvements in this refactored module is the addition of language-specific adapters. These adapters allow for custom handling of language-specific features:

- **Python Adapter**: Handles Python-specific features such as decorators, type hints, and async/await syntax.
- **C++ Adapter**: Handles C++-specific features such as templates, namespaces, and access specifiers.

The adapter system is extensible, allowing for the addition of more language adapters in the future.

## Memory Management

The original script had issues with memory management, particularly with the handling of CST nodes. The refactored module includes memory management utilities to help prevent memory leaks:

- `safe_process_with_gc()`: Execute a function with guaranteed garbage collection before and after.
- `clear_references()`: Clear references to objects to help garbage collection.
- `deep_cleanup()`: Recursively clean up an object and its attributes.

These utilities are used throughout the module to ensure proper cleanup of resources.

## Contributing

To add support for a new language, you need to:

1. Create a new adapter class that inherits from `LanguageAdapter`.
2. Implement the required methods:
   - `extract_signature()`: Extract function/method signatures
   - `generate_qualified_name()`: Generate qualified names for symbols
   - `process_special_cases()`: Handle language-specific features
   - `convert_ast_node()`: Convert AST nodes to canonical format
   - `convert_cst_node()`: Convert CST nodes to canonical format
3. Register the adapter in `languages/__init__.py`.
