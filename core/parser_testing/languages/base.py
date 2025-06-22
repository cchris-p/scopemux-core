"""
Base Language Adapter Interface

This module defines the base class for language-specific adapters used in
parsing and converting source code to AST/CST representations.

Each language adapter handles language-specific parsing rules, qualified name
generation, and special case processing for a particular programming language.
"""

from typing import Dict, List, Any, Optional, Union


class LanguageAdapter:
    """
    Base class for language-specific adapters.

    Language adapters provide customized handling for different programming languages,
    allowing language-specific features to be properly represented in the AST/CST.
    """

    def __init__(self, language_type: str, language_name: str):
        """
        Initialize a language adapter.

        Args:
            language_type: The identifier for this language (e.g., "python", "c", "cpp")
            language_name: The human-readable name for this language (e.g., "Python", "C", "C++")
        """
        self.language_type = language_type
        self.language_name = language_name

    def extract_signature(self, node: Any, source_code: str) -> str:
        """
        Extract function/method signature from the node.

        Args:
            node: The raw node from which to extract a signature
            source_code: The source code text

        Returns:
            A string representing the function/method signature
        """
        return "() -> None"  # Default implementation

    def generate_qualified_name(self, name: str, parent_node: Optional[Any] = None) -> str:
        """
        Generate qualified name following language conventions.

        Args:
            name: The base name
            parent_node: The parent node, if any

        Returns:
            A string representing the qualified name
        """
        if not parent_node or not hasattr(parent_node, 'qualified_name') or not parent_node.qualified_name:
            return name
        return f"{parent_node.qualified_name}.{name}"

    def process_special_cases(self, node: Any, context: Any) -> None:
        """
        Handle language-specific constructs.

        Args:
            node: The node to process
            context: The parsing context
        """
        pass

    def convert_ast_node(self, raw_node: Any, context: Any) -> Dict[str, Any]:
        """
        Convert raw AST node to canonical format.

        Args:
            raw_node: The raw AST node from the parser
            context: The parsing context

        Returns:
            A dictionary representing the converted AST node
        """
        # Base implementation - to be overridden by subclasses
        result = {
            "type": "UNKNOWN",
            "name": "",
            "qualified_name": "",
            "children": []
        }
        return result

    def convert_cst_node(self, raw_node: Any, context: Any) -> Dict[str, Any]:
        """
        Convert raw CST node to canonical format.

        Args:
            raw_node: The raw CST node from the parser
            context: The parsing context

        Returns:
            A dictionary representing the converted CST node
        """
        # Base implementation - to be overridden by subclasses
        result = {
            "type": "UNKNOWN",
            "content": "",
            "range": {
                "start": {"line": 0, "column": 0},
                "end": {"line": 0, "column": 0}
            },
            "children": []
        }
        return result

    def get_language_specific_fields(self) -> List[str]:
        """
        Return additional fields specific to this language.

        Returns:
            A list of field names specific to this language
        """
        return []
