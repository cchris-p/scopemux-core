"""
C++ Language Adapter

This module implements a language adapter for C++ source code,
handling C++-specific features and conversions.
"""

from typing import Dict, List, Any, Optional

from .base import LanguageAdapter


class CppAdapter(LanguageAdapter):
    """
    Language adapter for C++ source code.

    Handles C++-specific features such as:
    - Templates
    - Namespaces
    - Access specifiers
    - Operator overloading
    - Multiple inheritance
    """

    def __init__(self):
        """Initialize the C++ language adapter."""
        super().__init__("cpp", "C++")

    def extract_signature(self, node: Any, source_code: str) -> str:
        """
        Extract a C++ function signature including return type, templates, etc.

        Args:
            node: The function node
            source_code: The source code text

        Returns:
            A string representing the function signature
        """
        # If we have a direct signature from the node, use it
        if hasattr(node, "get_signature") and callable(getattr(node, "get_signature")):
            signature = node.get_signature()
            if signature:
                # Handle template prefix if present in raw data
                if hasattr(node, "raw_data") and isinstance(getattr(node, "raw_data"), dict):
                    if "template_parameters" in node.raw_data:
                        template_params = ", ".join(
                            node.raw_data["template_parameters"])
                        signature = f"template <{template_params}> {signature}"
                return signature

        # Otherwise try to construct from available parts
        name = node.get_name() if hasattr(node, "get_name") and callable(
            getattr(node, "get_name")) else "unknown"
        return_type = (
            node.get_return_type() if hasattr(node, "get_return_type") and callable(
                getattr(node, "get_return_type")) else "auto"
        )

        # Build parameter list
        params = "()"
        if hasattr(node, "get_parameters") and callable(getattr(node, "get_parameters")):
            param_list = node.get_parameters() or []
            if param_list:
                param_strs = []
                for param in param_list:
                    param_type = param.get_type() if hasattr(param, "get_type") else "auto"
                    param_name = param.get_name() if hasattr(param, "get_name") else ""
                    param_default = param.get_default() if hasattr(param, "get_default") else None

                    param_str = f"{param_type} {param_name}" if param_name else param_type
                    if param_default:
                        param_str = f"{param_str}={param_default}"

                    param_strs.append(param_str)

                params = f"({', '.join(param_strs)})"

        # Build full signature
        signature = f"{return_type} {name}{params}"

        # Add template prefix if available
        if hasattr(node, "raw_data") and isinstance(getattr(node, "raw_data"), dict):
            if "template_parameters" in node.raw_data:
                template_params = ", ".join(
                    node.raw_data["template_parameters"])
                signature = f"template <{template_params}> {signature}"

        # Add const qualifier if needed
        if hasattr(node, "raw_data") and isinstance(getattr(node, "raw_data"), dict):
            if node.raw_data.get("is_const", False):
                signature = f"{signature} const"

        return signature

    def generate_qualified_name(self, name: str, parent_node: Optional[Any] = None) -> str:
        """
        Generate qualified name following C++ conventions using :: operator.

        Args:
            name: The base name
            parent_node: The parent node, if any

        Returns:
            A string representing the qualified name
        """
        if not parent_node or not hasattr(parent_node, "qualified_name") or not parent_node.qualified_name:
            return name
        return f"{parent_node.qualified_name}::{name}"

    def process_special_cases(self, node: Any, context: Any) -> None:
        """
        Handle C++-specific features.

        Args:
            node: The node to process
            context: The parsing context
        """
        if not hasattr(node, "raw_data") or not isinstance(getattr(node, "raw_data"), dict):
            return

        raw_data = node.raw_data

        # Ensure we have a metadata object
        if not hasattr(node, "metadata"):
            node.metadata = {}

        # Process access specifiers
        if "access_specifier" in raw_data:
            node.metadata["access_specifier"] = raw_data["access_specifier"]

        # Process template parameters
        if "template_parameters" in raw_data:
            node.metadata["template_parameters"] = raw_data["template_parameters"]

        # Process inheritance
        if "base_classes" in raw_data:
            node.metadata["base_classes"] = raw_data["base_classes"]

        # Process namespace
        if "namespace" in raw_data:
            node.metadata["namespace"] = raw_data["namespace"]

        # Process modifiers (virtual, static, const, etc.)
        for modifier in ["is_virtual", "is_static", "is_const", "is_explicit", "is_inline"]:
            if modifier in raw_data and raw_data[modifier]:
                node.metadata[modifier] = True

    def convert_ast_node(self, raw_node: Any, context: Any) -> Dict[str, Any]:
        """
        Convert C++ AST node to canonical format.

        Args:
            raw_node: The raw AST node from the parser
            context: The parsing context

        Returns:
            A dictionary representing the converted AST node
        """
        # Start with base implementation
        result = super().convert_ast_node(raw_node, context)

        # Add C++-specific fields based on node type
        if hasattr(raw_node, "get_type") and callable(getattr(raw_node, "get_type")):
            node_type = raw_node.get_type()
            if node_type:
                result["type"] = node_type

        # Add function/method-specific data
        if result["type"] in ["FUNCTION", "METHOD"]:
            # Add return type
            if hasattr(raw_node, "get_return_type") and callable(getattr(raw_node, "get_return_type")):
                return_type = raw_node.get_return_type()
                if return_type:
                    result["return_type"] = return_type

            # Process parameters
            if hasattr(raw_node, "get_parameters") and callable(getattr(raw_node, "get_parameters")):
                param_list = raw_node.get_parameters() or []
                if param_list:
                    result["parameters"] = []
                    for param in param_list:
                        param_dict = {}
                        if hasattr(param, "get_name") and callable(getattr(param, "get_name")):
                            param_dict["name"] = param.get_name() or ""
                        if hasattr(param, "get_type") and callable(getattr(param, "get_type")):
                            param_dict["type"] = param.get_type() or "auto"
                        if hasattr(param, "get_default") and callable(getattr(param, "get_default")):
                            default = param.get_default()
                            if default:
                                param_dict["default"] = default
                        result["parameters"].append(param_dict)

        # Add class-specific data
        if result["type"] == "CLASS":
            # Add template information
            if hasattr(raw_node, "metadata") and "template_parameters" in raw_node.metadata:
                result["template_parameters"] = raw_node.metadata["template_parameters"]

            # Add base classes
            if hasattr(raw_node, "metadata") and "base_classes" in raw_node.metadata:
                result["base_classes"] = raw_node.metadata["base_classes"]

        # Add general C++ metadata
        if hasattr(raw_node, "metadata"):
            # Add access specifier
            if "access_specifier" in raw_node.metadata:
                result["access_specifier"] = raw_node.metadata["access_specifier"]

            # Add namespace
            if "namespace" in raw_node.metadata:
                result["namespace"] = raw_node.metadata["namespace"]

            # Add modifiers
            for modifier in ["is_virtual", "is_static", "is_const", "is_explicit", "is_inline"]:
                if modifier in raw_node.metadata and raw_node.metadata[modifier]:
                    result[modifier] = True

        return result

    def get_language_specific_fields(self) -> List[str]:
        """
        Return additional fields specific to C++.

        Returns:
            A list of field names specific to C++
        """
        return [
            "template_parameters",
            "access_specifier",
            "is_virtual",
            "is_static",
            "is_const",
            "is_explicit",
            "is_inline",
            "base_classes",
            "namespace",
            "operator_type"
        ]
