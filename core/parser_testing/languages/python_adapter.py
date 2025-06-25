"""
Python Language Adapter

This module implements a language adapter for Python source code,
handling Python-specific features and conversions.
"""

from typing import Dict, List, Any, Optional

from .base import LanguageAdapter


class PythonAdapter(LanguageAdapter):
    """
    Language adapter for Python source code.

    Handles Python-specific features such as:
    - Decorators
    - Type hints in function signatures
    - Docstrings
    - List/dict/set comprehensions
    - Async/await syntax
    """

    def __init__(self):
        """Initialize the Python language adapter."""
        super().__init__("python", "Python")

    def extract_signature(self, node: Any, source_code: str) -> str:
        """
        Extract a Python function signature including type hints if available.

        Args:
            node: The function node
            source_code: The source code text

        Returns:
            A string representing the function signature
        """
        # If we don't have access to raw signature extraction,
        # try to construct from function name and parameters
        if hasattr(node, "get_name") and callable(getattr(node, "get_name")):
            name = node.get_name() or "unknown"

            # Extract parameters if available
            parameters = []
            if hasattr(node, "get_parameters") and callable(getattr(node, "get_parameters")):
                param_list = node.get_parameters() or []
                for param in param_list:
                    param_str = param.get_name() if hasattr(param, "get_name") else "unknown"
                    param_type = param.get_type() if hasattr(param, "get_type") else None
                    param_default = param.get_default() if hasattr(param, "get_default") else None

                    if param_type:
                        param_str = f"{param_str}: {param_type}"
                    if param_default:
                        param_str = f"{param_str}={param_default}"

                    parameters.append(param_str)

            # Get return type if available
            return_type = ""
            if hasattr(node, "get_return_type") and callable(getattr(node, "get_return_type")):
                rt = node.get_return_type()
                if rt:
                    return_type = f" -> {rt}"

            signature = f"def {name}({', '.join(parameters)}){return_type}"
            return signature

        # Default fallback
        return "def unknown()"

    def process_special_cases(self, node: Any, context: Any) -> None:
        """
        Handle Python-specific features.

        Args:
            node: The node to process
            context: The parsing context
        """
        # Add handling for decorators
        if hasattr(node, "raw_data") and isinstance(getattr(node, "raw_data"), dict):
            raw_data = node.raw_data

            # Process decorators
            if "decorators" in raw_data and node.type in ("FUNCTION", "CLASS", "METHOD"):
                if not hasattr(node, "metadata"):
                    node.metadata = {}

                node.metadata["decorators"] = [
                    {
                        "name": decorator.get("name", "unknown"),
                        "arguments": decorator.get("arguments", [])
                    }
                    for decorator in raw_data["decorators"]
                ]

            # Process async functions
            if "is_async" in raw_data and raw_data["is_async"] and node.type == "FUNCTION":
                if not hasattr(node, "metadata"):
                    node.metadata = {}
                node.metadata["is_async"] = True

                # Update signature if needed
                if hasattr(node, "signature") and node.signature:
                    if not node.signature.startswith("async "):
                        node.signature = f"async {node.signature}"

    def convert_ast_node(self, raw_node: Any, context: Any) -> Dict[str, Any]:
        """
        Convert Python AST node to canonical format.

        Args:
            raw_node: The raw AST node from the parser
            context: The parsing context

        Returns:
            A dictionary representing the converted AST node
        """
        # Start with base implementation
        result = super().convert_ast_node(raw_node, context)

        # Add Python-specific fields
        if hasattr(raw_node, "get_type") and callable(getattr(raw_node, "get_type")):
            node_type = raw_node.get_type()
            if node_type:
                result["type"] = node_type

        # Handle docstrings specially in Python
        if hasattr(raw_node, "get_docstring") and callable(getattr(raw_node, "get_docstring")):
            docstring = raw_node.get_docstring()
            if docstring:
                result["docstring"] = docstring

        # Add Python-specific metadata
        if hasattr(raw_node, "metadata"):
            metadata = raw_node.metadata

            # Process decorators
            if "decorators" in metadata:
                result["decorators"] = metadata["decorators"]

            # Process async functions
            if "is_async" in metadata and metadata["is_async"]:
                result["is_async"] = True

        return result

    def get_language_specific_fields(self) -> List[str]:
        """
        Return additional fields specific to Python.

        Returns:
            A list of field names specific to Python
        """
        return [
            "decorators",
            "is_async",
            "comprehension_type",  # For list/dict/set comprehensions
            "is_generator",        # For generator functions/expressions
            "context_managers"     # For with statements
        ]
