"""
Parser Testing Framework

This package provides a framework for testing the ScopeMux parser by generating
expected JSON output for test cases. It includes utilities for parsing source code,
converting AST and CST nodes to canonical representations, and comparing results.

The framework is organized into several subpackages:
- converters: For converting AST/CST nodes to canonical dictionary representations
- generators: For generating JSON representations from source code
- languages: For language-specific adapters to handle different programming languages
- utils: For utility functions like file operations and memory management

Usage:
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

Or simply use the CLI:
    python -m core.parser_testing.cli path/to/source.py --mode both
"""

__version__ = "1.0.0"

from .converters import ASTConverter, CSTConverter
from .generators import JSONGenerator
from .languages import (
    LanguageAdapter,
    LanguageAdapterRegistry,
    get_adapter,
    get_adapter_by_file_extension,
    get_available_languages,
)

__all__ = [
    # Main classes
    'ASTConverter',
    'CSTConverter',
    'JSONGenerator',
    'LanguageAdapter',
    'LanguageAdapterRegistry',

    # Helper functions
    'get_adapter',
    'get_adapter_by_file_extension',
    'get_available_languages',
]
