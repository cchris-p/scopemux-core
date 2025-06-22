"""
Generators Package

This package provides generators for creating various representations
of parsed source code, such as JSON serializations of AST and CST structures.
"""

from .json_generator import JSONGenerator

__all__ = [
    'JSONGenerator',
]
