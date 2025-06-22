"""
CST Converter

This module implements a converter for Concrete Syntax Tree (CST) nodes,
transforming them into a canonical dictionary representation.
"""

from typing import Dict, List, Any, Optional

from .base_converter import BaseConverter


class CSTConverter(BaseConverter):
    """
    Converter for CST nodes.

    This class transforms raw CST nodes from the parser into a canonical
    dictionary representation that can be serialized to JSON.
    """

    def convert(self, node: Any, language_type: str, context: Any = None) -> Dict[str, Any]:
        """
        Convert a CST node to a canonical dictionary representation.

        Args:
            node: The CST node to convert
            language_type: The language type
            context: Optional context information

        Returns:
            A dictionary representation of the CST node
        """
        adapter = self.get_adapter_for_language(language_type)
        if adapter:
            return adapter.convert_cst_node(node, context)

        # Fallback to generic conversion if no adapter is available
        return self._generic_convert(node, context)

    def _generic_convert(self, node: Any, context: Any = None) -> Dict[str, Any]:
        """
        Generic fallback conversion for CST nodes when no language adapter is available.

        Args:
            node: The CST node to convert
            context: Optional context information

        Returns:
            A dictionary representation of the CST node
        """
        # Handle the case where the node is None
        if node is None:
            return {
                "type": "UNKNOWN",
                "content": "",
                "range": {
                    "start": {"line": 0, "column": 0},
                    "end": {"line": 0, "column": 0}
                },
                "children": []
            }

        # Check if we have a dictionary-style node (newer binding style)
        if isinstance(node, dict):
            return self._convert_dict_node(node)

        # Otherwise assume it's an object-style node (older binding style)
        return self._convert_object_node(node)

    def _convert_dict_node(self, node: Dict[str, Any]) -> Dict[str, Any]:
        """
        Convert a dictionary-style CST node.

        Args:
            node: The CST node as a dictionary

        Returns:
            A canonical dictionary representation of the CST node
        """
        result = {
            "type": str(node.get("type", "UNKNOWN")),
            "content": str(node.get("content", "")),
            "range": self._extract_range_from_dict(node.get("range")),
            "children": []
        }

        # Process children
        if "children" in node and isinstance(node["children"], list):
            for child in node["children"]:
                if child is not None:
                    child_dict = self._generic_convert(child)
                    if child_dict:
                        result["children"].append(child_dict)

        return result

    def _convert_object_node(self, node: Any) -> Dict[str, Any]:
        """
        Convert an object-style CST node.

        Args:
            node: The CST node as an object

        Returns:
            A canonical dictionary representation of the CST node
        """
        result = {
            "type": self._safe_get_attribute(node, "get_type", "UNKNOWN"),
            "content": self._safe_get_attribute(node, "get_content", ""),
            "range": self._extract_range_from_object(node),
            "children": []
        }

        # Process children
        if hasattr(node, "get_children") and callable(getattr(node, "get_children")):
            children = node.get_children()
            if children:
                for child in children:
                    if child is not None:
                        child_dict = self._generic_convert(child)
                        if child_dict:
                            result["children"].append(child_dict)

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

    def _extract_range_from_dict(self, range_data: Optional[Dict[str, Any]]) -> Dict[str, Any]:
        """
        Extract range information from a dictionary-style range object.

        Args:
            range_data: The range data dictionary

        Returns:
            A canonical range dictionary
        """
        if not range_data or not isinstance(range_data, dict):
            return {
                "start": {"line": 0, "column": 0},
                "end": {"line": 0, "column": 0}
            }

        # Handle nested start/end dictionaries
        if "start" in range_data and "end" in range_data:
            start = range_data["start"]
            end = range_data["end"]

            if isinstance(start, dict) and isinstance(end, dict):
                return {
                    "start": {
                        "line": int(start.get("line", 0)),
                        "column": int(start.get("column", 0))
                    },
                    "end": {
                        "line": int(end.get("line", 0)),
                        "column": int(end.get("column", 0))
                    }
                }

        # Handle flattened structure
        return {
            "start": {
                "line": int(range_data.get("start_line", 0)),
                "column": int(range_data.get("start_column", 0))
            },
            "end": {
                "line": int(range_data.get("end_line", 0)),
                "column": int(range_data.get("end_column", 0))
            }
        }

    def _extract_range_from_object(self, node: Any) -> Dict[str, Any]:
        """
        Extract range information from an object-style node.

        Args:
            node: The CST node

        Returns:
            A canonical range dictionary
        """
        if not hasattr(node, "get_range") or not callable(getattr(node, "get_range")):
            return {
                "start": {"line": 0, "column": 0},
                "end": {"line": 0, "column": 0}
            }

        try:
            range_data = node.get_range()
            if range_data is None:
                return {
                    "start": {"line": 0, "column": 0},
                    "end": {"line": 0, "column": 0}
                }

            # Handle dictionary-style range
            if isinstance(range_data, dict):
                return self._extract_range_from_dict(range_data)

            # Handle object-style range with attributes
            if all(hasattr(range_data, attr) for attr in ["start_line", "end_line", "start_column", "end_column"]):
                return {
                    "start": {
                        "line": int(getattr(range_data, "start_line", 0)),
                        "column": int(getattr(range_data, "start_column", 0))
                    },
                    "end": {
                        "line": int(getattr(range_data, "end_line", 0)),
                        "column": int(getattr(range_data, "end_column", 0))
                    }
                }
        except Exception:
            pass

        # Return default range if extraction fails
        return {
            "start": {"line": 0, "column": 0},
            "end": {"line": 0, "column": 0}
        }
