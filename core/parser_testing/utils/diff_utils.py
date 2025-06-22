"""
Diff Utilities

This module provides utilities for comparing and displaying differences
between JSON structures, particularly for AST and CST comparisons.
"""

import json
import difflib
from typing import Dict, Any, List, Tuple, Optional, Union


def generate_json_diff(
    old_json: Dict[str, Any],
    new_json: Dict[str, Any],
    indent: int = 2,
    context_lines: int = 3
) -> List[str]:
    """
    Generate a unified diff between two JSON structures.

    Args:
        old_json: The original JSON structure
        new_json: The new JSON structure
        indent: Number of spaces to use for indentation
        context_lines: Number of context lines to include in the diff

    Returns:
        A list of strings representing the unified diff
    """
    # Convert JSON to formatted strings
    old_str = json.dumps(old_json, indent=indent, sort_keys=True).splitlines()
    new_str = json.dumps(new_json, indent=indent, sort_keys=True).splitlines()

    # Generate the diff
    diff = list(difflib.unified_diff(
        old_str,
        new_str,
        fromfile="old",
        tofile="new",
        lineterm="",
        n=context_lines
    ))

    return diff


def highlight_diff(diff_lines: List[str]) -> str:
    """
    Add ANSI color highlighting to a diff.

    Args:
        diff_lines: A list of strings representing a unified diff

    Returns:
        A string with ANSI color highlighting
    """
    result = []
    for line in diff_lines:
        if line.startswith('+'):
            # Green for additions
            result.append(f"\033[92m{line}\033[0m")
        elif line.startswith('-'):
            # Red for deletions
            result.append(f"\033[91m{line}\033[0m")
        elif line.startswith('^'):
            # Cyan for change indicators
            result.append(f"\033[96m{line}\033[0m")
        elif line.startswith('@@'):
            # Magenta for diff headers
            result.append(f"\033[95m{line}\033[0m")
        else:
            # Normal color for context
            result.append(line)

    return '\n'.join(result)


def compare_ast_nodes(
    node1: Dict[str, Any],
    node2: Dict[str, Any],
    path: str = "",
    ignore_fields: Optional[List[str]] = None
) -> List[Tuple[str, Any, Any]]:
    """
    Compare two AST nodes and return a list of differences.

    Args:
        node1: The first AST node
        node2: The second AST node
        path: The current path in the AST
        ignore_fields: A list of field names to ignore in the comparison

    Returns:
        A list of tuples containing (path, value1, value2) for each difference
    """
    if ignore_fields is None:
        ignore_fields = []

    differences = []

    # Get all keys from both nodes
    all_keys = set(node1.keys()) | set(node2.keys())

    for key in all_keys:
        if key in ignore_fields:
            continue

        # Construct the current path
        current_path = f"{path}.{key}" if path else key

        # Check if the key is present in both nodes
        if key not in node1:
            differences.append((current_path, None, node2[key]))
            continue

        if key not in node2:
            differences.append((current_path, node1[key], None))
            continue

        # Get the values
        value1 = node1[key]
        value2 = node2[key]

        # Compare the values
        if isinstance(value1, dict) and isinstance(value2, dict):
            # Recursively compare dictionaries
            child_diffs = compare_ast_nodes(
                value1, value2, current_path, ignore_fields)
            differences.extend(child_diffs)
        elif isinstance(value1, list) and isinstance(value2, list):
            # Compare lists
            if len(value1) != len(value2):
                differences.append(
                    (current_path, f"List length: {len(value1)}", f"List length: {len(value2)}"))
            else:
                # Compare each item in the list
                for i, (item1, item2) in enumerate(zip(value1, value2)):
                    if isinstance(item1, dict) and isinstance(item2, dict):
                        child_diffs = compare_ast_nodes(
                            item1, item2, f"{current_path}[{i}]", ignore_fields)
                        differences.extend(child_diffs)
                    elif item1 != item2:
                        differences.append(
                            (f"{current_path}[{i}]", item1, item2))
        elif value1 != value2:
            differences.append((current_path, value1, value2))

    return differences


def format_differences(differences: List[Tuple[str, Any, Any]]) -> List[str]:
    """
    Format a list of differences for display.

    Args:
        differences: A list of tuples containing (path, value1, value2)

    Returns:
        A list of formatted strings describing the differences
    """
    result = []

    for path, value1, value2 in differences:
        if value1 is None:
            result.append(f"Added: {path} = {value2}")
        elif value2 is None:
            result.append(f"Removed: {path} = {value1}")
        else:
            result.append(f"Changed: {path}")
            result.append(f"  - {value1}")
            result.append(f"  + {value2}")

    return result
