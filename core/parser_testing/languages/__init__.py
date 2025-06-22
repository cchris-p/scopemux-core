"""
Language Adapter Registry

This module provides a registry of language adapters that can be used for
parsing and converting source code in different programming languages.
"""

from typing import Dict, Optional, Type, List

from .base import LanguageAdapter
from .python_adapter import PythonAdapter
from .cpp_adapter import CppAdapter


class LanguageAdapterRegistry:
    """
    Registry for language adapters.

    This class manages the registration and retrieval of language adapters,
    which are used to handle language-specific features during parsing
    and conversion of source code.
    """

    def __init__(self):
        """Initialize the language adapter registry."""
        self._adapters: Dict[str, LanguageAdapter] = {}

        # Register built-in adapters
        self.register_builtin_adapters()

    def register_adapter(self, adapter: LanguageAdapter) -> None:
        """
        Register a language adapter in the registry.

        Args:
            adapter: The language adapter to register
        """
        self._adapters[adapter.language_type] = adapter

    def get_adapter(self, language_type: str) -> Optional[LanguageAdapter]:
        """
        Get a language adapter by its language type.

        Args:
            language_type: The language type identifier

        Returns:
            The language adapter if found, None otherwise
        """
        return self._adapters.get(language_type)

    def get_adapter_by_file_extension(self, file_path: str) -> Optional[LanguageAdapter]:
        """
        Get a language adapter based on file extension.

        Args:
            file_path: The path to the file

        Returns:
            The language adapter if found, None otherwise
        """
        extension = file_path.lower().split(
            '.')[-1] if '.' in file_path else ''

        # Map file extensions to language types
        extension_map = {
            'py': 'python',
            'cpp': 'cpp',
            'cc': 'cpp',
            'cxx': 'cpp',
            'h': 'cpp',
            'hpp': 'cpp',
            'c': 'c',
            'js': 'javascript',
            'ts': 'typescript'
        }

        language_type = extension_map.get(extension)
        if language_type:
            return self.get_adapter(language_type)

        return None

    def register_builtin_adapters(self) -> None:
        """Register all built-in language adapters."""
        self.register_adapter(PythonAdapter())
        self.register_adapter(CppAdapter())

        # Note: JavaScript and TypeScript adapters will be registered
        # once they are implemented

    def get_available_languages(self) -> List[str]:
        """
        Get a list of available language names.

        Returns:
            A list of available language names
        """
        return [adapter.language_name for adapter in self._adapters.values()]


# Create a singleton instance
registry = LanguageAdapterRegistry()


# Helper functions
def get_adapter(language_type: str) -> Optional[LanguageAdapter]:
    """
    Get a language adapter by its language type.

    Args:
        language_type: The language type identifier

    Returns:
        The language adapter if found, None otherwise
    """
    return registry.get_adapter(language_type)


def get_adapter_by_file_extension(file_path: str) -> Optional[LanguageAdapter]:
    """
    Get a language adapter based on file extension.

    Args:
        file_path: The path to the file

    Returns:
        The language adapter if found, None otherwise
    """
    return registry.get_adapter_by_file_extension(file_path)


def get_available_languages() -> List[str]:
    """
    Get a list of available language names.

    Returns:
        A list of available language names
    """
    return registry.get_available_languages()
