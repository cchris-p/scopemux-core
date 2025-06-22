"""
AST Converter

This module implements a converter for Abstract Syntax Tree (AST) nodes,
transforming them into a canonical dictionary representation.
"""

from typing import Dict, List, Any, Optional

from .base_converter import BaseConverter


class ASTConverter(BaseConverter):
    """
    Converter for AST nodes.

    This class transforms raw AST nodes from the parser into a canonical
    dictionary representation that can be serialized to JSON.
    """

    def convert(self, node: Any, language_type: str, context: Any = None) -> Dict[str, Any]:
        """
        Convert an AST node to a canonical dictionary representation.

        Args:
            node: The AST node to convert
            language_type: The language type
            context: Optional context information

        Returns:
            A dictionary representation of the AST node
        """
        adapter = self.get_adapter_for_language(language_type)
        if adapter:
            return adapter.convert_ast_node(node, context)

        # Fallback to generic conversion if no adapter is available
        return self._generic_convert(node, context)

    def _generic_convert(self, node: Any, context: Any = None) -> Dict[str, Any]:
        """
        Generic fallback conversion for AST nodes when no language adapter is available.

        Args:
            node: The AST node to convert
            context: Optional context information

        Returns:
            A dictionary representation of the AST node
        """
        if not node:
            return {
                "type": None,
                "name": None,
                "qualified_name": None,
                "docstring": None,
                "signature": None,
                "return_type": None,
                "parameters": [],
                "path": None,
                "system": None,
                "range": None,
                "raw_content": None,
                "children": [],
            }

        # Extract basic node properties safely
        result = {
            "type": self._safe_get_attribute(node, "get_type", "UNKNOWN"),
            "name": self._safe_get_attribute(node, "get_name", ""),
            "qualified_name": self._safe_get_attribute(node, "get_qualified_name", ""),
            "docstring": self._safe_get_attribute(node, "get_docstring", None),
            "signature": self._safe_get_attribute(node, "get_signature", None),
            "return_type": self._safe_get_attribute(node, "get_return_type", None),
            "parameters": self._extract_parameters(node),
            "path": self._safe_get_attribute(node, "get_path", None),
            "system": self._safe_get_attribute(node, "is_system", None),
            "range": self._extract_range(node),
            "raw_content": self._safe_get_attribute(node, "get_raw_content", None),
            "children": [],
        }

        # Recursively process children
        result["children"] = self._extract_children(node)

        return result

    def _safe_get_attribute(self, obj: Any, method_name: str, default: Any = None) -> Any:
        """
        Safely get an attribute from an object using a method name.

        Args:
            obj: The object to get the attribute from
            method_name: The name of the method to call
            default: The default value to return if the method doesn't exist or fails

        Returns:
            The result of calling the method, or the default value
        """
        if hasattr(obj, method_name) and callable(getattr(obj, method_name)):
            try:
                result = getattr(obj, method_name)()
                return result if result is not None else default
            except Exception:
                return default
        return default

    def _extract_parameters(self, node: Any) -> List[Dict[str, Any]]:
        """
        Extract parameters from a node.

        Args:
            node: The node to extract parameters from

        Returns:
            A list of parameter dictionaries
        """
        parameters = []

        if hasattr(node, "get_parameters") and callable(getattr(node, "get_parameters")):
            params = node.get_parameters()
            if params:
                for param in params:
                    param_dict = {
                        "name": self._safe_get_attribute(param, "get_name", None),
                        "type": self._safe_get_attribute(param, "get_type", None),
                        "default": self._safe_get_attribute(param, "get_default", None),
                    }
                    parameters.append(param_dict)

        return parameters

    def _extract_range(self, node: Any) -> Optional[Dict[str, Any]]:
        """
        Extract source range information from a node.

        Args:
            node: The node to extract range from

        Returns:
            A dictionary with range information, or None if not available
        """
        if hasattr(node, "get_range") and callable(getattr(node, "get_range")):
            try:
                range_data = node.get_range()
                if range_data:
                    return {
                        "start_line": getattr(range_data, "start_line", 0),
                        "start_column": getattr(range_data, "start_column", 0),
                        "end_line": getattr(range_data, "end_line", 0),
                        "end_column": getattr(range_data, "end_column", 0),
                    }
            except Exception:
                pass

        return None

    def _extract_children(self, node: Any) -> List[Dict[str, Any]]:
        """
        Extract and convert children from a node.

        Args:
            node: The node to extract children from

        Returns:
            A list of converted child dictionaries
        """
        children = []

        if hasattr(node, "get_children") and callable(getattr(node, "get_children")):
            child_nodes = node.get_children()
            if child_nodes:
                for child in child_nodes:
                    if child:
                        # Use the same conversion method recursively
                        # We don't know the language type here, so use generic conversion
                        child_dict = self._generic_convert(child)
                        if child_dict:
                            children.append(child_dict)

        return children
