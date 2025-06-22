"""
Utilities Package

This package provides utility functions for file operations, memory management,
and diff generation for the parser testing framework.
"""

from .file_utils import (
    read_file,
    write_file,
    read_json_file,
    write_json_file,
    compare_files,
    get_expected_output_path,
    find_source_files,
)

from .memory_utils import (
    safe_process_with_gc,
    clear_references,
    deep_cleanup,
)

from .diff_utils import (
    generate_json_diff,
    highlight_diff,
    compare_ast_nodes,
    format_differences,
)

__all__ = [
    # File utilities
    'read_file',
    'write_file',
    'read_json_file',
    'write_json_file',
    'compare_files',
    'get_expected_output_path',
    'find_source_files',

    # Memory utilities
    'safe_process_with_gc',
    'clear_references',
    'deep_cleanup',

    # Diff utilities
    'generate_json_diff',
    'highlight_diff',
    'compare_ast_nodes',
    'format_differences',
]
