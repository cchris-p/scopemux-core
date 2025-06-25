"""
File Utilities

This module provides utilities for file operations such as reading, writing,
comparing, and managing test files and directories.
"""

import os
import json
import difflib
from typing import Dict, Any, Optional, List, Tuple, Union


def read_file(file_path: str) -> str:
    """
    Read the contents of a file.

    Args:
        file_path: Path to the file to read

    Returns:
        The contents of the file as a string

    Raises:
        FileNotFoundError: If the file does not exist
    """
    if not os.path.exists(file_path):
        raise FileNotFoundError(f"File not found: {file_path}")

    with open(file_path, "r", encoding="utf-8") as f:
        return f.read()


def write_file(file_path: str, content: str) -> None:
    """
    Write content to a file, creating directories as needed.

    Args:
        file_path: Path to the file to write
        content: Content to write to the file
    """
    # Ensure the directory exists
    directory = os.path.dirname(file_path)
    if directory and not os.path.exists(directory):
        os.makedirs(directory, exist_ok=True)

    # Write the file
    with open(file_path, "w", encoding="utf-8") as f:
        f.write(content)


def read_json_file(file_path: str) -> Dict[str, Any]:
    """
    Read and parse a JSON file.

    Args:
        file_path: Path to the JSON file to read

    Returns:
        The parsed JSON content as a dictionary

    Raises:
        FileNotFoundError: If the file does not exist
        json.JSONDecodeError: If the file is not valid JSON
    """
    content = read_file(file_path)
    return json.loads(content)


def write_json_file(file_path: str, data: Dict[str, Any], indent: int = 2) -> None:
    """
    Write data to a JSON file, creating directories as needed.

    Args:
        file_path: Path to the JSON file to write
        data: Data to write to the file
        indent: Number of spaces to use for indentation
    """
    # Convert to JSON string
    json_str = json.dumps(data, indent=indent, sort_keys=True)

    # Write to file
    write_file(file_path, json_str)


def compare_files(file_path1: str, file_path2: str) -> List[str]:
    """
    Compare two files and return a unified diff.

    Args:
        file_path1: Path to the first file
        file_path2: Path to the second file

    Returns:
        A list of strings representing the unified diff
    """
    # Read the files
    content1 = read_file(file_path1).splitlines()
    content2 = read_file(file_path2).splitlines()

    # Generate the diff
    diff = list(difflib.unified_diff(
        content1,
        content2,
        fromfile=file_path1,
        tofile=file_path2,
        lineterm=""
    ))

    return diff


def get_expected_output_path(source_path: str, output_dir: Optional[str] = None, root_dir: Optional[str] = None) -> str:
    """
    Get the path for the expected output file.

    Args:
        source_path: Path to the source file
        output_dir: Directory to write output file (default: same as input)
        root_dir: Root directory for preserving subdirectory structure

    Returns:
        The path for the expected output file
    """
    # Use the full filename (including extension) as the base for the output file name
    base = os.path.basename(source_path)
    output_file_name = f"{base}.expected.json"

    if output_dir:
        # If root_dir is provided, preserve subdirectory structure
        if root_dir:
            rel_path = os.path.relpath(os.path.dirname(source_path), root_dir)
            output_subdir = os.path.join(output_dir, rel_path)
            os.makedirs(output_subdir, exist_ok=True)
            return os.path.join(output_subdir, output_file_name)
        else:
            os.makedirs(output_dir, exist_ok=True)
            return os.path.join(output_dir, output_file_name)
    else:
        # Use the same directory as the source file
        return os.path.join(os.path.dirname(source_path), output_file_name)


def find_source_files(dir_path: str, extensions: List[str]) -> List[str]:
    """
    Find all files with the given extensions in a directory and its subdirectories.

    Args:
        dir_path: Path to the directory to search
        extensions: List of file extensions to include (e.g., ['.c', '.cpp', '.py'])

    Returns:
        A list of file paths
    """
    if not os.path.isdir(dir_path):
        raise ValueError(f"Directory not found: {dir_path}")

    # Normalize extensions to include the dot
    normalized_extensions = [ext if ext.startswith(
        '.') else f'.{ext}' for ext in extensions]

    file_paths = []
    for root, _, files in os.walk(dir_path):
        for file in files:
            _, ext = os.path.splitext(file)
            if ext.lower() in normalized_extensions:
                file_path = os.path.join(root, file)
                # Skip expected.json files
                if not file_path.endswith('.expected.json'):
                    file_paths.append(file_path)

    return file_paths
