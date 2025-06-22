"""
Base Converter Interface

This module defines the base class for AST and CST converters, which transform
raw parser nodes into canonical dictionary representations.
"""

from typing import Dict, Any, Optional

from ..languages import LanguageAdapterRegistry, get_adapter


class BaseConverter:
    """
    Base class for node converters.

    Node converters transform raw AST/CST nodes from the parser into a
    canonical dictionary representation that can be serialized to JSON.
    """

    def __init__(self, language_adapter_registry: Optional[LanguageAdapterRegistry] = None):
        """
        Initialize a converter with a language adapter registry.

        Args:
            language_adapter_registry: The registry of language adapters to use
        """
        # Use the provided registry or the default one
        self.language_adapter_registry = language_adapter_registry or LanguageAdapterRegistry()

    def get_adapter_for_language(self, language_type: str):
        """
        Get the appropriate language adapter for the given language type.

        Args:
            language_type: The language type identifier

        Returns:
            The language adapter if found, None otherwise
        """
        return get_adapter(language_type)

    def convert(self, node: Any, language_type: str, context: Any = None) -> Dict[str, Any]:
        """
        Convert a node to a canonical dictionary representation.

        Args:
            node: The node to convert
            language_type: The language type
            context: Optional context information

        Returns:
            A dictionary representation of the node
        """
        # This is an abstract method that should be implemented by subclasses
        raise NotImplementedError("Subclasses must implement convert()")

    def _generic_convert(self, node: Any, context: Any = None) -> Dict[str, Any]:
        """
        Generic fallback conversion when no language adapter is available.

        Args:
            node: The node to convert
            context: Optional context information

        Returns:
            A dictionary representation of the node
        """
        # This is an abstract method that should be implemented by subclasses
        raise NotImplementedError(
            "Subclasses must implement _generic_convert()")
